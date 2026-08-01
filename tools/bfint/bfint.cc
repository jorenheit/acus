#include <cassert>
#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>
#include <utility>
#include <ncurses.h>

#include "bfint.h"

BFInterpreter::BFInterpreter(Options const &opt):
  d_array(opt.tapeLength),
  d_uniformDist(0, (opt.randMax != 0) ? opt.randMax : 255),
  d_randomEnabled(opt.randomEnabled),
  d_randMax(opt.randMax),
  d_randomWarningEnabled(opt.randomWarningEnabled),
  d_gamingMode(opt.gamingMode)
{
  // init code
  std::ifstream file(opt.bfFile);
  if (!file.is_open())
    throw std::string("File not found: ") + opt.bfFile;
 
  std::stringstream buffer;
  buffer << file.rdbuf();
  d_code = buffer.str();

  // init rng
  auto t0 = std::chrono::system_clock::now().time_since_epoch();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t0).count();
  d_rng.seed(ms);
}

void BFInterpreter::reset() {
  std::fill(d_array.begin(), d_array.end(), 0);
  d_arrayPointer = 0;
}

std::vector<BFInterpreter::OpCode> BFInterpreter::preprocess(std::string const &code) {
  std::vector<OpCode> result;
  std::stack<int> loopStack;

  auto insert = [&](Op op) {
    result.push_back({op, 0});
  };

  auto insertRepeatable = [&](Op op) {
    if (result.size() > 0 && op == result.back().op) {
      ++result.back().arg;
    } else {
      result.push_back({op, 1});
    }
  };

  auto insertStartLoop = [&] {
    result.push_back({START_LOOP, 0});
    loopStack.push(result.size() - 1);
  };

  auto insertEndLoop = [&] {
    size_t const fromIndex = loopStack.top();
    result.push_back({END_LOOP, fromIndex});
    loopStack.pop();      

    size_t const toIndex = result.size() - 1;
    result[fromIndex].arg = toIndex;
  };

  for (char c: code) {
    switch (Op op = static_cast<Op>(c)) {
    case LEFT:
    case RIGHT:
    case PLUS:
    case MINUS:      insertRepeatable(op); break;
    case START_LOOP: insertStartLoop();    break;
    case END_LOOP:   insertEndLoop();      break;
    default:         insert(op);           break;
    }
  }

  return result;
}

int BFInterpreter::run() {
    
  // Setup ncurses window
  if (d_gamingMode) {
    auto win = initscr();
    scrollok(win, true);
    cbreak();
    noecho();
    nonl();
    nodelay(stdscr, TRUE);
    curs_set(0);

    signal(SIGINT,
	   [](int sig){
	     finish(sig);
	   });
  }

  std::vector<OpCode> code = preprocess(d_code);

  reset();  
  int codePointer = 0;
  while (codePointer < code.size()) {

    OpCode instr = code[codePointer];
    switch (instr.op) {
    case LEFT:       pointerDec(instr.arg);		break;
    case RIGHT:      pointerInc(instr.arg);		break;
    case PLUS:       plus(instr.arg);			break;
    case MINUS:      minus(instr.arg);			break;
    case PRINT:      print();				break;
    case READ:       read();				break;
    case START_LOOP: startLoop(codePointer, instr.arg);	break;
    case END_LOOP:   endLoop(codePointer, instr.arg);	break;
    case RAND:       random();				break;
    default: std::unreachable();
    }

    ++codePointer;
  }

  if (d_gamingMode) {
    nodelay(stdscr, false);
    getch();
    finish(0);
  }

  return 0;
}

void BFInterpreter::plus(int n) {
  d_array[d_arrayPointer] += n;
}
    
void BFInterpreter::minus(int n) {
  d_array[d_arrayPointer] -= n;
}

void BFInterpreter::pointerInc(int n) {
  d_arrayPointer += n;

  while (d_arrayPointer >= d_array.size())
    d_array.resize(2 * d_array.size());
}

void BFInterpreter::pointerDec(int n) {
  if (d_arrayPointer == 0)
    throw std::string("Error: trying to decrement pointer beyond beginning.");

  d_arrayPointer -= n;
}

void BFInterpreter::startLoop(int &current, int dest) {
  current = (d_array[d_arrayPointer] != 0) ? current : dest;
}

void BFInterpreter::endLoop(int &current, int dest) {
  current = (d_array[d_arrayPointer] != 0) ? dest : current;
}

void BFInterpreter::print() {
  if (d_gamingMode)
    printCurses();
  else
    printStream(std::cout);
}

void BFInterpreter::printStream(std::ostream &out) {
  out << (char)d_array[d_arrayPointer] << std::flush;
}

void BFInterpreter::printCurses() {
  static char const ESC = 27; // Control char
  static std::string ansiBuffer;
    
  char const c = d_array[d_arrayPointer];
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

void BFInterpreter::read() {
  if (d_gamingMode)
    readCurses();
  else
    readStream(std::cin);
}

void BFInterpreter::readStream(std::istream &in)
{
  char c;
  in.get(c);
  d_array[d_arrayPointer] = c;
}

void BFInterpreter::readCurses()
{ 
  int c = getch();
  d_array[d_arrayPointer] = (c < 0) ? 0 : static_cast<char>(c);
}

void BFInterpreter::random() {
  static bool warned = false;
  if (d_randomEnabled) {
    auto val = d_uniformDist(d_rng);
    d_array[d_arrayPointer] = val;
  }
  else if (d_randomWarningEnabled && !warned)
  {
    static std::string const warning =
      "\n"
      "=========================== !!!!!! ==============================\n"
      "Warning: BF-code contains '?'-commands, which may be\n"
      "interpreted as the random-operation, an extension to the\n"
      "canonical BF instructionset. This extension can be enabled\n"
      "with the --random option.\n"
      "This warning can be disabled with the --no-random-warning option.\n"
      "=========================== !!!!!! ==============================\n";
                        
    if (!d_gamingMode)
      std::cerr << warning;
    else
    {
      addstr(warning.c_str());
      assert(false);
    }
    warned = true;
  }
}

void BFInterpreter::printState()
{
  for (auto x: d_array)
    std::cout << (int)x << ' ';
  std::cout << '\n';
}

void BFInterpreter::finish(int sig)
{
  endwin();
  if (sig == SIGINT)
    exit(0);
}

