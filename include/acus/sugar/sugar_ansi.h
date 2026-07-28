#pragma once

namespace acus::sugar::impl {

  template <size_t Width, size_t Height, size_t Left, size_t Top>
  struct Screen {

    using Coord = u8;
    using Char = u8;

    static_assert(Width > 0);
    static_assert(Height > 0);

    struct MoveTo: LibraryFunction<MoveTo, void(Coord, Coord)> {
      static void emit(Expr const &x, Expr const &y);
    };

    inline static constexpr MoveTo move_to{};


    struct Put: LibraryFunction<Put, void(Coord, Coord, Char)> {
      static void emit(Expr const &x, Expr const &y, Expr const &character);
    };

    inline static constexpr Put put{};


    template <size_t N>
    struct Write: LibraryFunction<Write<N>, void(Coord, Coord, String<N>)> {
      static void emit(Expr const &x, Expr const &y, Expr const &text);
    };

    template <size_t N>
    inline static constexpr Write<N> write{};


    struct Clear: LibraryFunction<Clear, void()> {
      static void emit();
    };

    inline static constexpr Clear clear{};


    struct Begin: LibraryFunction<Begin, void()> {
      static void emit();
    };

    inline static constexpr Begin begin{};


    struct End: LibraryFunction<End, void()> {
      static void emit();
    };

    inline static constexpr End end{};

  }; // Screen

} // namespace acus::sugar::impl
