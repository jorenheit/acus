// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <tuple>
#include <utility>
#include <vector>

namespace acus {

// ============================================================
// MacroCell
// ============================================================

  // TODO: restructure; scratch cells should be close to value cells
struct MacroCell {
  enum Field {
    Value0,
    Value1,
    FrameMarker,
    Flag,
    SeekMarker,
    Scratch0,
    Scratch1,
    Payload0,
    Payload1,
    
    FieldCount
  };
};

// ============================================================
// RuntimePointer
// ============================================================

struct RuntimePointer {
  enum Field {
    FrameDepth,
    Offset,
    Size // number of logical cells used by pointer
  };
};

// ============================================================
// Cell
// ============================================================

struct Cell {
  int offset = 0;
  MacroCell::Field field = MacroCell::Value0;
  operator int() const { return offset; }
};

// ============================================================
// DataPointer
// ============================================================

class DataPointer {
  Cell _current;

public:
  Cell const &current() const { return _current; }
  MacroCell::Field field() const { return _current.field; }
  int offset() const { return _current.offset; }
  
  void moveRelative(int logicalCells) { _current.offset += logicalCells; }
  void set(MacroCell::Field field) { _current.field = field; }
  void set(int offset, MacroCell::Field field = MacroCell::Value0) {
    _current.offset = offset;
    _current.field = field;
  }
  void set(Cell cell) { set(cell.offset, cell.field); }
};

// ============================================================
// Payload
// ============================================================

class Payload {
public:  
  enum class Width { None, Single, Double };

private:
  std::vector<Width> units;

public:
  template <typename ... Args>
  Payload(Args ... args);
  
  int size() const { return units.size(); }
  operator bool() const { return units.size() > 0; }
  Width width(int index) const;
  
private:
  void addPairs() {}

  template <typename... Rest>
  void addPairs(int count, Width width, Rest... rest);
};

// ============================================================
// Temps
// ============================================================

template <size_t N>
struct Temps {
  std::array<Cell, N> _cells;

  constexpr Temps() = delete;

  template <typename ... Cells> requires (sizeof...(Cells) == N)
  constexpr Temps(Cells... cells);

  template <size_t I>
  constexpr Cell const &get() const;

  template <size_t ... Is>
  constexpr auto select() const;

  template <typename ... Args> requires (sizeof...(Args) > 0)
  static constexpr Temps select(Args ... args);
};

#include "acus/core/data.tpp"

} // namespace acus
