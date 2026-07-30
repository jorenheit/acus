// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <unordered_map>
#include <memory>

#include "acus/types/types_fwd.h"

#define API_HEADER
#include "acus/api/api.h"


// Implementation of literal types (literal_impl.cc)
namespace acus::literal::impl {

  struct Base {
    types::TypeHandle _type;
    Base(types::TypeHandle t);

    virtual ~Base() = default;
    virtual std::shared_ptr<Base> clone() const = 0;
    virtual std::string str() const = 0;
    virtual types::TypeHandle type() const;
  };

  using BasePtr = std::shared_ptr<Base>;
  using BasePtrConst = std::shared_ptr<Base const>;
  class Literal;
    
  struct Integer: Base {
    int const _semanticValue;
    Integer(types::TypeHandle t, int v);

    virtual std::string str() const override;
    unsigned encodedValue() const;
    int semanticValue() const;
  };
    
  struct u8: Integer {
    u8(u8 const &other) = default;
    u8(int v, API_CTX_IGNORE);
    virtual BasePtr clone() const override;
  };

  struct s8: Integer {
    s8(s8 const &other) = default;
    s8(int v, API_CTX_IGNORE);
    virtual BasePtr clone() const override;
  };
  
  struct u16: Integer {
    u16(u16 const& other) = default;
    u16(int v, API_CTX_IGNORE);
    virtual BasePtr clone() const override;
  };      

  struct s16: Integer {
    s16(s16 const& other) = default;
    s16(int v, API_CTX_IGNORE);
    virtual BasePtr clone() const override;
  };      
  
  struct ArrayLike: Base {
    std::vector<Literal> arr;

    ArrayLike(types::TypeHandle type);
    Literal element(size_t idx) const;
  };
    
  struct string: ArrayLike {
    std::string const _str;

    string(std::string const &s, size_t capacity, API_CTX);      
    string(string const &other);
    virtual std::string str() const override;      
    virtual BasePtr clone() const override;
    Literal element(size_t idx) const;
    std::string const &stdstr() const;
  }; // string

  struct structT: Base {

    std::vector<std::pair<std::string, Literal>> _fields;
    
    structT(types::TypeHandle type, std::unordered_map<std::string, Literal> const &fields, API_CTX);
    structT(structT const &other);      
    Literal field(std::string const &name) const;
    Literal field(size_t idx) const;      
    virtual BasePtr clone() const override;
    virtual std::string str() const override;
  }; // structT

  struct array: ArrayLike {
    types::TypeHandle elementType;

    array(types::TypeHandle elementType, std::vector<Literal> const &elements, API_CTX);      
    array(array const &other);

    virtual BasePtr clone() const override;
    virtual std::string str() const override;
  }; // array


  class Literal {
    BasePtr _ptr;

  public:
    Literal() = default;
    Literal(BasePtr ptr):
      _ptr(ptr)
    {}

    BasePtr get() { return _ptr; }
    BasePtrConst get() const { return _ptr; }

    Literal clone() const { return _ptr->clone(); }
    std::string str() const { return _ptr->str(); }
    types::TypeHandle type() const { return _ptr->type(); }
  };

  
  struct FunctionPointer: Base {
    std::string _functionName;
      
    FunctionPointer(types::TypeHandle functionType, std::string const &fname, API_CTX_IGNORE);
    FunctionPointer(FunctionPointer const &other) = default;
      
    virtual BasePtr clone() const override;      
    virtual std::string str() const override;
    std::string const &functionName() const;
  };
    
  template <typename V> requires std::derived_from<V, Base>
  auto cast(Literal v) {
    auto ptr = std::dynamic_pointer_cast<std::remove_cvref_t<V>>(v.get());
    assert(ptr != nullptr && "invalid value cast");
    return ptr;
  }
} // namespace literal::mpl

