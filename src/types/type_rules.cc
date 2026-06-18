// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/types/type_rules.h"
#include "acus/types/typesystem.h"

using namespace acus;

namespace {

  using types::TypeHandle;
  using types::rules::OpResult;
  
  OpResult accept(TypeHandle resultType) { return {resultType, resultType, ""}; }
  OpResult accept(TypeHandle resultType, TypeHandle workType) { return {resultType, workType, ""}; }
  OpResult reject(std::string msg) { return {types::null, types::null, std::move(msg)}; }

  bool bothIntegers(TypeHandle a, TypeHandle b) {
    return types::isInteger(a) && types::isInteger(b);
  }

  bool pointerCanDecayInto(TypeHandle target, TypeHandle t) {
    // TODO: this shouldnt be an assert
    assert(types::isPointer(t) && types::isPointer(target));

    if (t == target) return true;
    TypeHandle pointeeType = types::cast<types::PointerType>(t)->pointeeType();

    if (types::isArrayLike(pointeeType)) {
      auto arrayType = types::cast<types::ArrayLike>(pointeeType);
      return pointerCanDecayInto(target, ts::pointer(arrayType->elementType()));
    }
    if (types::isStruct(pointeeType)) {
      auto structType = types::cast<types::StructType>(pointeeType);
      return pointerCanDecayInto(target, ts::pointer(structType->fieldType(0)));
    }
    return false;
  }
  

  bool isLogical(BinOp op) {
    switch(op) {
    case BinOp::And:
    case BinOp::Nand:
    case BinOp::Or:
    case BinOp::Nor:
    case BinOp::Xor:
    case BinOp::Xnor: return true;
    default: return false;
    }
    std::unreachable();
  }

  bool isComparison(BinOp op) {
    switch(op) {
    case BinOp::Eq:
    case BinOp::Neq:
    case BinOp::Lt:
    case BinOp::Le:
    case BinOp::Gt:
    case BinOp::Ge: return true;
    default: return false;
    }
    std::unreachable();
  }

  TypeHandle promotedInteger(TypeHandle a, TypeHandle b) {
    if (!bothIntegers(a, b)) return types::null;
    
    auto i1 = cast<types::IntegerType>(a);
    auto i2 = cast<types::IntegerType>(b);
    
    if (i1 == i2) return i1;
    if (i1->signedness() == i2->signedness()) {
      // Same signedness, different width -> return 16-bit variant
      return i1->isSigned() ? ts::s16() : ts::u16();
    }
    if (i1->bits() == i2->bits()) {
      // Same width, different signedness
      return types::null; // TODO: allow implicit conversion to signed
    }
    // different width, different signedness
    // TODO: allow implicit conversion to s16
    return types::null;
  }


  OpResult logicResult(BinOp op, TypeHandle lhs, TypeHandle rhs) {
    assert(isLogical(op));
    auto promoted = promotedInteger(lhs, rhs);
    if (promoted) return accept(ts::u8(), promoted);
    
    return reject("operator '" + binOpStr(op) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected two integers.");
  }

  OpResult compareResult(BinOp op, TypeHandle lhs, TypeHandle rhs) {
    assert(isComparison(op));
    auto promoted = promotedInteger(lhs, rhs);
    if (promoted) return accept(ts::u8(), promoted);
    return reject("operator '" + binOpStr(op) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected two integers.");
  }
  
  OpResult addResult(TypeHandle lhs, TypeHandle rhs) {
    // Try to get a promoted integer type. If that works, the
    // types are compatible integers.
    auto result = promotedInteger(lhs, rhs);
    if (result) return accept(result);

    if (bothIntegers(lhs, rhs)) {
      // Non-compatible integers
      return reject("operator '" + binOpStr(BinOp::Add) + "' is not defined "
		    "for '" + lhs->str() + "' and '" + rhs->str() + "'.");
    }

    // Integer/Pointer
    if (types::isPointer(lhs) && types::isInteger(rhs)) return accept(lhs);
    if (types::isInteger(lhs) && types::isPointer(rhs)) return accept(rhs);

    // All other types
    return reject("operator '" + binOpStr(BinOp::Add) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected integer+integer or pointer+integer.");
  }

  OpResult subResult(TypeHandle lhs, TypeHandle rhs) {
    // Try to get a promoted integer type. If that works, the
    // types are compatible integers.
    auto result = promotedInteger(lhs, rhs);
    if (result) return accept(result);

    if (bothIntegers(lhs, rhs)) {
      // Non-compatible integers
      return reject("operator '" + binOpStr(BinOp::Sub) + "' is not defined "
		    "for '" + lhs->str() + "' and '" + rhs->str() + "'.");
    }

    // Integer/Pointer (pointer only on the left)
    if (types::isPointer(lhs) && types::isInteger(rhs)) return accept(lhs);

    // All other types
    return reject("operator '" + binOpStr(BinOp::Add) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected integer+integer or pointer+integer.");
  }

  OpResult mulResult(TypeHandle lhs, TypeHandle rhs) {
    auto result = promotedInteger(lhs, rhs);
    if (result) return accept(result);
    return reject("operator '" + binOpStr(BinOp::Mul) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected two integers.");
  }

  OpResult divResult(TypeHandle lhs, TypeHandle rhs) {
    auto result = promotedInteger(lhs, rhs);
    if (result) return accept(result);
    return reject("operator '" + binOpStr(BinOp::Div) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected two integers.");
  }

  OpResult modResult(TypeHandle lhs, TypeHandle rhs) {
    auto result = promotedInteger(lhs, rhs);
    if (result) return accept(result);
    return reject("operator '" + binOpStr(BinOp::Mod) + "' is not defined for '" +
		  lhs->str() + "' and '" + rhs->str() +
		  "'. Expected two integers.");
  }
 

  OpResult integerAssignResult(TypeHandle dest, TypeHandle src) {
    assert(types::isInteger(dest));

    if (dest == src) return accept(dest);
    if (not isInteger(src)) {
      return reject("cannot assign non-integer type '" + src->str() +
		    "' to integer type '" + dest->str() + "'.");
    }

    auto destInt = cast<types::IntegerType>(dest);
    auto srcInt = cast<types::IntegerType>(src);

    if (destInt->signedness() != srcInt->signedness()) {
      return reject("cannot assign integer type '" + src->str() +
		    "' to integer type '" + dest->str() +
		    "': implicit conversion between signed and unsigned integers "
		    "is not allowed.");
    }
    if (destInt->bits() < srcInt->bits()) {
      return reject("cannot assign integer type '" + src->str() +
		    "' to smaller integer type '" + dest->str() +
		    "': narrowing integer conversion is not allowed.");
    }      

    return accept(dest);
  }
  
  OpResult pointerAssignResult(TypeHandle dest, TypeHandle src) {
    assert(types::isPointer(dest));

    if (dest == src) return accept(dest);
    if (pointerCanDecayInto(dest, src)) return accept(dest);
    return reject("cannot assign value of type '" + src->str() +
		  "' to destination of type '" + dest->str() +
		  "'; pointer-types not compatible.");
  }

  
  OpResult stringAssignResult(TypeHandle dest, TypeHandle src) {
    assert(types::isString(dest));
    
    if (dest == src) return accept(dest);
    if (not types::isString(src)) {
      return reject("cannot assign value of type '" + src->str() +
		    "' to destination of type '" + dest->str() +
		    "': expected a string.");
    }

    if (src->size() > dest->size()) {
      return reject("cannot assign string of type '" + src->str() +
		    "' to destination of type '" + dest->str() +
		    "': source capacity exceeds destination capacity.");
    }

    return accept(dest);

  }

  OpResult rawAssignResult(TypeHandle dest, TypeHandle src) {
    assert(types::isRaw(dest));
    
    if (src->size() <= dest->size()) return accept(dest);
    return reject("cannot assign value of type '" + src->str() +
		  "' to destination of type '" + dest->str() +
		  "': source size (" + std::to_string(src->size()) +
		  ") exceeds destination size (" + std::to_string(dest->size()) + ").");
  }
  
} // namespace



OpResult types::rules::binOpResult(BinOp op, TypeHandle lhs, TypeHandle rhs) {

  if (isLogical(op)) return logicResult(op, lhs, rhs);
  if (isComparison(op)) return compareResult(op, lhs, rhs);

  switch (op) {
  case BinOp::Add: return addResult(lhs, rhs);
  case BinOp::Sub: return subResult(lhs, rhs);
  case BinOp::Mul: return mulResult(lhs, rhs);
  case BinOp::Div: return divResult(lhs, rhs);
  case BinOp::Mod: return modResult(lhs, rhs);
  default: std::unreachable();
  }
  std::unreachable();
  
}

OpResult types::rules::assignResult(TypeHandle dest, TypeHandle src) {
  // Same types: always OK, except when null
  if (dest == types::null || src == types::null) 
    return reject("cannot assign because one of the operand types is null.");
  if (dest == src)  return accept(dest);

  // All cases where source and dest are of different types
  switch (dest->tag()) {
  case U8: 
  case U16:
  case S8:
  case S16:     return integerAssignResult(dest, src);
  case POINTER: return pointerAssignResult(dest, src);
  case STRING:  return stringAssignResult(dest, src);
  case RAW:     return rawAssignResult(dest, src);

    // Other cases always fail
  case ARRAY:
    return reject("cannot assign value of type '" + src->str() +
		  "' to destination of type '" + dest->str() +
		  "': arrays are only assignable when their types match exactly.");
  case STRUCT:
    return reject("cannot assign value of type '" + src->str() +
		  "' to destination of type '" + dest->str() +
		  "': structs are only assignable when their types match exactly.");
  case VOID:
    return reject("cannot assign value of type '" + src->str() +
		  "' to destination of type 'void'.");
  default: std::unreachable();
  }
}

OpResult types::rules::castResult(TypeHandle from, TypeHandle to) {
  if (!types::isInteger(from)) return reject("source type must be an integer type, but got '" + from->str() + "'.");
  if (!types::isInteger(to)) return reject("target type must be an integer type, but got '" + to->str() + "'.");
  return accept(to);
}
