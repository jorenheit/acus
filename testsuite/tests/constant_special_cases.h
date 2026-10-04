// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace bftest {
  enum class ConstantOperation { Multiply, Divide, Modulo };

  // Runtime input on the left and a literal on the right deliberately prevent
  // literal folding while selecting the constant slot-operation dispatch.
  inline TestCase constantOperationCase(std::string name, unsigned bits,
                                       bool isSigned, ConstantOperation operation,
                                       std::vector<int> values,
                                       std::vector<int> constants,
                                       bool chained = false) {
    auto appendRaw = [bits](std::string &out, std::int64_t value) {
      auto const raw = static_cast<std::uint64_t>(value);
      out.push_back(static_cast<char>(raw & 0xff));
      if (bits == 16) out.push_back(static_cast<char>((raw >> 8) & 0xff));
    };
    auto calculate = [bits, operation, chained](int value, int constant) -> std::int64_t {
      std::int64_t result = value;
      if (operation == ConstantOperation::Multiply) result *= constant;
      else if (operation == ConstantOperation::Divide)
        result = constant == 0 ? ((std::int64_t{1} << bits) - 1) : result / constant;
      else result = constant == 0 ? 0 : result % constant;
      if (chained) result /= constant; // Dedicated positive-denominator halving cases.
      return result;
    };
    std::string input, expected;
    for (int constant : constants) {
      for (int value : values) {
        for (bool expression : {false, true}) {
          input.push_back(static_cast<char>(static_cast<unsigned>(value) & 0xff));
          appendRaw(expected, calculate(value, constant));
          if (expression) appendRaw(expected, value); // Pure expression preserves its source.
          expected += 'G'; // Adjacent local must survive scratch work.
        }
      }
    }
    return TestCase{
      .name = std::move(name),
      .buildProgram = [=] {
        using namespace acus::api;
        Assembler c;
        c.program("constant_regression", "main").begin();
        auto type = bits == 8 ? (isSigned ? ts::s8() : ts::u8())
                             : (isSigned ? ts::s16() : ts::u16());
        auto literalValue = [=](int value) -> literal::Literal {
          if (bits == 8) return isSigned ? literal::s8(value) : literal::u8(value);
          return isSigned ? literal::s16(value) : literal::u16(value);
        };
        c.function("main").begin();
        c.declareLocal("x", type);
        c.declareLocal("result", type);
        c.declareLocal("guard", ts::u8());
        c.assign("guard", literal::u8('G'));
        for (int constant : constants) {
          for (int value : values) {
            for (bool expression : {false, true}) {
              c.scope().begin();
              // read() consumes one byte even for a 16-bit slot. Initialize
              // the complete value first, then supply its low byte at runtime.
              c.assign("x", literalValue(value));
              c.read("x");
              auto rhs = literalValue(constant);
              if (expression) {
                if (operation == ConstantOperation::Multiply) c.assign("result", c.mul("x", rhs));
                else if (operation == ConstantOperation::Divide) c.assign("result", c.div("x", rhs));
                else c.assign("result", c.mod("x", rhs));
              } else {
                if (operation == ConstantOperation::Multiply) c.mulAssign("x", rhs);
                else if (operation == ConstantOperation::Divide) c.divAssign("x", rhs);
                else c.modAssign("x", rhs);
              }
              char const *target = expression ? "result" : "x";
              if (chained) c.divAssign(target, rhs);
              c.write(target);
              if (expression) c.write("x");
              c.write("guard");
              c.endScope();
            }
          }
        }
        c.returnFromFunction();
        c.endFunction();
        c.endProgram();
        return c.brainfuck("constant_regression");
      },
      .expectedOutput = std::move(expected),
      .input = std::move(input),
      .requireHalt = true,
      .maxSteps = 200'000'000,
    };
  }

  inline void addConstantSpecialCaseTests(std::vector<TestCase> &tests) {
    std::vector<int> u8, s8;
    for (int i = 0; i < 256; ++i) { u8.push_back(i); s8.push_back(i - 128); }
    std::vector<int> const bytes{0, 1, 2, 3, 7, 8, 15, 16, 63, 64, 127, 128, 129, 254, 255};
    std::vector<int> const signedBytes{-128, -127, -65, -17, -9, -3, -2, -1, 0, 1, 2, 3, 17, 64, 127};
    std::vector<int> const words{0, 1, 2, 3, 127, 128, 255, 256, 257, 511, 512, 513, 0x1234, 0x1235, 0x7fff, 0x8000, 0x8001, 0xff00, 0xff01, 0xfffe, 0xffff};
    std::vector<int> const signedWords{-32768, -32767, -513, -257, -256, -255, -129, -128, -17, -9, -3, -2, -1, 0, 1, 2, 3, 127, 128, 255, 256, 257, 513, 32767};
    auto add = [&](std::string name, unsigned bits, bool sign, ConstantOperation op,
                   std::vector<int> values, std::vector<int> constants, bool chain = false) {
      tests.push_back(constantOperationCase(std::move(name), bits, sign, op,
                                           std::move(values), std::move(constants), chain));
    };
    using enum ConstantOperation;
    add("Constant half u8 exhaustive", 8, false, Divide, u8, {2});
    add("Constant half s8 exhaustive", 8, true, Divide, s8, {2, -2});
    add("Constant half u16 byte carries", 16, false, Divide, words, {2});
    add("Constant half s16 signs and minimum", 16, true, Divide, signedWords, {2, -2});
    add("Repeated constant half u16 scratch cleanup", 16, false, Divide, words, {2}, true);
    add("Constant quarter u8", 8, false, Divide, bytes, {4});
    add("Constant quarter u16", 16, false, Divide, words, {4});
    add("Constant power division u8", 8, false, Divide, bytes, {8, 16, 32, 64, 128});
    add("Constant power division s8", 8, true, Divide, signedBytes, {8, -8, 16, -16, 64, -64, -128});
    add("Constant power division u16", 16, false, Divide, words, {8, 16, 128, 256, 512, 32768});
    add("Constant power division s16", 16, true, Divide, signedWords, {8, -8, 16, -16, 256, -256, -32768});
    add("Constant multiply special cases u8", 8, false, Multiply, bytes, {0, 1, 2, 4, 8, 16, 64, 128, 3, 5, 7, 9, 15, 17, 127, 129, 255, 6});
    add("Constant multiply special cases s8", 8, true, Multiply, signedBytes, {0, 1, -1, 2, -2, 4, -4, 8, -8, 3, -3, 7, -7, 15, -15, -128, 6});
    add("Constant multiply special cases u16", 16, false, Multiply, words, {0, 1, 2, 4, 8, 128, 256, 32768, 3, 5, 7, 15, 17, 255, 257, 32767, 65535, 6});
    add("Constant multiply special cases s16", 16, true, Multiply, signedWords, {0, 1, -1, 2, -2, 4, -4, 8, -8, 3, -3, 15, -15, 17, -17, 256, -256, -32768, 6});
    add("Constant div zero and identity u8", 8, false, Divide, bytes, {0, 1});
    add("Constant div zero and identity u16", 16, false, Divide, words, {0, 1});
    add("Constant div zero and identities s8", 8, true, Divide, signedBytes, {0, 1, -1});
    add("Constant div zero and identities s16", 16, true, Divide, signedWords, {0, 1, -1});
    add("Constant remainder u8", 8, false, Modulo, bytes, {0, 1, 2, 4, 8, 128});
    add("Constant remainder s8", 8, true, Modulo, signedBytes, {0, 1, -1, 2, -2, 4, -4, 8, -8});
    add("Constant remainder u16", 16, false, Modulo, words, {0, 1, 2, 4, 8, 256});
    add("Constant remainder s16", 16, true, Modulo, signedWords, {0, 1, -1, 2, -2, 4, -4, 8, -8, 256, -256});
    add("Constant quarter s8", 8, true, Divide, signedBytes, {4, -4});
    add("Constant quarter s16", 16, true, Divide, signedWords, {4, -4});
  }
}
