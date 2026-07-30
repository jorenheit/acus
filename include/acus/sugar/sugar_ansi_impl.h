#pragma once
#include "acus/sugar/sugar_ansi.h"

namespace acus::sugar::impl {

  template <size_t Width, size_t Height, size_t Left, size_t Top>
  void Screen<Width, Height, Left, Top>::MoveTo::emit(Expr const &x, Expr const &y) {
    print("\x1b[");
    print(y + Coord{Top});
    print(';');
    print(x + Coord{Left});
    print('H');
  }


  template <size_t Width, size_t Height, size_t Left, size_t Top>
  void Screen<Width, Height, Left, Top>::Put::emit(Expr const &x, Expr const &y,
                                                   Expr const &character) {
    MoveTo::emit(x, y);
    sugar::put(character);
  }


  template <size_t Width, size_t Height, size_t Left, size_t Top>
  template <concepts::String StringType>
  void Screen<Width, Height, Left, Top>::Write<StringType>::emit(Expr const &x, Expr const &y,
								 Expr const &text) {
    MoveTo::emit(x, y);
    print(text);
  }


  template <size_t Width, size_t Height, size_t Left, size_t Top>
  void Screen<Width, Height, Left, Top>::Clear::emit() {
    static std::string const blankLine(Width, ' ');

    for (size_t y = 0; y < Height; ++y) {
      MoveTo::emit(Coord{0}, Coord{y});
      print(blankLine);
    }

    MoveTo::emit(Coord{0}, Coord{0});
  }


  template <size_t Width, size_t Height, size_t Left, size_t Top>
  void Screen<Width, Height, Left, Top>::Begin::emit() {
    print("\x1b[?25l");
  }


  template <size_t Width, size_t Height, size_t Left, size_t Top>
  void Screen<Width, Height, Left, Top>::End::emit() {
    MoveTo::emit(Coord{0}, Coord{Height});
    print("\x1b[?25h");
  }

} // namespace acus::sugar::impl
