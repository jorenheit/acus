// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus.h"
#include <iostream>
#include <stdexcept>

namespace acus {
// Inspect allocator state without exposing allocation internals in the API.
struct MemoryRegressionAccess {
  static void require(bool condition, char const *message) {
    if (!condition) throw std::runtime_error(message);
  }
  static void start(Assembler &c) {
    c.program("memory-regression", "main").begin();
    c.function("main").begin();
  }
  static int area(Assembler const &c) {
    return c._currentFunction->frame.localAreaSize();
  }
  static void run() {
    {
      Assembler c; start(c);
      auto first = c.getTemp(ts::raw(4));
      c.freeTempSlot(first);
      auto reused = c.getTemp(ts::u8()); // also split the block
      require(reused.offset() == first.offset(), "free block was not reused");
      c.freeTempSlot(reused);
      require(area(c) == 4, "split changed the reserved extent");
      auto whole = c.getTemp(ts::raw(4));
      require(whole.offset() == first.offset() && area(c) == 4,
              "freeing a reused handle did not release the allocator record");
    }
    {
      Assembler c; start(c);
      auto large = c.getTemp(ts::raw(8));
      auto guard = c.getTemp(ts::u8());
      auto small = c.getTemp(ts::raw(2));
      c.freeTempSlot(large); c.freeTempSlot(small);
      auto two = c.getTemp(ts::raw(2));
      auto eight = c.getTemp(ts::raw(8));
      require(two.offset() == small.offset() && eight.offset() == large.offset(),
              "best-fit allocation wasted the larger hole");
      require(area(c) == 11 && guard.kind() == Slot::Temp,
              "best-fit allocation grew the frame or damaged a live slot");
    }
    {
      Assembler c; start(c);
      auto tail = c.getTemp(ts::raw(3)); c.freeTempSlot(tail);
      auto grown = c.getTemp(ts::raw(4));
      require(grown.offset() == tail.offset() && area(c) == 4,
              "a free tail was not extended in place");
      c.freeTempSlot(grown);
      c.getTemp(ts::u8());
      require(area(c) == 4, "reuse discarded the reserved frame extent");
    }
    {
      Assembler c; start(c);
      c.scope().begin();
      c.declareLocal("arr", ts::array(ts::u8(), 4));
      c.declareLocal("idx", ts::u8());
      c.assign("idx", literal::u8(1));
      c.assign(c.arrayElement("arr", "idx"), literal::u8(42));
      require(!c._cache.empty(), "scope test did not create a cache entry");
      c.endScope();
      require(c._cache.empty(), "cache retained storage from an ended scope");
      for (auto const &slot: c._currentFunction->frame.locals)
        require(slot.kind() == Slot::Available,
                "scope cleanup left an allocation live");
    }
    for (auto type: {ts::s8(), ts::s16()}) {
      Assembler c; start(c);
      c.declareLocal("x", type);
      c.assign("x", type == ts::s8() ? literal::s8(-12) : literal::s16(-1234));
      c.printDecimalSlotSigned(*c.localSlot("x"));
      int const firstArea = area(c);
      for (int i = 0; i != 10; ++i)
        c.printDecimalSlotSigned(*c.localSlot("x"));
      require(area(c) == firstArea, "repeated signed printing grew the frame");
      for (auto const &slot: c._currentFunction->frame.locals)
        require(slot.kind() != Slot::Temp, "signed printing leaked a temporary");
    }
  }
};
}

int main() {
  try {
    acus::MemoryRegressionAccess::run();
    std::cout << "All memory regression tests passed\n";
    return 0;
  } catch (std::exception const &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
