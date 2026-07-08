// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  impl::StructField::StructField(SlotProxy obj, std::string fieldName):
    Base(types::cast<types::StructType>(obj.type())->fieldType(fieldName)),
    _obj(obj),
    _fieldIndex(types::cast<types::StructType>(obj.type())->fieldIndex(fieldName)),
    _fieldOffset(types::cast<types::StructType>(obj.type())->fieldOffset(fieldName)),
    _fieldName(fieldName)
  {}
  
  Kind impl::StructField::kind() const {
    return Kind::StructField;
  }     
      
  bool impl::StructField::directRelative() const {
    return true;
  }
      
  bool impl::StructField::directAbsolute() const {
    return _obj.directAbsolute();
  }

  bool impl::StructField::dependsOnDereferencedPointer() const {
    return _obj.dependsOnDereferencedPointer();
  }
      
  std::string impl::StructField::name() const {
    return _obj.name() + "." + _fieldName;
  }
  
  std::string impl::StructField::uniqueName() const {
    return _obj.uniqueName() + "." + _fieldName;
  }
  
  std::optional<SlotProxy> impl::StructField::enclosingProxy() const {
    return _obj;
  }

  bool impl::StructField::dependsOn(SlotProxy other) const {
    return other == _obj;
  }
  
  Slot impl::StructField::getFieldSlot(Slot const obj) const {
    auto structType = static_cast<types::StructType const *>(_obj.type());
    return Slot {
      SlotData {
	.name = std::string("__field_") + name(),
	.uniqueName = std::string("__field_") + uniqueName(),
	.type = structType->_fields[_fieldIndex].type,
	.kind = obj.kind() == Slot::Temp ? Slot::Temp : Slot::Dummy,
	.offset = obj.offset() + _fieldOffset
      },
      false
    };
  }
    
  Slot impl::StructField::materialize(Assembler &a) const {
    return getFieldSlot(a.materialize(_obj));
  }

  void impl::StructField::materialize(Assembler &, Slot) const {
    assert(false && "struct field materialization never requires a target slot");
    std::unreachable();
  }
  
  // Write an anonymous value to a slot at known offset
  void impl::StructField::write(Assembler &a, literal::Literal src) const {
    Slot const objSlot = a.materialize(_obj);
    Slot const fieldSlot = getFieldSlot(objSlot);
    a.assignSlot(fieldSlot, src);
  }

  // Write a slot-proxy to a slot at known offset
  void impl::StructField::write(Assembler &a, SlotProxy src) const {
    Slot const objSlot = a.materialize(_obj);
    Slot const srcSlot = a.materialize(src);
    Slot const fieldSlot = getFieldSlot(objSlot);
    a.assignSlot(fieldSlot, srcSlot);
  }

  void impl::StructField::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    Slot const objSlot = a.materialize(_obj);
    Slot const fieldSlot = getFieldSlot(objSlot);
    writeInto(fieldSlot);
  }
  
  Slot impl::StructField::addressOf(Assembler &a) const {
    Slot ptr = _obj.addressOf(a);
    ptr.get().type = ts::pointer(this->type());
    a.addAssign(ptr, literal::u16(_fieldOffset));
    return ptr;
  }

  
} // namespace proxy
