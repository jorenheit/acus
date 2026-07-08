// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"


std::optional<Slot> Assembler::localSlot(std::string const &varName) const {
  Function::Scope *targetScope = _currentScope;
  while (true) {
    for (Slot const &slot: _currentFunction->frame.locals) {
      if (slot.name() == varName && slot.scope() == targetScope) {
	return slot;
      }
    }
    if (targetScope == nullptr) break;
    targetScope = targetScope->parent;
  }
  
  return {};
}

std::optional<Slot> Assembler::globalSlot(std::string const &varName) const {
 for (Slot const &slot: _program.globals) {
   if (slot.name() == varName) {
      return slot;
    }
  }
 
 return {};
}

SlotProxy Assembler::proxyFromVariableName(std::string const& varName, API_CTX) const {
  assert(_currentFunction != nullptr);

  // Try to find local slot
  if (auto const result = localSlot(varName); result.has_value()) {
    return proxy::direct(*result);
  }
  
  // Not found in local scope, check globals
  if (auto const result = globalSlot(varName); result.has_value()) {
    return proxy::globalReference(*result);
  }

  API_REQUIRE(false, error::ErrorCode::NameNotInScope, "'", varName, "' was not declared in this scope.");
  std::unreachable();
}

std::string Assembler::makeFullName(std::string const &name) {
  std::string result = _currentFunction->name + "::";
  if (_currentScope != nullptr) {
    result += "scope<" + std::to_string(_currentScope->id) + ">::";
  }
  result += name;
  return result;
}

std::string Assembler::makeFullGlobalName(std::string const &name) {
  return "global::" + name;
}


Slot Assembler::allocSlot(std::string const &name, types::TypeHandle type, SlotData::Kind kind) {

  assert(_currentFunction != nullptr);
  
  auto const tryFindAvailableSlot = [&](std::string const &name,
				        types::TypeHandle type,
				        SlotData::Kind kind) -> std::optional<Slot> {
    
    // Now check if there is an existing slot that fits this type
    auto &frame = _currentFunction->frame;  
    for (auto slot: frame.locals) {
      if (slot.kind() != Slot::Available || slot.type()->size() < type->size())
	continue;
      
      // Reuse this slot
      SlotData &data = slot.get();
      data.name = name;
      data.uniqueName = makeFullName(name);
      data.type = type;
      data.kind = kind;
      data.scope = _currentScope;
      
      // Split the slot if there is still room
      int const diff = slot.type()->size() - type->size();      
      if (diff > 0) {
	std::string const dummyName = [] {
	  static int counter = 0; return "__dummy_" + std::to_string(counter++);
	}();
	
	frame.locals.push_back( Slot {
	    SlotData {
	      .name = dummyName,
	      .uniqueName = makeFullName(dummyName),
	      .type = ts::raw(diff),
	      .kind = Slot::Available,
	      .offset = slot.offset() + slot.type()->size(),
	      .scope = nullptr
	    },
	    true
	  });
      }
      return slot;
    }
    return {};
  };

  auto const newSlot = [&](std::string const &name,
			   types::TypeHandle type,
			   SlotData::Kind kind) -> Slot {

    auto &frame = _currentFunction->frame;
    SlotData newSlot {
      .name = name,
      .uniqueName = makeFullName(name),
      .type = type,
      .kind = kind,
      .offset = frame.localBase() + frame.localAreaSize(),
      .scope = _currentScope
    };

    frame.locals.emplace_back(newSlot, true);
    return frame.locals.back();
  };

  auto opt = tryFindAvailableSlot(name, type, kind);
  if (opt) return *opt;
  return newSlot(name, type, kind);
}

void Assembler::mergeAvailableSlots() {
  auto &locals = _currentFunction->frame.locals;

  bool changed = true;
  while (changed) {
    changed = false;
    
    for (size_t i = 0; i < locals.size() && !changed; ++i) {
      Slot &a = locals[i];
      if (a.kind() != Slot::Available) continue;

      for (size_t j = 0; j < locals.size(); ++j) {
        if (i == j) continue;
	Slot &b = locals[j];
        if (b.kind() != Slot::Available) continue;

	// a just before b
        if (a.offset() + a.size() == b.offset()) {
          a.get().type = ts::raw(a.size() + b.size());
          locals.erase(locals.begin() + static_cast<std::ptrdiff_t>(j));
          changed = true;
          break;
        }

	// b just before a
        if (b.offset() + b.size() == a.offset()) {
          b.get().type = ts::raw(b.size() + a.size());
          locals.erase(locals.begin() + static_cast<std::ptrdiff_t>(i));
          changed = true;
          break;
        }
      }
    }
  }
}

void Assembler::markSlotAvailable(Slot slot) {
  assert(slot.managed());
  
  size_t const size = slot.size();

  SlotData &data = slot.get();
  data.name = "";
  data.uniqueName = "";
  data.type = ts::raw(size);
  data.kind = Slot::Available;
  data.scope = nullptr;
}

void Assembler::markSlotsAvailable(auto&& condition) {
  for (auto slot: _currentFunction->frame.locals) {
    if (condition(slot)) {
      markSlotAvailable(slot);
    }
  }
}

void Assembler::markSlotTemp(Slot slot) {
  assert(slot.managed());

  SlotData &data = slot.get();
  data.name = "";
  data.uniqueName = "";
  data.kind = Slot::Temp;
}

bool Assembler::freeAllSlots(auto&& condition) {
  bool success = false;
  for (auto &slot: _currentFunction->frame.locals) {
    if (condition(slot)) {
      markSlotAvailable(slot);
      success = true;
    }
  }

  if (success) mergeAvailableSlots();
  return success;
}


void Assembler::freeTempSlot(Slot target) {
  assert(target.kind() == Slot::Temp);
  assert(target.managed());

  markSlotAvailable(target);
  mergeAvailableSlots();
}


void Assembler::freeTempSlots() {
  freeAllSlots([&](Slot slot){
    return slot.kind() == Slot::Temp;
  });
}

void Assembler::freeCacheSlot(Slot target) {
  assert(target.kind() == Slot::Cache);
  assert(target.managed() == true);

  markSlotAvailable(target);
  mergeAvailableSlots();
}

void Assembler::freeCacheSlots() {
  freeAllSlots([&](Slot slot) {
    return slot.kind() == Slot::Cache;
  });
}

void Assembler::freeScope(Function::Scope const *scope) {
  freeAllSlots([&](Slot slot) {
    return slot.scope() == scope;
  });
}

Slot Assembler::getTemp(types::TypeHandle type) {
  assert(_currentBlock != nullptr);
  static size_t tmpID = 0;
  return allocSlot("__tmp_" + std::to_string(tmpID++), type, Slot::Temp);
}

Slot Assembler::getTemp(literal::Literal value) {
  Slot tmp = getTemp(value.type());
  assignSlot(tmp, value);
  return tmp;
}

Slot Assembler::getCache(types::TypeHandle type) {
  assert(_currentBlock != nullptr);
  static size_t cacheID = 0;
  return allocSlot("__cache_" + std::to_string(cacheID++), type, Slot::Cache);
}

Slot Assembler::getCache(literal::Literal value) {
  Slot const slot = getCache(value.type());
  assignSlot(slot, value);
  return slot;
}

void Assembler::declareGlobal(std::string const &name, types::TypeHandle type, API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_DECLARE_GLOBAL_ALLOWED();
  API_REQUIRE_INSIDE_PROGRAM_BLOCK();
  API_REQUIRE_OUTSIDE_FUNCTION_BLOCK();
  API_REQUIRE_GLOBAL_NAME_AVAILABLE(name);

  int const offset = _program.globalVariableFrameSize();
  auto slot  = Slot {
    SlotData {
      .name = name,
      .uniqueName = makeFullGlobalName(name),
      .type = type,
      .kind = Slot::Global,
      .offset = offset,
      .scope = nullptr
    },
    true
  };

  _program.globals.emplace_back(slot);
}


Expression Assembler::declareLocal(std::string const& name, types::TypeHandle type, API_FUNC) {
  API_FUNC_BEGIN();
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_NOT_IN_CURRENT_SCOPE(name);

  return Expression{allocSlot(name, type, Slot::Local)};
}

