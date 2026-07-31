#ifndef BFINT_H
#define BFINT_H

#include <vector>
#include <stack>
#include <random>
#include <iostream>

struct Options {
  int          err{0};
  int          tapeLength{30000};
  std::string  bfFile;
  bool         randomEnabled{false};
  int          randMax{0};
  bool         randomWarningEnabled{true};
  bool         gamingMode{false};
};

class BFInterpreter {
  
  std::vector<char> d_array;
  std::string d_code;
  size_t d_arrayPointer{0};
  size_t d_codePointer{0};
  std::stack<int> d_loopStack;

  using RngType = std::mt19937;
  std::uniform_int_distribution<RngType::result_type> d_uniformDist;
  RngType d_rng;

  // Options
  bool const d_randomEnabled{false};
  int  const d_randMax{0};
  bool const d_randomWarningEnabled{true};
  bool const d_gamingMode{false};
    
  enum Op: char {
      PLUS  = '+',
      MINUS = '-',
      LEFT  = '<',
      RIGHT = '>',
      START_LOOP = '[',
      END_LOOP = ']',
      PRINT = '.',
      READ = ',',
      RAND = '?',
    };

  struct OpCode {
    Op op;
    size_t arg;
  };

  public:
  BFInterpreter(Options const &opt);
  int run();

private:
  static std::vector<OpCode> preprocess(std::string const &code);
  
  int run(std::istream &in, std::ostream &out);
  int consume(Op op);
  void plus(int n);
  void minus(int n);
  void pointerInc(int n);
  void pointerDec(int n);
  void startLoop(int &current, int dest);
  void endLoop(int &current, int dest);
  void print();
  void printStream(std::ostream &out);
  void printCurses();
  void read();
  void readStream(std::istream &in);
  void readCurses();
  void random();
  void printState();
  void handleAnsi(std::string &ansiStr, bool const force);
  static void finish(int sig);
  void reset();
};



#endif
