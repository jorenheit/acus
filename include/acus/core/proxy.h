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

    struct SlotProxy: public BasePtr {
      SlotProxy(BasePtr ptr): BasePtr(std::move(ptr)) {}
      SlotProxy(Slot const &slot);
      bool operator==(SlotProxy const &other) const;
    };
    
    class Base {
      types::TypeHandle _type;
    public:
      Base(types::TypeHandle t): _type(t) {}
      virtual ~Base() = default;

      types::TypeHandle type() const { return _type; }
      virtual Kind kind() const = 0;
      virtual std::string name() const = 0;
      virtual std::string uniqueName() const = 0;
      virtual Slot materialize(Assembler &a, bool const writeIntent = false) const = 0;
      virtual void materialize(Assembler &a, Slot const &dest, bool const writeIntent = false) const = 0;      
      virtual void write(Assembler &a, SlotProxy src) const = 0;
      virtual void write(Assembler &a, acus::literal::Literal src) const = 0;
      virtual Slot addressOf(Assembler &a) const = 0;
      virtual bool directAbsolute() const = 0;
      virtual bool directRelative() const = 0;
      virtual std::optional<SlotProxy> enclosingProxy() const = 0;
      virtual bool dependsOn(SlotProxy other) const = 0;
      virtual bool dependsOnDereferencedPointer() const = 0;
    };
    

    class Direct: public Base {
      Slot _slot;
    
    public:
      Direct(Slot const &slot): Base(slot.type), _slot(slot) {} 
      virtual Kind kind() const { return Kind::Direct; }     
      virtual Slot materialize(Assembler &a, bool const writeIntent = false) const override;
      virtual void materialize(Assembler &a, Slot const &dest, bool const writeIntent = false) const override;      
      virtual void write(Assembler &a, SlotProxy src) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;      
      virtual bool directAbsolute() const override { return true; }
      virtual bool directRelative() const override { return true; }
      virtual Slot addressOf(Assembler &a) const override;
      virtual bool dependsOnDereferencedPointer() const { return false; }

      virtual std::string name() const override {
	return _slot.name;
      }

      virtual std::string uniqueName() const override {
	return _slot.uniqueName;
      }

      virtual std::optional<SlotProxy> enclosingProxy() const {
	return {};
      }

      virtual bool dependsOn(SlotProxy) const override { return false; }
      
    };

    class GlobalReference: public Base {
      Slot _slot;
      
    public:
      GlobalReference(Slot const &slot): Base(slot.type), _slot(slot) {}

      virtual std::string name() const override {
	return "global<" + _slot.name + ">";
      }

      virtual Kind kind() const { return Kind::GlobalReference; }     
      
      virtual std::string uniqueName() const override {
	return "global<" + _slot.uniqueName + ">";
      }

      virtual bool directAbsolute() const override { return false; }
      virtual bool directRelative() const override { return false; }
      virtual std::optional<SlotProxy> enclosingProxy() const override { return {}; }
      virtual bool dependsOn(SlotProxy) const override { return false; }
      
      virtual Slot materialize(Assembler &a, bool const writeIntent = false) const override;      
      virtual void materialize(Assembler &a, Slot const &dest, bool const writeIntent = false) const override;      
      virtual void write(Assembler &a, SlotProxy src) const override;      
      virtual void write(Assembler &a, acus::literal::Literal src) const override;      
      virtual Slot addressOf(Assembler &a) const override;
      virtual bool dependsOnDereferencedPointer() const { return false; }
      
    };
    

    
    class ArrayElement: public Base {
      SlotProxy _arr;
      std::variant<int, SlotProxy> _index;
    
    public:
      ArrayElement(SlotProxy arr, int index):
	Base(cast<types::ArrayLike>(arr->type())->elementType()),
	_arr(std::move(arr)),
	_index(index)
      {}
      
      ArrayElement(SlotProxy arr, SlotProxy index):
	Base(cast<types::ArrayLike>(arr->type())->elementType()),
	_arr(std::move(arr)),
	_index(std::move(index))
      {}

      virtual bool dependsOnDereferencedPointer() const {
	if (_arr->dependsOnDereferencedPointer()) return true;
	if (std::holds_alternative<SlotProxy>(_index)) {
	  return std::get<SlotProxy>(_index)->dependsOnDereferencedPointer();
	}
	return false;
      }

      
      virtual Kind kind() const { return Kind::ArrayElement; }     

      
      virtual bool directRelative() const override {
	return std::holds_alternative<int>(_index);
      }
      
      virtual bool directAbsolute() const override {
	return _arr->directAbsolute() && directRelative();
      }

      // TODO: name helper
      virtual std::string name() const override {
	std::string idx = std::holds_alternative<int>(_index)
	  ? std::to_string(std::get<int>(_index))
	  : std::get<SlotProxy>(_index)->name();
	return _arr->name() + "[" + idx + "]";
      }

      virtual std::string uniqueName() const override {
	std::string idx = std::holds_alternative<int>(_index)
	  ? std::to_string(std::get<int>(_index))
	  : std::get<SlotProxy>(_index)->uniqueName();
	return _arr->uniqueName() + "[" + idx + "]";
      }
      
      virtual Slot materialize(Assembler &a, bool const writeIntent) const override {
	assert(std::holds_alternative<int>(_index) && "indirect arrayElement requires a target (cache) slot");
	return materializeImpl(a, std::get<int>(_index), writeIntent);
      }

      virtual void materialize(Assembler &a, Slot const &target, bool const writeIntent) const override {
	assert(std::holds_alternative<SlotProxy>(_index) && "direct arrayElement does not require a target (cache) slot");
	materializeImpl(a, std::get<SlotProxy>(_index), target, writeIntent);
      }
      
      virtual void write(Assembler &a, SlotProxy src) const override {
	return std::holds_alternative<int>(_index)
	  ? writeImpl(a, std::get<int>(_index), src)
	  : writeImpl(a, std::get<SlotProxy>(_index), src);
      }

      virtual void write(Assembler &a, acus::literal::Literal src) const override {
	return std::holds_alternative<int>(_index)
	  ? writeImpl(a, std::get<int>(_index), src)
	  : writeImpl(a, std::get<SlotProxy>(_index), src);
      }
      
      virtual Slot addressOf(Assembler &a) const override;

      virtual std::optional<SlotProxy> enclosingProxy() const {
	return _arr;
      }

      virtual bool dependsOn(SlotProxy other) const override {
	if (other == _arr) return true;
	if (std::holds_alternative<SlotProxy>(_index) && std::get<SlotProxy>(_index) == other) return true;
	return false;
      }
      
    private:
      Slot materializeImpl(Assembler &a, int index, bool const writeIntent = false) const;
      void materializeImpl(Assembler &a, SlotProxy index, Slot const &target, bool const writeIntent = false) const;

      void writeImpl(Assembler &a, int index, SlotProxy src) const;
      void writeImpl(Assembler &a, int index, acus::literal::Literal) const;
      void writeImpl(Assembler &a, SlotProxy index, SlotProxy src) const;
      void writeImpl(Assembler &a, SlotProxy index, acus::literal::Literal) const;

      Slot getElementSlot(Slot const &arrSlot, int index) const;
    }; // ArrayElement

    class StructField: public Base {
      SlotProxy _obj;
      int _fieldIndex;
      int _fieldOffset;
      std::string _fieldName;

    public:
      StructField(SlotProxy obj, std::string fieldName);

      virtual Kind kind() const { return Kind::StructField; }     
      
      virtual bool directRelative() const override {
	return true;
      }
      
      virtual bool directAbsolute() const override {
	return _obj->directAbsolute();
      }

      virtual bool dependsOnDereferencedPointer() const {
	return _obj->dependsOnDereferencedPointer();
      }
      
      // TODO: name helper
      virtual std::string name() const override {
	return _obj->name() + "." + _fieldName;
      }

      virtual std::string uniqueName() const override {
	return _obj->uniqueName() + "." + _fieldName;
      }
      
      virtual Slot materialize(Assembler &a, bool const writeIntent = false) const override;
      virtual void materialize(Assembler &a, Slot const &target, bool const writeIntent = false) const override;
      virtual void write(Assembler &a, SlotProxy src) const override;
      virtual void write(Assembler &a, acus::literal::Literal src) const override;
      virtual Slot addressOf(Assembler &a) const override;


      virtual std::optional<SlotProxy> enclosingProxy() const {
	return _obj;
      }

      virtual bool dependsOn(SlotProxy other) const override {
	return other == _obj;
      }
      
      
    private:
      Slot getFieldSlot(Slot const obj) const;
    }; // StructField

    class DereferencedPointer: public Base {
      SlotProxy _ptr;

    public:
      DereferencedPointer(SlotProxy ptr):
	Base(types::cast<types::PointerType>(ptr->type())->pointeeType()),
	_ptr(std::move(ptr))
      {}

      virtual Kind kind() const { return Kind::DereferencedPointer; }     
      virtual bool dependsOnDereferencedPointer() const {
	return true;
      }
      
      virtual Slot materialize(Assembler &a, bool const writeIntent = false) const;
      virtual void materialize(Assembler &a, Slot const &target, bool const writeIntent = false) const;
      virtual void write(Assembler &a, SlotProxy src) const;;
      virtual void write(Assembler &a, acus::literal::Literal src) const;
      virtual Slot addressOf(Assembler &a) const override;      
      virtual bool directRelative() const { return false; }
      virtual bool directAbsolute() const { return false; }

      // TODO: name helper
      virtual std::string name() const {
	return std::string("deref<") + _ptr->name() + ">";
      }

      virtual std::string uniqueName() const {
	return std::string("deref<") + _ptr->uniqueName() + ">";
      }
      
      virtual std::optional<SlotProxy> enclosingProxy() const {
	return {};
      }
      
      virtual bool dependsOn(SlotProxy other) const override {
	return other == _ptr;
      }

      
    };
    

    
  } // namespace impl
      
  using SlotProxy = impl::SlotProxy;
  
  inline SlotProxy direct(Slot const &slot) {
    return SlotProxy(std::make_shared<impl::Direct>(slot));
  }

  inline SlotProxy globalReference(Slot const &slot) {
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
  using SlotProxy = proxy::impl::SlotProxy;
}
  
