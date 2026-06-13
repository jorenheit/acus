// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "acus.h"
using namespace acus::api;

namespace ct {

using acus::error::ErrorCode;

struct TestCase {
  std::string name;
  ErrorCode expectedCode;
  std::function<void()> body;
};

struct TestOutcome {
  std::string name;
  bool passed = false;
  ErrorCode expectedCode;
  std::optional<ErrorCode> actualCode;
  std::string message;
};

std::string codeName(ErrorCode code) {
  switch (code) {
  case ErrorCode::UnexpectedApiCall: return "UnexpectedApiCall";
  case ErrorCode::EmptyProgram: return "EmptyProgram";
  case ErrorCode::DuplicateFunctionParameters: return "DuplicateFunctionParameters";
  case ErrorCode::DuplicateGlobalReferences: return "DuplicateGlobalReferences";
  case ErrorCode::FunctionDoesNotExist: return "FunctionDoesNotExist";
  case ErrorCode::LabelDoesNotExist: return "LabelDoesNotExist";
  case ErrorCode::TakingAddressOfTemporary: return "TakingAddressOfTemporary";
  case ErrorCode::ReadOnlyExpression: return "ReadOnlyExpression";
  case ErrorCode::IncompatibleOperands: return "IncompatibleOperands";
  case ErrorCode::NotPrintable: return "NotPrintable";
  case ErrorCode::ExpectedOutsideProgramBlock: return "ExpectedOutsideProgramBlock";
  case ErrorCode::ExpectedInsideProgramBlock: return "ExpectedInsideProgramBlock";
  case ErrorCode::ExpectedOutsideFunctionBlock: return "ExpectedOutsideFunctionBlock";
  case ErrorCode::ExpectedInsideFunctionBlock: return "ExpectedInsideFunctionBlock";
  case ErrorCode::ExpectedNoScope: return "ExpectedNoScope";
  case ErrorCode::GlobalDeclarationNotAllowed: return "GlobalDeclarationNotAllowed";
  case ErrorCode::GlobalNameUnavailable: return "GlobalNameUnavailable";
  case ErrorCode::ExpectedGlobal: return "ExpectedGlobal";
  case ErrorCode::NameNotInScope: return "NameNotInScope";
  case ErrorCode::NameAlreadyInScope: return "NameAlreadyInScope";
  case ErrorCode::NameAlreadyInCurrentScope: return "NameAlreadyInCurrentScope";
  case ErrorCode::ExpectedInteger: return "ExpectedInteger";
  case ErrorCode::ExpectedSignedInteger: return "ExpectedSignedInteger";
  case ErrorCode::ExpectedUnsignedInteger: return "ExpectedUnsignedInteger";
  case ErrorCode::ExpectedArray: return "ExpectedArray";
  case ErrorCode::ExpectedString: return "ExpectedString";
  case ErrorCode::ExpectedArrayOrString: return "ExpectedArrayOrString";
  case ErrorCode::ExpectedStruct: return "ExpectedStruct";
  case ErrorCode::ExpectedPointer: return "ExpectedPointer";
  case ErrorCode::ExpectedFunctionType: return "ExpectedFunctionType";
  case ErrorCode::ExpectedFunctionPointer: return "ExpectedFunctionPointer";
  case ErrorCode::FieldIndexOutOfBounds: return "FieldIndexOutOfBounds";
  case ErrorCode::FieldCountMismatch: return "FieldCountMismatch";
  case ErrorCode::MissingField: return "MissingField";
  case ErrorCode::MultipleInitializationsOfSameField: return "MultipleInitializationsOfSameField";
  case ErrorCode::FieldNotFound: return "FieldNotFound";
  case ErrorCode::IndexOutOfBounds: return "IndexOutOfBounds";
  case ErrorCode::NegativeIndex: return "NegativeIndex";
  case ErrorCode::TooManyElementsInArrayInitialization: return "TooManyElementsInArrayInitialization";
  case ErrorCode::TooFewElementsInArrayInitialization: return "TooFewElementsInArrayInitialization";
  case ErrorCode::AssignmentTypeMismatch: return "AssignmentTypeMismatch";
  case ErrorCode::UnexpectedType: return "UnexpectedType";
  case ErrorCode::FunctionParameterCountMismatch: return "FunctionParameterCountMismatch";
  case ErrorCode::InvalidFunctionPointerCall: return "InvalidFunctionPointerCall";
  case ErrorCode::BuilderNotFinalized: return "BuilderNotFinalized";
  case ErrorCode::BuilderUsedAfterFinalize: return "BuilderUsedAfterFinalize";
  case ErrorCode::ReturnTypeSpecifiedMultipleTimes: return "ReturnTypeSpecifiedMultipleTimes";
  case ErrorCode::EntryFunctionNotDefined: return "EntryFunctionNotDefined";
  case ErrorCode::WrongEntryFunctionType: return "WrongEntryFunctionType";
  case ErrorCode::ExecutionPathWithoutReturn: return "ExecutionPathWithoutReturn";
  case ErrorCode::UnreachableCodeSection: return "UnreachableCodeSection";
  case ErrorCode::InvalidReadType: return "InvalidReadType";
  }
  return "<unknown>";
}

void resetApiSequencing() {
  acus::api::impl::clearExpected();
}

TestOutcome run(TestCase const &test) {
  resetApiSequencing();
  TestOutcome out{.name = test.name, .expectedCode = test.expectedCode};
  try {
    test.body();
    out.passed = false;
    out.message = "expected an acus::error::Error, but no exception was thrown";
  }
  catch (acus::error::Error const &e) {
    out.actualCode = e.code();
    out.passed = (e.code() == test.expectedCode);
    out.message = e.what();
  }
  catch (std::exception const &e) {
    out.passed = false;
    out.message = std::string("expected acus::error::Error, got std::exception: ") + e.what();
  }
  catch (...) {
    out.passed = false;
    out.message = "expected acus::error::Error, got unknown exception";
  }
  resetApiSequencing();
  return out;
}

void beginProgram(Assembler &c) {
  c.program("test", "main").begin();
}

void beginMain(Assembler &c) {
  c.function("main").begin();
}

void beginBasicMain(Assembler &c) {
  beginProgram(c);
  beginMain(c);
  c.label("start");
}

void finishProgram(Assembler &c) {
  c.endFunction();
  c.endProgram();
}

TypeHandle pairStruct() {
  auto existing = ts::struct_t("Pair");
  if (existing) return existing;
  return ts::defineStruct("Pair").field("a", ts::u8()).field("b", ts::u8()).done();
}

TypeHandle singleStruct() {
  auto existing = ts::struct_t("Single");
  if (existing) return existing;
  return ts::defineStruct("Single").field("a", ts::u8()).done();
}

std::vector<TestCase> buildTests() {
  using enum ErrorCode;
  std::vector<TestCase> tests;

  auto add = [&](std::string name, ErrorCode code, std::function<void()> body) {
    tests.push_back(TestCase{std::move(name), code, std::move(body)});
  };

  add("endProgram before program begin", ExpectedInsideProgramBlock, [] {
    Assembler c;
    c.endProgram();
  });

  add("program begin while program already open", ExpectedOutsideProgramBlock, [] {
    Assembler c;
    beginProgram(c);
    c.program("other", "main").begin();
  });

  add("function outside program", ExpectedInsideProgramBlock, [] {
    Assembler c;
    c.function("main").begin();
  });

  add("endProgram while inside function", ExpectedOutsideFunctionBlock, [] {
    Assembler c;
    beginProgram(c);
    beginMain(c);
    c.endProgram();
  });

  add("label outside function", ExpectedInsideFunctionBlock, [] {
    Assembler c;
    beginProgram(c);
    c.label("start");
  });

  add("endFunction while scope is open", ExpectedNoScope, [] {
    Assembler c;
    beginBasicMain(c);
    c.scope().begin();
    c.endFunction();
  });

  add("empty program", EmptyProgram, [] {
    Assembler c;
    beginProgram(c);
    c.endProgram();
  });

  add("global declaration after functions started", GlobalDeclarationNotAllowed, [] {
    Assembler c;
    beginProgram(c);
    beginMain(c);
    c.declareGlobal("g", ts::u8());    
    c.returnFromFunction();
    c.endFunction();
  });

  add("duplicate global name", GlobalNameUnavailable, [] {
    Assembler c;
    beginProgram(c);
    c.declareGlobal("g", ts::u8());
    c.declareGlobal("g", ts::u8());
  });

  add("unknown variable", NameNotInScope, [] {
    Assembler c;
    beginBasicMain(c);
    c.write("missing");
  });

  add("duplicate local in current scope", NameAlreadyInCurrentScope, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.declareLocal("x", ts::u8());
  });

  add("function with duplicate parameter names", DuplicateFunctionParameters, [] {
    Assembler c;
    beginProgram(c);
    c.function("main").param("x", ts::u8()).param("x", ts::u8()).begin();
  });

  add("emit after jump requires label", UnexpectedApiCall, [] {
    Assembler c;
    beginBasicMain(c);
    c.jump("done");
    c.write(literal::u8('X'));
  });

  add("jump target must exist", LabelDoesNotExist, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.jumpIf("x", "missing", "after_jump");
    c.label("after_jump");
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("jumpIf target must exist", LabelDoesNotExist, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.jumpIf("x", "yes", "missing");
    c.label("yes");
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("jumpIf requires integer condition", ExpectedInteger, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("arr", ts::array(ts::u8(), 2));
    c.jumpIf("arr", "yes", "no");
  });

  add("field access requires struct", ExpectedStruct, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.structField("x", "a");
  });

  add("field index out of bounds", FieldIndexOutOfBounds, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("s", singleStruct());
    c.structField("s", 1);
  });

  add("field name not found", FieldNotFound, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("s", singleStruct());
    c.structField("s", "missing");
  });

  add("array element requires array or string", ExpectedArrayOrString, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.arrayElement("x", 0);
  });

  add("static array index out of bounds", IndexOutOfBounds, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("arr", ts::array(ts::u8(), 2));
    c.arrayElement("arr", 2);
  });

  add("dynamic array index must be integer", ExpectedInteger, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("arr", ts::array(ts::u8(), 2));
    c.declareLocal("idx", ts::array(ts::u8(), 1));
    c.arrayElement("arr", "idx");
  });

  add("negative dynamic array index", NegativeIndex, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("arr", ts::array(ts::u8(), 2));
    c.arrayElement("arr", literal::s8(-1));
  });

  add("dereference requires pointer", ExpectedPointer, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.dereferencePointer("x");
  });

  add("callFunctionPointer requires function pointer", ExpectedFunctionPointer, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.callFunctionPointer("x").done();
  });

  add("function pointer call arity", InvalidFunctionPointerCall, [] {
    Assembler c;
    beginBasicMain(c);
    auto fType = ts::function().param(ts::u8()).done();
    c.callFunctionPointer(literal::function_pointer(fType, "somewhere")).done();
  });

  add("direct function call to missing function", FunctionDoesNotExist, [] {
    Assembler c;
    beginBasicMain(c);
    c.callFunction("missing").done();
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("direct function call arity", InvalidFunctionPointerCall, [] {
    Assembler c;
    beginProgram(c);
    c.function("callee").param("x", ts::u8()).begin();
    c.label("callee_start");
    c.returnFromFunction();
    c.endFunction();
    beginMain(c);
    c.label("main_start");
    c.callFunction("callee").done();
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("assignment type mismatch", AssignmentTypeMismatch, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    auto arr = literal::array(ts::array(ts::u8(), 1)).push(literal::u8('A')).done();
    c.assign("x", arr);
  });

  add("unexpected literal element type", UnexpectedType, [] {
    literal::array(ts::array(ts::u8(), 1)).push(literal::u16(123)).done();
  });

  add("incompatible binary operands", IncompatibleOperands, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    c.declareLocal("arr", ts::array(ts::u8(), 1));
    c.add("x", "arr");
  });

  add("taking address of temporary", TakingAddressOfTemporary, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("x", ts::u8());
    auto tmp = c.add("x", literal::u8(1));
    c.addressOf(tmp);
  });

  add("print unsupported type", NotPrintable, [] {
    Assembler c;
    beginBasicMain(c);
    c.declareLocal("arr", ts::array(ts::u8(), 2));
    c.print("arr");
  });

  add("literal builder requires array type", ExpectedArray, [] {
    literal::array(ts::u8());
  });

  add("struct literal requires struct type", ExpectedStruct, [] {
    literal::struct_t(ts::u8()).done();
  });

  add("struct literal field count mismatch", FieldCountMismatch, [] {
    literal::struct_t(pairStruct()).init("a", literal::u8('A')).done();
  });

  add("struct literal missing field", MissingField, [] {
    literal::struct_t(pairStruct())
      .init("a", literal::u8('A'))
      .init("c", literal::u8('C'))
      .done();
  });

  add("struct literal duplicate field initialization", MultipleInitializationsOfSameField, [] {
    literal::struct_t(singleStruct())
      .init("a", literal::u8('A'))
      .init("a", literal::u8('B'));
  });

  add("too many array literal elements", TooManyElementsInArrayInitialization, [] {
    literal::array(ts::array(ts::u8(), 1))
      .push(literal::u8('A'))
      .push(literal::u8('B'));
  });

  add("too few array literal elements", TooFewElementsInArrayInitialization, [] {
    literal::array(ts::array(ts::u8(), 2))
      .push(literal::u8('A'))
      .done();
  });

  add("return type specified twice on type builder", ReturnTypeSpecifiedMultipleTimes, [] {
    auto builder = ts::function();
    builder.ret(ts::u8());
    builder.ret(ts::u16());
  });

  add("return type specified twice on function builder", ReturnTypeSpecifiedMultipleTimes, [] {
    Assembler c;
    beginProgram(c);
    auto builder = c.function("main");
    builder.ret(ts::u8());
    builder.ret(ts::u16());
  });

  add("missing entry function", EntryFunctionNotDefined, [] {
    Assembler c;
    c.program("test", "missing").begin();
    beginMain(c);
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("entry function wrong type", WrongEntryFunctionType, [] {
    Assembler c;
    c.program("test", "main").begin();
    c.function("main").param("x", ts::u8()).begin();
    c.returnFromFunction();
    c.endFunction();
    c.endProgram();
  });

  add("execution path without return", ExecutionPathWithoutReturn, [] {
    Assembler c;
    c.program("test", "main").begin();
    c.function("main").begin();
    c.declareLocal("x", ts::u8());
    c.jumpIf("x", "true", "false");
    c.label("true");
    c.returnFromFunction();
    c.label("false");
    c.endFunction();
    c.endProgram();
  });

  add("unreachable block", UnreachableCodeSection, [] { 
    Assembler c;
    c.program("test", "main").begin();
    c.function("main").begin();
    c.declareLocal("x", ts::u8());
    c.returnFromFunction();
    c.label("unreachable");
    c.assign("x", literal::u8(0));
    c.endFunction();
    c.endProgram();
  });

  return tests;
}

} // namespace ct

int main() {
  auto tests = ct::buildTests();
  std::size_t passed = 0;

  for (auto const &test : tests) {
    auto outcome = ct::run(test);
    std::cout << (outcome.passed ? "[PASS] " : "[FAIL] ") << outcome.name;
    if (!outcome.passed) {
      std::cout << "\n"
                << "  expected code: " << ct::codeName(outcome.expectedCode) << "\n"
                << "  actual code:   "
                << (outcome.actualCode ? ct::codeName(*outcome.actualCode) : std::string("<none>")) << "\n"
                << "  message:       " << std::quoted(outcome.message);
    }
    std::cout << "\n";
    if (outcome.passed) ++passed;
  }

  std::cout << "\nSummary: " << passed << "/" << tests.size() << " passed\n";
  return passed == tests.size() ? EXIT_SUCCESS : EXIT_FAILURE;
}
