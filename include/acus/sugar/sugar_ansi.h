#pragma once

namespace acus::sugar::impl {

  template <size_t Width, size_t Height, size_t Left, size_t Top>
  struct Screen {

    using CoordType = u8;
    static_assert(Width  > 0);
    static_assert(Height > 0);

    /*
     * Move to a zero-based coordinate inside the screen.
     *
     * Sugar: moveTo(x, y)
     * ANSI:  ESC[Top+y;Left+xH
     */

    struct MoveTo: LibraryFunction<MoveTo, void(CoordType, CoordType)> {

      static void emit(Expr const &x, Expr const &y, SUGAR_LOC) {
	io::print("\x1b[");
	io::print(y + CoordType{Top});
	io::print(';');
	io::print(x + CoordType{Left});
	io::print('H');
      }
    };

    inline static constexpr MoveTo moveTo{};


    /*
     * Place one character without moving back afterward.
     */
    
    struct Put: LibraryFunction<Put, void(CoordType, CoordType, u8)> {

      static void emit(Expr const &x, Expr const &y, Expr const &character, SUGAR_LOC) {
        MoveTo::emit(x, y, LOC_FWD);
	io::write(character);
      }
    };

    inline static constexpr Put put{};


    /*
     * Write a fixed-size string starting at x, y.
     */
    template <size_t N>
    struct Write: LibraryFunction<  Write<N>, void(CoordType, CoordType, string<N>)> {

      static void emit(Expr const &x, Expr const &y, Expr const &text, SUGAR_LOC) {
        MoveTo::emit(x, y, LOC_FWD);
	io::print(text);
      }
      
    };

    template <size_t N>
    inline static constexpr Write<N> write{};


    /*
     * Clear only this screen region, rather than the whole terminal.
     *
     * The C++ loops execute while constructing the Acus program.
     */
    struct Clear: LibraryFunction<Clear, void()> {

      static void emit(SUGAR_LOC) {
        static std::string const blankLine(Width, ' ');

        for (size_t y = 0; y < Height; ++y) {
          MoveTo::emit(CoordType{0}, CoordType{y}, LOC_FWD);
	  io::print(blankLine);
        }

        MoveTo::emit(CoordType{0}, CoordType{0}, LOC_FWD);
      }
    };

    inline static constexpr Clear clear{};


    /*
     * Hide the terminal cursor while drawing.
     */
    struct Begin: LibraryFunction<Begin, void()> {
      static void emit(SUGAR_LOC) {
	io::print("\x1b[?25l");
      }
    };

    inline static constexpr Begin begin{};


    /*
     * Restore the cursor and place it below the screen.
     */
    struct End: LibraryFunction<End, void()> {
      
      static void emit(SUGAR_LOC) {
        MoveTo::emit(
          CoordType{0},
          CoordType{Height},
          LOC_FWD
        );

	io::print("\x1b[?25h");
      }
    };

    inline static constexpr End end{};

  }; // Screen

} // namespace impl
