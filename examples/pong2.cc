#include <iostream>

#include <acus/sugar/sugar.h>
#include <acus/sugar/sugar_std.h>

using namespace acus::sugar;
using namespace acus::sugar::io;

namespace {

constexpr size_t ScreenWidth  = 40;
constexpr size_t ScreenHeight = 20;
constexpr size_t PaddleHeight = 4;

constexpr size_t LeftPaddleX  = 2;
constexpr size_t RightPaddleX = ScreenWidth - 3;

constexpr size_t MinPaddleY = 1;
constexpr size_t MaxPaddleY = ScreenHeight - PaddleHeight - 1;

constexpr size_t InitialPaddleY = ScreenHeight / 2 - PaddleHeight / 2;
constexpr size_t InitialBallX   = ScreenWidth / 2;
constexpr size_t InitialBallY   = ScreenHeight / 2;

constexpr size_t BallDelay = 16;

using Screen = ansi::Screen<ScreenWidth, ScreenHeight>;

using GameState = Struct<"GameState",
  Field<"leftY", u8>,
  Field<"rightY", u8>,
  Field<"ballX", u8>,
  Field<"ballY", u8>,
  Field<"ballRight", u8>,
  Field<"ballDown", u8>,
  Field<"running", u8>,
  Field<"tick", u8>
>;

using GameStatePtr = ptr<GameState>;

} // namespace


int main() try {
  program_("pong");
  {
    auto drawPaddle =
      function_<void(u8, u8, u8)>("drawPaddle", "x", "y", "glyph") | define {
        auto x     = var_("x");
        auto y     = var_("y");
        auto glyph = var_("glyph");

        Screen::put(x, y, glyph);
        Screen::put(x, y + 1, glyph);
        Screen::put(x, y + 2, glyph);
        Screen::put(x, y + 3, glyph);

        return_;
      };


    auto drawBorder = function_<void()>("drawBorder") | define {
      for (size_t x = 0; x < ScreenWidth; ++x) {
        Screen::put(x, 0, '-');
        Screen::put(x, ScreenHeight - 1, '-');
      }

      for (size_t y = 1; y < ScreenHeight - 1; ++y) {
        Screen::put(0, y, '|');
        Screen::put(ScreenWidth - 1, y, '|');
      }

      Screen::put(0, 0, '+');
      Screen::put(ScreenWidth - 1, 0, '+');
      Screen::put(0, ScreenHeight - 1, '+');
      Screen::put(ScreenWidth - 1, ScreenHeight - 1, '+');

      return_;
    };


    auto resetBall =
      function_<void(GameStatePtr, u8, u8)>("resetBall", "state", "right", "down") | define {
        auto state = *var_("state");

        auto ballX     = state.field("ballX");
        auto ballY     = state.field("ballY");
        auto ballRight = state.field("ballRight");
        auto ballDown  = state.field("ballDown");

        ballX     = InitialBallX;
        ballY     = InitialBallY;
        ballRight = var_("right");
        ballDown  = var_("down");

        return_;
      };


    auto handleInput =
      function_<void(GameStatePtr, u8)>("handleInput", "state", "key") | define {
        auto state = *var_("state");
        auto key   = var_("key");

        auto leftY   = state.field("leftY");
        auto rightY  = state.field("rightY");
        auto running = state.field("running");

        if_(key == 'q') {
          running = 0;
        };

        if_(key == 'w') {
          if_(leftY > MinPaddleY) {
            drawPaddle(LeftPaddleX, leftY, ' ');
            --leftY;
            drawPaddle(LeftPaddleX, leftY, '#');
          };
        };

        if_(key == 's') {
          if_(leftY < MaxPaddleY) {
            drawPaddle(LeftPaddleX, leftY, ' ');
            ++leftY;
            drawPaddle(LeftPaddleX, leftY, '#');
          };
        };

        if_(key == 'o') {
          if_(rightY > MinPaddleY) {
            drawPaddle(RightPaddleX, rightY, ' ');
            --rightY;
            drawPaddle(RightPaddleX, rightY, '#');
          };
        };

        if_(key == 'l') {
          if_(rightY < MaxPaddleY) {
            drawPaddle(RightPaddleX, rightY, ' ');
            ++rightY;
            drawPaddle(RightPaddleX, rightY, '#');
          };
        };

        return_;
      };


    auto stepBall =
      function_<void(GameStatePtr)>("stepBall", "state") | define {
        auto state = *var_("state");

        auto leftY      = state.field("leftY");
        auto rightY     = state.field("rightY");
        auto ballX      = state.field("ballX");
        auto ballY      = state.field("ballY");
        auto movingRight = state.field("ballRight");
        auto movingDown  = state.field("ballDown");

        Screen::put(ballX, ballY, ' ');

        if_(movingDown) {
          if_(ballY == ScreenHeight - 2) {
            movingDown = 0;
            --ballY;
          }
          else_ {
            ++ballY;
          };
        }
        else_ {
          if_(ballY == 1) {
            movingDown = 1;
            ++ballY;
          }
          else_ {
            --ballY;
          };
        };

        if_(movingRight) {
          if_(ballX == RightPaddleX - 1) {
            if_(ballY >= rightY && ballY < rightY + PaddleHeight) {
              movingRight = 0;
              --ballX;
            }
            else_ {
              resetBall(var_("state"), 0, 1);
            };
          }
          else_ {
            ++ballX;
          };
        }
        else_ {
          if_(ballX == LeftPaddleX + 1) {
            if_(ballY >= leftY && ballY < leftY + PaddleHeight) {
              movingRight = 1;
              ++ballX;
            }
            else_ {
              resetBall(var_("state"), 1, 0);
            };
          }
          else_ {
            --ballX;
          };
        };

        Screen::put(ballX, ballY, 'O');
        return_;
      };


    function_<void()>("main") | define {
      auto state = let_<GameState>("state");
      auto statePtr = (let_<GameStatePtr>("statePtr") = &state);      

      auto leftY   = state.field("leftY");
      auto rightY  = state.field("rightY");
      auto ballX   = state.field("ballX");
      auto ballY   = state.field("ballY");
      auto running = state.field("running");
      auto tick    = state.field("tick");

      leftY   = InitialPaddleY;
      rightY  = InitialPaddleY;
      running = 1;
      tick    = 0;

      resetBall(statePtr, 1, 1);

      Screen::clear();
      drawBorder();

      drawPaddle(LeftPaddleX, leftY, '#');
      drawPaddle(RightPaddleX, rightY, '#');
      Screen::put(ballX, ballY, 'O');

      auto key = let_<u8>("key");

      while_(running) {
        read(key);
        handleInput(statePtr, key);

        ++tick;

        if_(tick == BallDelay) {
          tick = 0;
          stepBall(statePtr);
        };
      };

      Screen::clear();
      return_;
    };
  }
  endProgram();

  std::cout << generateBrainfuck("pong");
}
catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
