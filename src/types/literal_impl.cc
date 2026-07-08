// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <bitset>

#include "acus/types/typesystem.h"
#include "acus/types/literal_impl.h"

namespace {

  int wrapUnsigned(int v, int bits) {
    assert(bits == 8 || bits == 16);
    int const mask = (1 << bits) - 1;
    return v & mask;
  }

  int wrapSigned(int v, int bits) {
    assert(bits == 8 || bits == 16);
    int const modulus = 1 << bits;
    int const half = 1 << (bits - 1);
    
    v %= modulus;
    if (v < 0) v += modulus;
    if (v >= half) return v - modulus;
    return v;
  }
}


namespace acus::literal::impl {

  // Base
  Base::Base(types::TypeHandle t):
    _type(t)
  {}
  
  types::TypeHandle Base::type() const { return _type; }      

  // Integer
  Integer::Integer(types::TypeHandle t, int v):
    Base(t),
    _semanticValue(v)
  {}

  std::string Integer::str() const {
    return std::to_string(_semanticValue);
  }

  int Integer::semanticValue() const {
    return _semanticValue;
  }

  unsigned Integer::encodedValue() const {
    auto intType = types::cast<types::IntegerType>(_type);
    unsigned const raw = static_cast<unsigned>(_semanticValue);

    if (intType->bits() == 8)  return raw & 0xffu;
    if (intType->bits() == 16) return raw & 0xffffu;

    std::unreachable();
  }  

  // u8
  u8::u8(int v, API_CTX_IGNORE):
    Integer(ts::u8(), wrapUnsigned(v, 8))
  {}

  BasePtr u8::clone() const {
    return std::make_shared<u8>(*this);
  }

  // s8
  s8::s8(int v, API_CTX_IGNORE):
    Integer(ts::s8(), wrapSigned(v, 8))
  {}

  BasePtr s8::clone() const {
    return std::make_shared<s8>(*this);
  }
  
  // i16
  u16::u16(int v, API_CTX_IGNORE):
    Integer(ts::u16(), wrapUnsigned(v, 16))
  {}

  BasePtr u16::clone() const  {
    return std::make_shared<u16>(*this);
  }      

  // s16
  s16::s16(int v, API_CTX_IGNORE):
    Integer(ts::s16(), wrapSigned(v, 16))
  {}

  BasePtr s16::clone() const  {
    return std::make_shared<s16>(*this);
  }      
  
  // ArrayLike
  ArrayLike::ArrayLike(types::TypeHandle type):
    Base(type)
  {}

  Literal ArrayLike::element(size_t idx) const {
    assert(idx < arr.size());
    return arr[idx];
  }

  // string
  string::string(std::string const &s, API_CTX):
    ArrayLike(ts::string(s.length())),
    _str(s)
  {
    for (char c: _str)  (ArrayLike::arr).emplace_back(std::make_shared<u8>(c, API_FWD));
    (ArrayLike::arr).emplace_back(std::make_shared<u8>(0, API_FWD));
  }
      
  string::string(string const &other):
    ArrayLike(other.type()),
    _str(other._str)
  {
    for (auto const &element: other.arr) {
      arr.push_back(element.clone());
    }
  }

  std::string const &string::stdstr() const {
    return _str;
  }
  
  std::string string::str() const {
    return std::string("\"") + _str + "\"";
  }
      
  BasePtr string::clone() const {
    return std::make_shared<string>(*this);
  }

  Literal string::element(size_t idx) const {
    assert(idx < _str.size() + 1);
    return arr[idx];
  }

  // structT
  structT::structT(types::TypeHandle type, std::unordered_map<std::string, Literal> const &fields, API_CTX):
    Base(type)
  {
    API_REQUIRE_IS_STRUCT(type);
      
    // Check if fields match and add to vector
    auto structType = types::cast<types::StructType>(type);
    API_REQUIRE(structType->fieldCount() == (int)fields.size(),
		error::ErrorCode::FieldCountMismatch,
		"field-count mismatch; expected ", structType->fieldCount(), ", but got ", fields.size());
      
    for (int i = 0; i != structType->fieldCount(); ++i) {
      std::string const &name = structType->fieldName(i);
      types::TypeHandle type = structType->fieldType(i);

      API_REQUIRE(fields.contains(name),
		  error::ErrorCode::MissingField,
		  "missing field '", name, "' in instantiation of struct literal.");
      API_EXPECT_TYPE(fields.at(name).type(), type);

      _fields.emplace_back(name, fields.at(name));
    }
  }

  structT::structT(structT const &other):
    Base(other.type())
  {
    for (auto const &[name, value]: other._fields) {
      _fields.emplace_back(name, value.clone());
    }
  }
      
  Literal structT::field(std::string const &name) const {
    for (auto const &[fieldName, value]: _fields) if (fieldName == name) return value;
    assert(false && "invalid field name");
    std::unreachable();
  }

  Literal structT::field(size_t idx) const {
    assert(idx < _fields.size() && "field index out of bounds");
    return _fields[idx].second;
  }      
      
  BasePtr structT::clone() const {
    return std::make_shared<structT>(*this);
  }

  std::string structT::str() const {
    std::ostringstream oss;
    oss << "{";
    for (auto const &[name, value]: _fields) {
      oss << name << ": " << value.str() << ",";
    }
    oss << "}";
    return oss.str();
  }

  // array
  array::array(types::TypeHandle elementType, std::vector<Literal> const &elements, API_CTX):
    ArrayLike(ts::array(elementType, elements.size())),
    elementType(elementType)
  {
    for (auto const &value: elements) {
      API_EXPECT_TYPE(value.type(), elementType);
    }
    ArrayLike::arr = elements;
  }
      
      
  array::array(array const &other): ArrayLike(other.type()), elementType(other.elementType) {
    ArrayLike::arr.reserve(other.arr.size());
    for (auto const& elem: other.arr) {
      ArrayLike::arr.emplace_back(elem.clone());
    }
  }

  BasePtr array::clone() const {
    return std::make_shared<array>(*this);
  }      

  std::string array::str() const {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i != arr.size(); ++i) {
      oss << arr[i].str();
      if (i < arr.size() - 1) oss << ", ";
    }
    oss << "}";
    return oss.str();
  }

  // FunctionPointer      
  FunctionPointer::FunctionPointer(types::FunctionType const *functionType, std::string const &fname, API_CTX_IGNORE):
    Base(ts::function_pointer(functionType)),
    _functionName(fname)
  {}

  std::shared_ptr<Base> FunctionPointer::clone() const {
    return std::make_shared<FunctionPointer>(*this);
  }

  std::string FunctionPointer::str() const {
    return "fptr<" + _functionName + ">";
  }

  std::string const &FunctionPointer::functionName() const {
    return _functionName;
  }
    
} // namespace literal::mpl
