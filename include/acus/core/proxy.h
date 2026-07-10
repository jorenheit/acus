// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <optional>
#include <variant>
#include <functional>
#include "acus/api/api.h"
#include "acus/core/slot.h"
#include "acus/types/literal.h"

namespace acus {
  class Builder;
}

namespace acus::proxy {

  enum class Kind {
    Direct,
    GlobalReference,
    ArrayElement,
    StructField,
    DereferencedPointer
  };
  
  namespace impl {
    class Base;
    using BasePtr = std::shared_ptr<Base>;
    using BasePtrConst = std::shared_ptr<Base const>;
    using SlotWriteCallback = std::function<void(Slot)>;
    
    class SlotProxy {
      BasePtr _ptr;

    public:
      SlotProxy() = default;
      SlotProxy(BasePtr ptr);
      SlotProxy(Slot slot);

      types::TypeHandle type() const;
      Slot addressOf(Assembler &a, API_CTX) const;
      bool dependsOn(SlotProxy other) const;
      bool dependsOnDereferencedPointer() const;
      // TODO: I don't think directAbsolute is ever used, just rename directRelative to direct
      bool directAbsolute() const;
      bool directRelative() const;
      std::optional<SlotProxy> enclosingProxy();
      Kind kind() const;
      Slot materialize(Assembler &a) const;
      void materialize(Assembler &a, Slot dest) const;
      std::string name() const;
      std::string uniqueName() const;
      void write(Assembler &a, SlotProxy src, TransferMode mode) const;
      void write(Assembler &a, acus::literal::Literal src) const;
      void write(Assembler &a, SlotWriteCallback const &writeInto) const;

      bool operator==(SlotProxy const &other) const;
    };
    
    class Base {
      types::TypeHandle _type;
    public:
      Base(types::TypeHandle t): _type(t) {}
      types::TypeHandle type() const { return _type; }
      virtual ~Base() = default;

      virtual Slot addressOf(Assembler &a, API_CTX) const = 0;
      virtual bool dependsOn(SlotProxy other) const = 0;
      virtual bool dependsOnDereferencedPointer() const = 0;
      virtual bool directAbsolute() const = 0;
      virtual bool directRelative() const = 0;
      virtual std::optional<SlotProxy> enclosingProxy() const = 0;
      virtual Kind kind() const = 0;
      virtual Slot materialize(Assembler &a) const = 0;
      virtual void materialize(Assembler &a, Slot dest) const = 0;      
      virtual std::string name() const = 0;
      virtual std::string uniqueName() const = 0;
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const = 0;
      virtual void write(Assembler &a, acus::literal::Literal src) const = 0;
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const = 0;
    };
    

    class Direct: public Base {
      Slot _slot;
    
    public:
      Direct(Slot slot);

      virtual Slot addressOf(Assembler &a, API_CTX) const override;
      virtual bool dependsOn(SlotProxy) const override;
      virtual bool dependsOnDereferencedPointer() const override;
      virtual bool directAbsolute() const override;
      virtual bool directRelative() const override;
      virtual std::optional<SlotProxy> enclosingProxy() const;
      virtual Kind kind() const override;
      virtual Slot materialize(Assembler &a) const override;
      virtual void materialize(Assembler &a, Slot dest) const override;
      virtual std::string name() const override;
      virtual std::string uniqueName() const override;
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;      
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const override;
    };

    class GlobalReference: public Base {
      Slot _slot;
      
    public:
      GlobalReference(Slot slot);

      virtual Slot addressOf(Assembler &a, API_CTX) const override;
      virtual Kind kind() const override;
      virtual bool directAbsolute() const override;
      virtual bool directRelative() const override;
      virtual std::optional<SlotProxy> enclosingProxy() const override;
      virtual bool dependsOn(SlotProxy) const override;
      virtual bool dependsOnDereferencedPointer() const override;
      virtual Slot materialize(Assembler &a) const override;      
      virtual void materialize(Assembler &a, Slot dest) const override;      
      virtual std::string name() const override;
      virtual std::string uniqueName() const override;
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const override;      
      virtual void write(Assembler &a, acus::literal::Literal src) const override;      
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const override;
    };
    

    class ArrayElement: public Base {
      SlotProxy _arr;
      std::variant<int, SlotProxy> _index;
    
    public:
      ArrayElement(SlotProxy arr, int index);      
      ArrayElement(SlotProxy arr, SlotProxy index);

      virtual Slot addressOf(Assembler &a, API_CTX) const override;
      virtual Kind kind() const override;  
      virtual bool directAbsolute() const override;
      virtual bool directRelative() const override;      
      virtual bool dependsOn(SlotProxy other) const override;
      virtual bool dependsOnDereferencedPointer() const override;
      virtual std::optional<SlotProxy> enclosingProxy() const override;
      virtual Slot materialize(Assembler &a) const override;
      virtual void materialize(Assembler &a, Slot target) const override;      
      virtual std::string name() const override;
      virtual std::string uniqueName() const override;            
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const override;      

    private:
      Slot materializeImpl(Assembler &a, int index) const;
      void materializeImpl(Assembler &a, SlotProxy index, Slot target) const;

      void writeImpl(Assembler &a, int index, SlotProxy src, TransferMode mode) const;
      void writeImpl(Assembler &a, int index, acus::literal::Literal) const;
      void writeImpl(Assembler &a, int index, SlotWriteCallback const &writeInto) const;
      void writeImpl(Assembler &a, SlotProxy index, SlotProxy src, TransferMode mode) const;
      void writeImpl(Assembler &a, SlotProxy index, acus::literal::Literal) const;
      void writeImpl(Assembler &a, SlotProxy index, SlotWriteCallback const &writeInto) const;

      Slot getElementSlot(Slot arrSlot, int index) const;
      std::string constructName(auto&& getName) const;
    }; // ArrayElement

    class StructField: public Base {
      SlotProxy _obj;
      int _fieldIndex;
      int _fieldOffset;
      std::string _fieldName;

    public:
      StructField(SlotProxy obj, std::string fieldName);

      virtual Slot addressOf(Assembler &a, API_CTX) const override;
      virtual Kind kind() const override;
      virtual bool dependsOn(SlotProxy other) const override;      
      virtual bool dependsOnDereferencedPointer() const override;      
      virtual bool directAbsolute() const override;
      virtual bool directRelative() const override;      
      virtual std::optional<SlotProxy> enclosingProxy() const;
      virtual Slot materialize(Assembler &a) const override;
      virtual void materialize(Assembler &a, Slot target) const override;
      virtual std::string name() const override;
      virtual std::string uniqueName() const override;      
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const override;
      
    private:
      Slot getFieldSlot(Slot const obj) const;
    }; // StructField

    class DereferencedPointer: public Base {
      SlotProxy _ptr;

    public:
      DereferencedPointer(SlotProxy ptr);

      virtual Slot addressOf(Assembler &a, API_CTX) const override;      
      virtual bool dependsOn(SlotProxy other) const override; 
      virtual bool dependsOnDereferencedPointer() const override;      
      virtual bool directAbsolute() const override;
      virtual bool directRelative() const override;
      virtual std::optional<SlotProxy> enclosingProxy() const override;      
      virtual Kind kind() const override;
      virtual Slot materialize(Assembler &a) const override;
      virtual void materialize(Assembler &a, Slot target) const override;
      virtual std::string name() const override;
      virtual std::string uniqueName() const override;      
      virtual void write(Assembler &a, SlotProxy src, TransferMode mode) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;
      virtual void write(Assembler &a, SlotWriteCallback const &writeInto) const override;
    };
    

    
  } // namespace impl
      
  using SlotProxy = impl::SlotProxy;
  
  inline SlotProxy direct(Slot slot) {
    return SlotProxy(std::make_shared<impl::Direct>(slot));
  }

  inline SlotProxy globalReference(Slot slot) {
    return SlotProxy(std::make_shared<impl::GlobalReference>(slot));
  }
  
  inline SlotProxy arrayElement(SlotProxy arr, int index) {
    return SlotProxy(std::make_shared<impl::ArrayElement>(std::move(arr), index));
  }

  inline SlotProxy arrayElement(SlotProxy arr, SlotProxy index) {
    return SlotProxy(std::make_shared<impl::ArrayElement>(std::move(arr), std::move(index)));
  }  

  inline SlotProxy structField(SlotProxy obj, std::string const &fieldName) {
    return SlotProxy(std::make_shared<impl::StructField>(std::move(obj), fieldName));
  }

  inline SlotProxy dereferencedPointer(SlotProxy ptr) {
    return SlotProxy(std::make_shared<impl::DereferencedPointer>(std::move(ptr)));
  }
  
} // namespace acus::proxy
  
namespace acus {
  using SlotProxy = proxy::SlotProxy;
}
  
