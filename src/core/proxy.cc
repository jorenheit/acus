// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  impl::SlotProxy::SlotProxy(Slot const &slot):
    BasePtr(proxy::direct(slot))
  {}
  
  bool impl::SlotProxy::operator==(SlotProxy const &other) const {
    return (*this)->uniqueName() == other->uniqueName();
  }

  
  Slot impl::Direct::materialize(Assembler &, bool const) const {
    return _slot;
  }

  void impl::Direct::materialize(Assembler &, Slot const &, bool const) const {
    assert(false && "trying to materialize a direct proxy into another slot");
  }
  
  void impl::Direct::write(Assembler &a, SlotProxy src) const {
    a.assignSlot(_slot, a.materialize(src));
  }

  void impl::Direct::write(Assembler &a, literal::Literal src) const {
    a.assignSlot(_slot, src);
  }

  Slot impl::Direct::addressOf(Assembler &a) const {
    return a.addressOfSlot(_slot);
  }
  
  Slot impl::ArrayElement::getElementSlot(Slot const &arrSlot, int index) const {
    return arrSlot.sub(this->type(), index * this->type()->size());
  }

  // Materialize a slot at known offset
  Slot impl::ArrayElement::materializeImpl(Assembler &a, int index, bool const writeIntent) const {
    return getElementSlot(a.materialize(_arr, writeIntent), index);
  }

  // Materialize a slot at unknown offset
  void impl::ArrayElement::materializeImpl(Assembler &a, SlotProxy index, Slot const &target, bool const) const {
    Slot const arrSlot = a.materialize(_arr); // not intending to write directly to this slot, even for writeIntent = true
    Slot const indexSlot = a.materialize(index);
    a.copyElementIntoSlot(target, arrSlot, indexSlot);
  }

  // Write an anonymous value to a slot at known offset
  void impl::ArrayElement::writeImpl(Assembler &a, int index, literal::Literal src) const {
    Slot const arrSlot = a.materialize(_arr, true); // known offset -> we're about to write to this slot
    Slot const elementSlot = getElementSlot(arrSlot, index);
    a.assignSlot(elementSlot, src);
  }

  // Write a slot-proxy to a slot at known offset
  void impl::ArrayElement::writeImpl(Assembler &a, int index, SlotProxy src) const {
    Slot const arrSlot = a.materialize(_arr, true); // known offset -> we're about to write to this slot
    Slot const srcSlot = a.materialize(src);
    Slot const elementSlot = getElementSlot(arrSlot, index);
    a.assignSlot(elementSlot, srcSlot);
  }

  // Write an anonymous value to a dynamic offset
  void impl::ArrayElement::writeImpl(Assembler &a, SlotProxy index, literal::Literal src) const {
    Slot const arrSlot   = a.materialize(_arr, true); // known offset -> we're about to write to this slot
    Slot const indexSlot = a.materialize(index);
    a.copyConstIntoElement(src, arrSlot, indexSlot);
  }

  // Write a slot-proxy to a dynamic offset
  void impl::ArrayElement::writeImpl(Assembler &a, SlotProxy index, SlotProxy src) const {
    Slot const arrSlot = a.materialize(_arr, true);
    Slot const srcSlot = a.materialize(src);
    Slot const indexSlot = a.materialize(index);
    a.copySlotIntoElement(srcSlot, arrSlot, indexSlot);
  }

  Slot impl::ArrayElement::addressOf(Assembler &a) const {
    Slot ptr = _arr->addressOf(a);
    ptr.type = ts::pointer(this->type());
    if (std::holds_alternative<int>(_index)) {
      a.addAssign(ptr, literal::u16(std::get<int>(_index)));
    } else {
      a.addAssign(ptr, std::get<SlotProxy>(_index));
    }
    return ptr;
  }
  
  impl::StructField::StructField(SlotProxy obj, std::string fieldName):
    Base(types::cast<types::StructType>(obj->type())->fieldType(fieldName)),
    _obj(obj),
    _fieldIndex(types::cast<types::StructType>(obj->type())->fieldIndex(fieldName)),
    _fieldOffset(types::cast<types::StructType>(obj->type())->fieldOffset(fieldName)),
    _fieldName(fieldName)
  {}
  
  Slot impl::StructField::getFieldSlot(Slot const obj) const {
    auto structType = static_cast<types::StructType const *>(_obj->type());
    return Slot {
      .name = std::string("__field_") + name(),
      .type = structType->_fields[_fieldIndex].type,
      .kind = obj.kind == Slot::Temp ? Slot::Temp : Slot::StructField,
      .offset = obj.offset + _fieldOffset
    };  
  }
    
  Slot impl::StructField::materialize(Assembler &a, bool const writeIntent) const {
    return getFieldSlot(a.materialize(_obj, writeIntent));
  }

  void impl::StructField::materialize(Assembler &, Slot const &, bool const) const {
    assert(false && "struct field materialization never requires a target slot");
    std::unreachable();
  }
  
  // Write an anonymous value to a slot at known offset
  void impl::StructField::write(Assembler &a, literal::Literal src) const {
    Slot const objSlot = a.materialize(_obj, true);
    Slot const fieldSlot = getFieldSlot(objSlot);
    a.assignSlot(fieldSlot, src);
  }

  // Write a slot-proxy to a slot at known offset
  void impl::StructField::write(Assembler &a, SlotProxy src) const {
    Slot const objSlot = a.materialize(_obj, true);
    Slot const srcSlot = a.materialize(src);
    Slot const fieldSlot = getFieldSlot(objSlot);
    a.assignSlot(fieldSlot, srcSlot);
  }

  Slot impl::StructField::addressOf(Assembler &a) const {
    Slot ptr = _obj->addressOf(a);
    ptr.type = ts::pointer(this->type());
    a.addAssign(ptr, literal::u16(_fieldOffset));
    return ptr;
  }

  Slot impl::DereferencedPointer::materialize(Assembler &, bool const) const {
    assert(false && "dereferenced pointer materialization always requires a target slot");
    std::unreachable();
  }

  void impl::DereferencedPointer::materialize(Assembler &a, Slot const &target, bool const writeIntent) const {
    Slot const ptrSlot = a.materialize(_ptr);
    a.dereferencePointerIntoSlot(ptrSlot, target);
  }

  void impl::DereferencedPointer::write(Assembler &a, SlotProxy src) const {
    Slot const ptrSlot = a.materialize(_ptr);
    Slot const srcSlot = a.materialize(src);
    a.writeSlotThroughDereferencedPointer(ptrSlot, srcSlot);
  }

  void impl::DereferencedPointer::write(Assembler &a, literal::Literal src) const {
    Slot const ptrSlot = a.materialize(_ptr);
    a.writeConstThroughDereferencedPointer(ptrSlot, src);
  }

  Slot impl::DereferencedPointer::addressOf(Assembler &a) const {
    return a.materialize(_ptr);
  }


  // Global References
  Slot impl::GlobalReference::materialize(Assembler &a, bool const writeIntent) const {
    assert(false && "global reference materialization always requires a target slot");
  }
  
  void impl::GlobalReference::materialize(Assembler &a, Slot const &dest, bool const) const {
    a.fetchGlobal(_slot, dest);
  }
      
  void impl::GlobalReference::write(Assembler &a, SlotProxy src) const {
    Slot const srcSlot = a.materialize(src);
    a.putGlobal(_slot, srcSlot);
  }
      
  void impl::GlobalReference::write(Assembler &a, acus::literal::Literal src) const {
    a.putGlobal(_slot, src);
  }
      
  Slot impl::GlobalReference::addressOf(Assembler &a) const {
    return a.addressOfSlot(_slot);
  }

  
} // namespace proxy
