#pragma once

namespace acus::sugar::impl {

  template <size_t Width, size_t Height, size_t Left, size_t Top>
  struct Screen {

    using Coord = u8;
    using Char  = u8;
    static_assert(Width  > 0);
    static_assert(Height > 0);

    /*
     * Move to a zero-based coordinate inside the screen.
     *
     * Sugar: moveTo(x, y)
     * ANSI:  ESC[Top+y;Left+xH
     */

    struct MoveTo: LibraryFunction<MoveTo, void(Coord, Coord)> {

      static void emit(Expr const &x, Expr const &y) {
	io::print("\x1b[");
	io::print(y + Coord{Top});
	io::print(';');
	io::print(x + Coord{Left});
	io::print('H');
      }
    };

    inline static constexpr MoveTo moveTo{};


    /*
     * Place one character without moving back afterward.
     */
    
    struct Put: LibraryFunction<Put, void(Coord, Coord, Char)> {

      static void emit(Expr const &x, Expr const &y, Expr const &character) {
        MoveTo::emit(x, y);
	io::write(character);
      }
    };

    inline static constexpr Put put{};


    /*
     * Write a fixed-size string starting at x, y.
     */
    template <size_t N>
    struct Write: LibraryFunction<  Write<N>, void(Coord, Coord, string<N>)> {

      static void emit(Expr const &x, Expr const &y, Expr const &text) {
        MoveTo::emit(x, y);
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

      static void emit() {
        static std::string const blankLine(Width, ' ');

        for (size_t y = 0; y < Height; ++y) {
          MoveTo::emit(Coord{0}, Coord{y});
	  io::print(blankLine);
        }

        MoveTo::emit(Coord{0}, Coord{0});
      }
    };

    inline static constexpr Clear clear{};


    /*
     * Hide the terminal cursor while drawing.
     */
    struct Begin: LibraryFunction<Begin, void()> {
      static void emit() {
	io::print("\x1b[?25l");
      }
    };

    inline static constexpr Begin begin{};


    /*
     * Restore the cursor and place it below the screen.
     */
    struct End: LibraryFunction<End, void()> {
      
      static void emit() {
        MoveTo::emit(Coord{0}, Coord{Height});
	io::print("\x1b[?25h");
      }
    };

    inline static constexpr End end{};

  }; // Screen

} // namespace impl
