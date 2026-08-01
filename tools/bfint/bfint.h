#ifndef BFINT_H
#define BFINT_H

#include <array>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <random>
#include <stack>
#include <vector>

struct Options {
  int          err{0};
  int          tapeLength{30000};
  std::string  bfFile;
  std::string  profileFile;
  bool         randomEnabled{false};
  int          randMax{0};
  bool         randomWarningEnabled{true};
  bool         gamingMode{false};
};

class BFInterpreter {

  Options d_opt;

  using RngType = std::mt19937;
  std::uniform_int_distribution<RngType::result_type> d_uniformDist;
  RngType d_rng;

  struct OpCode {
    char op;
    size_t arg;
  };

  std::vector<OpCode> d_code;

  enum Instructions {
    PLUS,
    MINUS,
    LEFT,
    RIGHT,
    START_LOOP,
    END_LOOP,
    PRINT,
    READ,
    RAND,
    NUM_OPS
  };

  struct Stats {
    size_t maxAddress{0};
    size_t maxNestingDepth{0};
    std::array<std::uint64_t, NUM_OPS> opCount{};
    std::chrono::steady_clock::duration elapsed{};
    bool interrupted{false};
    bool failed{false};
  };

  static volatile std::sig_atomic_t s_interrupted;

public:
  BFInterpreter(Options const &opt);
  int run();

private:
  void preprocess(std::istream &code);
  void printCurses(char c);
  void random(char &c);
  void printStats(Stats const &stats);
  void handleAnsi(std::string &ansiStr, bool const force);
  static void handleSignal(int sig);
  static void finishCurses();
};

#endif
