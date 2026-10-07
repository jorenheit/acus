// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

namespace bftest {
inline void addStraightLineTests(std::vector<TestCase> &tests) {
  tests.push_back(TestCase{
    .name = "Straight line input, arithmetic and inline decimal printing",
    .buildProgram = [] {
      using namespace acus::api;
      Assembler c; c.program("straight", acus::Program::Mode::StraightLine).begin();
      c.declareLocal("x", ts::u16()); c.assign("x", literal::u16(0)); c.read("x");
      c.mulAssign("x", literal::u16(3)); c.print("x");
      c.print(literal::string("/")); c.divAssign("x", literal::u16(2)); c.print("x");
      c.declareLocal("s", ts::s16()); c.assign("s", literal::s16(-32768));
      c.print(literal::string("/")); c.print("s");
      c.endProgram(); return c.brainfuck("straight");
    }, .expectedOutput = "21/10/-32768", .input = std::string(1, char(7)),
    .requireHalt = true, .maxSteps = 200'000'000});
  tests.push_back(TestCase{
    .name = "Straight line pointers, dynamic array cache and assembler reuse",
    .buildProgram = [] {
      using namespace acus::api;
      Assembler c; c.program("first", "main").begin(); c.function("main").begin();
      c.print(literal::string("A")); c.returnFromFunction(); c.endFunction(); c.endProgram();
      c.program("second", acus::Program::Mode::StraightLine).begin();
      c.declareLocal("x", ts::u8()); c.declareLocal("p", ts::pointer(ts::u8()));
      c.assign("x", literal::u8('X')); c.assign("p", c.addressOf("x"));
      c.write(c.dereferencePointer("p")); c.assign(c.dereferencePointer("p"), literal::u8('Y')); c.write("x");
      c.declareLocal("arr", ts::array(ts::u8(), 3)); c.declareLocal("i", ts::u8());
      c.assign("i", literal::u8(1)); c.assign(c.arrayElement("arr", "i"), literal::u8('Z'));
      c.write(c.arrayElement("arr", "i"));
      c.scope().begin(); c.declareLocal("local", ts::u8()); c.assign("local", literal::u8('S')); c.write("local"); c.endScope();
      c.endProgram();
      c.program("third", "main").begin(); c.function("main").begin();
      c.print(literal::string("B")); c.returnFromFunction(); c.endFunction(); c.endProgram();
      if (c.brainfuck("first").empty() || c.brainfuck("third").empty())
        throw std::runtime_error("dispatched program lost during reuse");
      return c.brainfuck("second");
    }, .expectedOutput = "XYZS", .requireHalt = true, .maxSteps = 200'000'000});
  tests.push_back(TestCase{
    .name = "Straight line rejects dispatch APIs and unclosed scopes",
    .buildProgram = [] {
      using namespace acus::api;
      auto reject = [](auto action, acus::error::ErrorCode code) {
        Assembler c; c.program("reject", acus::Program::Mode::StraightLine).begin();
        try { action(c); } catch (acus::error::Error const &e) {
          if (e.code() == code) return;
          throw;
        }
        throw std::runtime_error("straight-line API unexpectedly accepted");
      };
      auto const code = acus::error::ErrorCode::NotAvailableInStraightLineMode;
      reject([](Assembler &c){ c.function("f"); }, code);
      reject([](Assembler &c){ c.callFunction("f"); }, code);
      reject([](Assembler &c){ c.callFunctionPointer(literal::function_pointer(ts::void_function(), "f")); }, code);
      reject([](Assembler &c){ c.label("label"); }, code);
      reject([](Assembler &c){ c.jump("label"); }, code);
      reject([](Assembler &c){ c.jumpIf(literal::u8(1), "yes", "no"); }, code);
      reject([](Assembler &c){ c.returnFromFunction(); }, code);
      reject([](Assembler &c){ c.returnFromFunction(literal::u8(1)); }, code);
      reject([](Assembler &c){ c.abortProgram(); }, code);
      reject([](Assembler &c){ c.unreachable(); }, code);
      reject([](Assembler &c){ c.endFunction(); }, code);
      reject([](Assembler &c){ c.setBlockPriority(1); }, code);
      reject([](Assembler &c){ c.setFunctionPriority(1); }, code);
      reject([](Assembler &c){ c.declareLocal("fp", ts::function_pointer(ts::void_function())); c.assign("fp", literal::function_pointer(ts::void_function(), "f")); }, code);
      reject([](Assembler &c){ c.scope().begin(); c.endProgram(); }, acus::error::ErrorCode::ExpectedNoScope);
      Assembler c; c.program("empty", acus::Program::Mode::StraightLine).begin(); c.endProgram();
      if (c.brainfuck("empty").find('[') != std::string::npos)
        throw std::runtime_error("empty straight-line program contains dispatch loop");
      c.program("done", acus::Program::Mode::StraightLine).begin(); c.print(literal::string("OK")); c.endProgram();
      return c.brainfuck("done");
    }, .expectedOutput = "OK", .requireHalt = true, .maxSteps = 200'000'000});
}
}
