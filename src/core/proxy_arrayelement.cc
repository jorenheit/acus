// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/core/proxy.h"
#include "acus/assembler/assembler.h"

namespace acus::proxy {

  
  impl::ArrayElement::ArrayElement(SlotProxy arr, int index):
    Base(cast<types::ArrayLike>(arr.type())->elementType()),
    _arr(std::move(arr)),
    _index(index)
  {}
      
  impl::ArrayElement::ArrayElement(SlotProxy arr, SlotProxy index):
    Base(cast<types::ArrayLike>(arr.type())->elementType()),
    _arr(std::move(arr)),
    _index(std::move(index))
  {}

  Kind impl::ArrayElement::kind() const {
    return Kind::ArrayElement;
  }     

  bool impl::ArrayElement::dependsOnDereferencedPointer() const {
    if (_arr.dependsOnDereferencedPointer()) return true;
    if (std::holds_alternative<SlotProxy>(_index)) {
      return std::get<SlotProxy>(_index).dependsOnDereferencedPointer();
    }
    return false;
  }

  bool impl::ArrayElement::direct() const {
    return std::holds_alternative<int>(_index);
  }
      
  std::string impl::ArrayElement::constructName(auto&& getName) const {
    std::string idx = std::holds_alternative<int>(_index)
      ? std::to_string(std::get<int>(_index))
      : getName(std::get<SlotProxy>(_index));
    return getName(_arr) + "[" + idx + "]";
      
  }
  
  std::string impl::ArrayElement::name() const {
    return constructName([](SlotProxy proxy){ return proxy.name(); });
  }

  std::string impl::ArrayElement::uniqueName() const {
    return constructName([](SlotProxy proxy){ return proxy.uniqueName(); });
  }
      
  Slot impl::ArrayElement::materialize(Assembler &a) const {
    assert(std::holds_alternative<int>(_index) && "indirect arrayElement requires a target (cache) slot");
    return materializeImpl(a, std::get<int>(_index));
  }

  void impl::ArrayElement::materialize(Assembler &a, Slot target) const {
    assert(std::holds_alternative<SlotProxy>(_index) && "direct arrayElement does not require a target (cache) slot");
    materializeImpl(a, std::get<SlotProxy>(_index), target);
  }
      
  void impl::ArrayElement::write(Assembler &a, SlotProxy src, TransferMode mode) const {
    return std::holds_alternative<int>(_index)
      ? writeImpl(a, std::get<int>(_index), src, mode)
      : writeImpl(a, std::get<SlotProxy>(_index), src, mode);
  }

  void impl::ArrayElement::write(Assembler &a, acus::literal::Literal src) const {
    return std::holds_alternative<int>(_index)
      ? writeImpl(a, std::get<int>(_index), src)
      : writeImpl(a, std::get<SlotProxy>(_index), src);
  }

  void impl::ArrayElement::write(Assembler &a, SlotWriteCallback const &writeInto) const {
    return std::holds_alternative<int>(_index)
      ? writeImpl(a, std::get<int>(_index), writeInto)
      : writeImpl(a, std::get<SlotProxy>(_index), writeInto);
  }
      
  std::optional<SlotProxy> impl::ArrayElement::enclosingProxy() const {
    return _arr;
  }

  bool impl::ArrayElement::dependsOn(SlotProxy other) const {
    if (other == _arr) return true;
    if (std::holds_alternative<SlotProxy>(_index) && std::get<SlotProxy>(_index) == other) return true;
    return false;
  }
      
    
  Slot impl::ArrayElement::getElementSlot(Slot arrSlot, int index) const {
    return arrSlot.sub(this->type(), index * this->type()->size());
  }

  // Materialize a slot at known offset
  Slot impl::ArrayElement::materializeImpl(Assembler &a, int index) const {
    return getElementSlot(a.materialize(_arr), index);
  }

  // Materialize a slot at unknown offset
  void impl::ArrayElement::materializeImpl(Assembler &a, SlotProxy index, Slot target) const {
    Slot const arrSlot = a.materialize(_arr);
    Slot const indexSlot = a.materialize(index);
    a.copyElementIntoSlot(target, arrSlot, indexSlot);
  }

  // Write an anonymous value to a slot at known offset
  void impl::ArrayElement::writeImpl(Assembler &a, int index, literal::Literal src) const {
    Slot const arrSlot = a.materialize(_arr);
    Slot const elementSlot = getElementSlot(arrSlot, index);
    a.assignSlot(elementSlot, src);
  }

  // Write a slot-proxy to a slot at known offset
  void impl::ArrayElement::writeImpl(Assembler &a, int index, SlotProxy src, TransferMode mode) const {
    Slot const arrSlot = a.materialize(_arr);
    Slot const srcSlot = a.materialize(src);
    Slot const elementSlot = getElementSlot(arrSlot, index);
    a.assignSlot(elementSlot, srcSlot, mode);
  }

  // Apply a callback to a slot at known offset
  void impl::ArrayElement::writeImpl(Assembler &a, int index, SlotWriteCallback const &writeInto) const {
    Slot const arrSlot = a.materialize(_arr);
    Slot const elementSlot = getElementSlot(arrSlot, index);
    writeInto(elementSlot);
  }
  
  // Write an anonymous value to a dynamic offset
  void impl::ArrayElement::writeImpl(Assembler &a, SlotProxy index, literal::Literal src) const {
    Slot const arrSlot   = a.materialize(_arr);
    Slot const indexSlot = a.materialize(index);
    a.copyConstIntoElement(src, arrSlot, indexSlot);
  }

  // Write a slot-proxy to a dynamic offset
  void impl::ArrayElement::writeImpl(Assembler &a, SlotProxy index, SlotProxy src, TransferMode mode) const {
    Slot const arrSlot = a.materialize(_arr);
    Slot const srcSlot = a.materialize(src);
    Slot const indexSlot = a.materialize(index);
    a.copySlotIntoElement(srcSlot, arrSlot, indexSlot, mode);
  }

  // Apply a callback to a dynamic offset
  void impl::ArrayElement::writeImpl(Assembler &a, SlotProxy index, SlotWriteCallback const &writeInto) const {
    Slot tmp = a.getTemp(this->type());
    writeInto(tmp);
    writeImpl(a, index, tmp, TransferMode::Move);
    a.freeTempSlot(tmp);
  }

  Slot impl::ArrayElement::addressOf(Assembler &a, API_CTX) const {
    Slot ptr = _arr.addressOf(a, API_FWD);
    ptr.get().type = ts::pointer(this->type());
    if (std::holds_alternative<int>(_index)) {
      a.addAssign(ptr, literal::u16(std::get<int>(_index)));
    } else {
      a.addAssign(ptr, std::get<SlotProxy>(_index));
    }
    return ptr;
  }
 

} // namespace proxy
