#include <cassert>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <utility>

#include <ncurses.h>

#include "bfint.h"

volatile std::sig_atomic_t BFInterpreter::s_interrupted = 0;

BFInterpreter::BFInterpreter(Options const &opt):
  d_opt(opt),
  d_uniformDist(0, (opt.randMax != 0) ? opt.randMax : 255)
{
  if (opt.bfFile.empty()) {
    preprocess(std::cin);
  }
  else {
    std::ifstream file(opt.bfFile);
    if (!file.is_open()) {
      throw std::string("File not found: ") + opt.bfFile;
    }
    preprocess(file);
  }

  // init rng
  auto t0 = std::chrono::system_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0).count();
  d_rng.seed(ms);
}

void BFInterpreter::preprocess(std::istream &in) {
  std::vector<OpCode> result;
  std::stack<size_t> loopStack;

  auto const insert = [&](char op) {
    if (std::string("[].,?").contains(op)) {
      result.push_back({op, 0});
    }
  };

  auto const insertRepeatable = [&](char op) {
    if (!result.empty() && op == result.back().op) {
      ++result.back().arg;
    }
    else {
      result.push_back({op, 1});
    }
  };

  auto const insertStartLoop = [&] {
    result.push_back({'[', 0});
    loopStack.push(result.size() - 1);
  };

  auto const insertEndLoop = [&] {
    if (loopStack.empty()) {
      throw std::string("Error: unmatched ']'.");
    }

    size_t const fromIndex = loopStack.top();
    result.push_back({']', fromIndex});
    loopStack.pop();

    size_t const toIndex = result.size() - 1;
    result[fromIndex].arg = toIndex;
  };

  char c;
  while (in >> c) {
    switch (c) {
    case '<':
    case '>':
    case '+':
    case '-': insertRepeatable(c); break;
    case '[': insertStartLoop();   break;
    case ']': insertEndLoop();     break;
    default:  insert(c);           break;
    }
  }

  if (!loopStack.empty()) {
    throw std::string("Error: unmatched '['.");
  }

  std::swap(d_code, result);
}

int BFInterpreter::run() {
  if (d_opt.tapeLength <= 0) {
    throw std::string("Error: tape length must be positive.");
  }

  s_interrupted = 0;
  auto const previousHandler = std::signal(SIGINT, &BFInterpreter::handleSignal);

  bool cursesActive = false;
  if (d_opt.gamingMode) {
    auto *win = initscr();
    cursesActive = true;
    scrollok(win, true);
    cbreak();
    noecho();
    nonl();
    nodelay(stdscr, TRUE);
    curs_set(0);
  }

  std::vector<char> tape(static_cast<size_t>(d_opt.tapeLength));
  size_t codePointer = 0;
  int dataPointer = 0;
  int depth = 0;
  Stats stats{};
  auto const startedAt = std::chrono::steady_clock::now();
  bool timingStopped = false;

  auto const stopTiming = [&] {
    if (!timingStopped) {
      stats.elapsed = std::chrono::steady_clock::now() - startedAt;
      timingStopped = true;
    }
  };

  auto const cleanup = [&] {
    if (cursesActive) {
      finishCurses();
      cursesActive = false;
    }

    if (previousHandler != SIG_ERR) {
      std::signal(SIGINT, previousHandler);
    }
  };

  auto const extendTape = [&] {
    while (dataPointer >= static_cast<int>(tape.size())) {
      tape.resize(2 * tape.size());
    }
  };

  auto const checkNegative = [&] {
    if (dataPointer < 0) {
      throw std::string("Error: trying to decrement pointer beyond beginning.");
    }
    if (depth < 0) {
      throw std::string("Error: unbalanced loops.");
    }
  };

  auto const plus = [&](size_t n) {
    stats.opCount[PLUS] += n;
    tape[dataPointer] += static_cast<char>(n);
  };

  auto const minus = [&](size_t n) {
    stats.opCount[MINUS] += n;
    tape[dataPointer] -= static_cast<char>(n);
  };

  auto const right = [&](size_t n) {
    stats.opCount[RIGHT] += n;
    dataPointer += static_cast<int>(n);
    extendTape();

    if (static_cast<size_t>(dataPointer) > stats.maxAddress) {
      stats.maxAddress = static_cast<size_t>(dataPointer);
    }
  };

  auto const left = [&](size_t n) {
    stats.opCount[LEFT] += n;
    dataPointer -= static_cast<int>(n);
    checkNegative();
  };

  auto const startLoop = [&](size_t dest) {
    ++stats.opCount[START_LOOP];
    if (tape[dataPointer]) {
      if (static_cast<size_t>(++depth) > stats.maxNestingDepth) {
        stats.maxNestingDepth = static_cast<size_t>(depth);
      }
    }
    else {
      codePointer = dest;
    }
  };

  auto const endLoop = [&](size_t dest) {
    ++stats.opCount[END_LOOP];
    if (tape[dataPointer]) {
      codePointer = dest;
    }
    else {
      --depth;
      checkNegative();
    }
  };

  auto const print = [&] {
    ++stats.opCount[PRINT];
    char const c = tape[dataPointer];
    if (d_opt.gamingMode) {
      printCurses(c);
    }
    else {
      std::cout << c << std::flush;
    }
  };

  auto const read = [&] {
    ++stats.opCount[READ];
    if (d_opt.gamingMode) {
      int const c = getch();
      tape[dataPointer] = (c < 0) ? 0 : static_cast<char>(c);
    }
    else {
      char c = 0;
      if (!std::cin.get(c)) {
        c = 0;
      }
      tape[dataPointer] = c;
    }
  };

  auto const random = [&] {
    ++stats.opCount[RAND];
    BFInterpreter::random(tape[dataPointer]);
  };

  try {
    while (codePointer < d_code.size()) {
      if (s_interrupted) {
        stats.interrupted = true;
        break;
      }

      OpCode const instr = d_code[codePointer];
      switch (instr.op) {
      case '<': left(instr.arg);      break;
      case '>': right(instr.arg);     break;
      case '+': plus(instr.arg);      break;
      case '-': minus(instr.arg);     break;
      case '.': print();              break;
      case ',': read();               break;
      case '[': startLoop(instr.arg); break;
      case ']': endLoop(instr.arg);   break;
      case '?': random();             break;
      default: std::unreachable();
      }

      ++codePointer;
    }

    stopTiming();

    if (d_opt.gamingMode && !stats.interrupted) {
      nodelay(stdscr, false);
      getch();
    }

    cleanup();
    printStats(stats);
  }
  catch (...) {
    stats.failed = true;
    stopTiming();
    cleanup();
    printStats(stats);
    throw;
  }

  if (stats.interrupted) {
    std::cerr << "\nInterrupted by SIGINT.\n";
    return 130;
  }

  return 0;
}

void BFInterpreter::printCurses(char c) {
  static char const ESC = 27; // Control char
  static std::string ansiBuffer;
    
  if (c == ESC) {
    if (ansiBuffer.empty()) {
      ansiBuffer.push_back(c);
    } else {
      handleAnsi(ansiBuffer, true);
      ansiBuffer.push_back(c);
    }
  }
  else {
    if (ansiBuffer.empty()) {
      addch(c);
    } else {
      ansiBuffer.push_back(c);
      handleAnsi(ansiBuffer, false);
    }
  }
        
  refresh();
}

void BFInterpreter::random(char &dest) {
  static bool warned = false;

  if (d_opt.randomEnabled) {
    dest = static_cast<char>(d_uniformDist(d_rng));
  }
  else if (d_opt.randomWarningEnabled && !warned) {
    static std::string const warning =
      "\n"
      "=========================== !!!!!! ==============================\n"
      "Warning: BF-code contains '?'-commands, which may be\n"
      "interpreted as the random-operation, an extension to the\n"
      "canonical BF instruction set. This extension can be enabled\n"
      "with the --random option.\n"
      "This warning can be disabled with the --no-random-warning option.\n"
      "=========================== !!!!!! ==============================\n";

    if (d_opt.gamingMode) {
      addstr(warning.c_str());
      refresh();
    }
    else {
      std::cerr << warning;
    }

    warned = true;
  }
}


void BFInterpreter::handleAnsi(std::string &ansiStr, bool const force) {
  static char const ESC = 27; // Control char
  assert(ansiStr.length() > 1 && "handleAnsi called with less than 2 characters");
  assert(ansiStr[0] == ESC && "handleAnsi called on string not starting with ESC");

  auto const flush = [&] {
    addstr(ansiStr.c_str());
    ansiStr.clear();
  };
    
  if (ansiStr.length() == 2 && ansiStr[1] != '[') {
    // ESC not followed by '[' -> not ansi
    flush();
    return;
  }
  
  if (ansiStr.length() < 3) {
    if (force) flush();
    return; // cannot be a complete ansi escape sequence
  }

  // 3 or more characters present
  bool handled = true;  
  int row, col;
  getyx(stdscr, row, col);

  // helpers
  auto cursorUp = [&] {
    int n = std::stoi(ansiStr.substr(2, ansiStr.length() - 3));
    if (row) {
      row = std::max(0, row - n);
      move(row, col);
    }
  };

  auto cursorDown = [&] {
    int n = std::stoi(ansiStr.substr(2, ansiStr.length() - 3));
    move(row + n, col);
  };

  auto cursorRight = [&] {
    int n = std::stoi(ansiStr.substr(2, ansiStr.length() - 3));
    move(row, col + n);
  };

  auto cursorLeft = [&] {
    if (col) {
      int n = std::stoi(ansiStr.substr(2, ansiStr.length() - 3));      
      col = std::max(0, col - n);
      move(row, col);
      clrtoeol();
    }
  };

  auto cursorTo = [&] {
    if (ansiStr.length() == 3) {
      move(0, 0);
      return;
    }
            
    size_t const pos = ansiStr.find(';');
    if (pos == std::string::npos) {
      flush();
      return;
    }

    row = std::stoi(ansiStr.substr(2, pos - 2)) - 1;
    col = std::stoi(ansiStr.substr(pos + 1, ansiStr.length() - pos - 2)) - 1;
    move(row, col);
  };

  auto clearLine = [&] {
    int n = (ansiStr.length() == 3) ? 0 : std::stoi(ansiStr.substr(2, ansiStr.length() - 3));
    switch (n) {
    case 0: clrtoeol(); return;
    case 1: {
      move(row, 0);
      addstr(std::string(col, ' ').c_str());
      return;
    }
    case 2: {
      move(row, 0);
      clrtoeol();
      move(row, col);
      return;
    }
    default: handled = false;
    }
  };

  auto clearScreen = [&] {
    int n = (ansiStr.length() == 3) ? 0 : std::stoi(ansiStr.substr(2, ansiStr.length() - 3));
    switch (n) {
    case 0: clrtobot(); return;
    case 1: {
      for (int i = 0; i <= row; ++i) {
	move(i, 0);
	clrtoeol();
      }
      move(row, col);
      return;
    }
    case 2: {
      move(0,0);
      clrtobot();
      return;
    }
    default: handled = false;
    }
  };
  
  switch (ansiStr.back()) {
  case 'A': cursorUp();    break;
  case 'B': cursorDown();  break;
  case 'C': cursorRight(); break;
  case 'D': cursorLeft();  break;
  case 'H': cursorTo();    break;
  case 'K': clearLine();   break;
  case 'J': clearScreen(); break;
  default:  handled = false;
  }
    
  if (handled) {
    ansiStr.clear();
    return;
  }
    
  // ANSI sequence not yet terminated. Check if character is allowed
  if (std::string("0123456789;").find(ansiStr.back()) == std::string::npos || force)
    flush();

  // Still going. Don't do anything and wait for next call
}

void BFInterpreter::printStats(Stats const &stats) {
  if (d_opt.profileFile.empty()) {
    return;
  }

  std::ofstream file(d_opt.profileFile);
  if (!file) {
    std::cerr << "Could not open file for writing: " << d_opt.profileFile << '\n';
    return;
  }

  static constexpr std::array<char, NUM_OPS> opChars{
    '+', '-', '<', '>', '[', ']', '.', ',', '?'
  };

  auto const formatInteger = [](std::uint64_t value) {
    std::string result = std::to_string(value);
    for (std::ptrdiff_t pos = static_cast<std::ptrdiff_t>(result.size()) - 3;
         pos > 0;
         pos -= 3) {
      result.insert(static_cast<size_t>(pos), ",");
    }
    return result;
  };

  auto const formatDuration = [](std::chrono::steady_clock::duration duration) {
    double const seconds = std::chrono::duration<double>(duration).count();
    std::ostringstream out;

    if (seconds >= 1.0) {
      out << std::fixed << std::setprecision(3) << seconds << " s";
    }
    else {
      out << std::fixed << std::setprecision(3) << seconds * 1000.0 << " ms";
    }

    return out.str();
  };

  auto const formatRate = [](double rate) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);

    if (rate >= 1'000'000'000.0) {
      out << rate / 1'000'000'000.0 << " Gop/s";
    }
    else if (rate >= 1'000'000.0) {
      out << rate / 1'000'000.0 << " Mop/s";
    }
    else if (rate >= 1'000.0) {
      out << rate / 1'000.0 << " kop/s";
    }
    else {
      out << rate << " op/s";
    }

    return out.str();
  };

  std::uint64_t executedOps = 0;
  for (std::uint64_t const count : stats.opCount) {
    executedOps += count;
  }

  std::uint64_t sourceOps = 0;
  for (OpCode const &instr : d_code) {
    switch (instr.op) {
    case '+':
    case '-':
    case '<':
    case '>': sourceOps += instr.arg; break;
    default:  ++sourceOps;            break;
    }
  }

  double const seconds = std::chrono::duration<double>(stats.elapsed).count();
  double const rate = (seconds > 0.0)
                    ? static_cast<double>(executedOps) / seconds
                    : 0.0;

  char const *status = stats.interrupted ? "interrupted (SIGINT)"
                     : stats.failed      ? "failed"
                                         : "completed";

  file << "     Brainfuck Interpreter Profile\n"
       << "========================================\n"
       << " Program:                  "
       << (d_opt.bfFile.empty() ? "<stdin>" : d_opt.bfFile) << '\n'
       << " Status:                   " << status << '\n'
       << " Elapsed time:             " << formatDuration(stats.elapsed) << '\n'
       << '\n'
       << " Program\n"
       << " -------\n"
       << " Brainfuck instructions:   " << formatInteger(sourceOps) << '\n'
       << " Preprocessed operations:  " << formatInteger(d_code.size()) << '\n'
       << '\n'
       << " Execution\n"
       << " ---------\n"
       << " Executed instructions:    " << formatInteger(executedOps) << '\n'
       << " Execution rate:           " << formatRate(rate) << '\n'
       << " Highest tape address:     " << formatInteger(stats.maxAddress) << '\n'
       << " Tape cells touched:       " << formatInteger(stats.maxAddress + 1) << '\n'
       << " Maximum loop depth:       " << formatInteger(stats.maxNestingDepth) << '\n'
       << '\n'
       << " Instruction counts\n"
       << " ------------------\n"
       << std::left
       << std::setw(5)  << " Op"
       << std::right
       << std::setw(18) << "Count"
       << std::setw(12) << "Percent" << '\n';

  for (size_t i = 0; i < NUM_OPS; ++i) {
    double const percentage = (executedOps != 0)
                            ? 100.0 * static_cast<double>(stats.opCount[i])
                                    / static_cast<double>(executedOps)
                            : 0.0;

    file << " "
	 << std::left
         << std::setw(3)  << opChars[i]
         << std::right
         << std::setw(18) << formatInteger(stats.opCount[i])
         << std::setw(11) << std::fixed << std::setprecision(2) << percentage
         << "%\n";
  }
}

void BFInterpreter::handleSignal(int) {
  s_interrupted = 1;
}

void BFInterpreter::finishCurses() {
  endwin();
}
