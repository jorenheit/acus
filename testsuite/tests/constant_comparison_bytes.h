// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace bftest {
  enum class ByteComparison { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };

  inline TestCase constantByteComparisonCase(std::string name, bool isSigned,
                                             ByteComparison operation, std::vector<int> constants) {
    int const min = isSigned ? -32768 : 0;
    int const max = isSigned ? 32767 : 65535;
    std::vector<std::pair<int, std::vector<int>>> cases;
    for (int constant : constants) {
      std::vector<int> values{min, min + 1, max - 1, max, 0, 1, 127, 128, 255, 256, 257};
      if (isSigned) values.insert(values.end(), {-32767, -513, -257, -256, -255, -1});
      for (int delta : {-257, -256, -255, -1, 0, 1, 255, 256, 257}) {
        int const value = constant + delta;
        if (value >= min && value <= max) values.push_back(value);
      }
      // Same high byte with different low bytes, including both sides of 127/128.
      unsigned const encoded = static_cast<unsigned>(constant) & 0xffff;
      for (unsigned low : {0u, 1u, 127u, 128u, 254u, 255u}) {
        int value = static_cast<int>((encoded & 0xff00) | low);
        if (isSigned && value >= 32768) value -= 65536;
        values.push_back(value);
      }
      std::sort(values.begin(), values.end());
      values.erase(std::unique(values.begin(), values.end()), values.end());
      cases.emplace_back(constant, std::move(values));
    }

    auto appendWord = [](std::string &out, int value) {
      unsigned const encoded = static_cast<unsigned>(value);
      out += static_cast<char>(encoded & 0xff);
      out += static_cast<char>((encoded >> 8) & 0xff);
    };
    auto compare = [=](int value, int constant) {
      switch (operation) {
        case ByteComparison::Equal:        return value == constant;
        case ByteComparison::NotEqual:     return value != constant;
        case ByteComparison::Less:         return value < constant;
        case ByteComparison::LessEqual:    return value <= constant;
        case ByteComparison::Greater:      return value > constant;
        case ByteComparison::GreaterEqual: return value >= constant;
      }
      std::unreachable();
    };
    std::string input, expected;
    for (auto const &[constant, values] : cases) {
      for (int value : values) {
        for (bool expression : {false, true}) {
          input += static_cast<char>(static_cast<unsigned>(value) & 0xff);
          int const result = compare(value, constant);
          appendWord(expected, result);           // Exactly 0/1, high byte zero.
          if (expression) appendWord(expected, value); // Source remains unchanged.
          expected += 'G';                        // Adjacent local remains intact.
          appendWord(expected, result + 1);       // Immediate arithmetic scratch reuse.
          appendWord(expected, 0);                // Another comparison on the same slot.
          expected += 'G';
        }
      }
    }
    return TestCase{
      .name = std::move(name),
      .buildProgram = [=] {
        using namespace acus::api;
        Assembler c;
        c.program("byte_comparison_regression", "main").begin();
        c.function("main").begin();
        c.declareLocal("x", isSigned ? ts::s16() : ts::u16());
        c.declareLocal("result", ts::u16());
        c.declareLocal("guard", ts::u8());
        c.assign("guard", literal::u8('G'));
        auto literalValue = [=](int value) -> literal::Literal {
          return isSigned ? literal::s16(value) : literal::u16(value);
        };
        for (auto const &[constant, values] : cases) {
          for (int value : values) {
            for (bool expression : {false, true}) {
              c.scope().begin();
              // read() updates only the low byte. Initialize the whole word first
              // and then feed its original low byte at runtime to prevent folding.
              c.assign("x", literalValue(value));
              c.read("x");
              auto rhs = literalValue(constant);
              if (expression) {
                switch (operation) {
                  case ByteComparison::Equal:        c.assign("result", c.eq("x", rhs)); break;
                  case ByteComparison::NotEqual:     c.assign("result", c.neq("x", rhs)); break;
                  case ByteComparison::Less:         c.assign("result", c.lt("x", rhs)); break;
                  case ByteComparison::LessEqual:    c.assign("result", c.le("x", rhs)); break;
                  case ByteComparison::Greater:      c.assign("result", c.gt("x", rhs)); break;
                  case ByteComparison::GreaterEqual: c.assign("result", c.ge("x", rhs)); break;
                }
              } else {
                switch (operation) {
                  case ByteComparison::Equal:        c.eqAssign("x", rhs); break;
                  case ByteComparison::NotEqual:     c.neqAssign("x", rhs); break;
                  case ByteComparison::Less:         c.ltAssign("x", rhs); break;
                  case ByteComparison::LessEqual:    c.leAssign("x", rhs); break;
                  case ByteComparison::Greater:      c.gtAssign("x", rhs); break;
                  case ByteComparison::GreaterEqual: c.geAssign("x", rhs); break;
                }
              }
              char const *target = expression ? "result" : "x";
              c.write(target);
              if (expression) c.write("x");
              c.write("guard");
              auto one = expression ? literal::u16(1) : literalValue(1);
              auto zero = expression ? literal::u16(0) : literalValue(0);
              c.addAssign(target, one);
              c.write(target);
              c.eqAssign(target, zero); // result + 1 cannot be zero.
              c.write(target);
              c.write("guard");
              c.endScope();
            }
          }
        }
        c.returnFromFunction();
        c.endFunction();
        c.endProgram();
        return c.brainfuck("byte_comparison_regression");
      },
      .expectedOutput = std::move(expected),
      .input = std::move(input),
      .requireHalt = true,
      .maxSteps = 500'000'000ULL * constants.size(),
    };
  }

  inline void addConstantByteComparisonTests(std::vector<TestCase> &tests) {
    using enum ByteComparison;
    for (bool sign : {false, true}) {
      for (auto op : {Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual}) {
        std::vector<int> constants{0, 1, 2, 127, 128, 254, 255};
        auto lowZeroConstants = sign ? std::vector<int>{-32768, -32512, -512, -256, 256, 512, 32512}
                                     : std::vector<int>{256, 512, 32512, 32768, 33024, 65280};
        constants.insert(constants.end(), lowZeroConstants.begin(), lowZeroConstants.end());
        if (op == LessEqual || op == Greater) {
          auto lowMaxConstants = sign ? std::vector<int>{-32513, -257, -1, 511, 32767}
                                      : std::vector<int>{511, 32767, 33023, 65279, 65535};
          constants.insert(constants.end(), lowMaxConstants.begin(), lowMaxConstants.end());
        }
        char const *opName = op == Equal ? "eq" : op == NotEqual ? "ne" : op == Less ? "lt"
                           : op == LessEqual ? "le" : op == Greater ? "gt" : "ge";
        std::string name = std::string("Constant byte comparisons ") + (sign ? "s16 " : "u16 ") + opName;
        tests.push_back(constantByteComparisonCase(std::move(name), sign, op, std::move(constants)));
      }
    }
  }
}
