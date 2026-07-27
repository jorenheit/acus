// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"
#include "acus/sugar/sugar_std.h"
using namespace acus;
using namespace sugar;
using namespace sugar::io;


int main() try {

  program_("test");
  {
    using Point = Struct<"Point",
      Field<"x", u8>,
      Field<"y", u8>
      >;

    using Vec2 = Array<u8, 2>;

    
    auto hello = function_<void(string<10>)>("hello") | declare;
    auto foo = function_<void(s8)>("foo") | declare;
    
    function_<void()>("main") | define {
      for_(let_<u8>("i") = 0, var_("i") < 10, ++var_("i")) {
	hello("you");
      };
      
      let_<u8>("x") = 5;
      println(-var_("x"));
      if_(var_("x") < 10) {
	println("small");
      } else_ {
	println("big");
      };

      var_("x") += 30;
      println(var_("x"));
      if_(var_("x") < 10) {
	println("small");
      };

      let_<Point>("p") = Point{'Q','Z'};
      var_("p").field("x") = 'X';
      var_("p").field("y") = 'Y';
      println(var_("p"));

      foo(s8{-200});

      let_<s8>("z") = -1;
      //      let_<Vec2>("v") = Vec2{'X', var_("z")};
      let_<Vec2>("v") = Vec2{'X', 'Y'};
      println(var_("v"));

      let_<u8>("i") = 0;
      while_(var_("i") < 10) {
	println(var_("i"));
	if_(var_("i") == 7) {
	  break_;
	};
	++var_("i");
      };
      
      return_;
    };

    function_< void(string<10>) >("hello", "str") | define {
      print("Hello, ");
      println(var_("str"));
      return_;
    };

    function_< void(s8) >("foo", "x") | define {
      println(var_("x"));
      return_;
    };
    
  }
  endProgram();


  program_("fibonacci");
  {
    function_<void()>("main") | define {

      let_<u8>("N") = 10;

      let_<u16>("a") = 0;
      let_<u16>("b") = 1;

      for_(let_<u8>("i") = 0, var_("i") < var_("N"), ++var_("i")) {
	println(var_("a"));

	let_<u16>("next") = var_("a") + var_("b");
	var_("a") = var_("b");
	var_("b") = var_("next");
      };

      return_;
    };
  }
  endProgram();


  program_("fibonacci_recursive");
  {
    auto fib = function_<void(u16, u16, u16)>("fib") | declare;

    function_<void()>("main") | define {
      let_<u8>("N") = 20;
      fib(var_("N"), 0, 1);
      return_;
    };

    function_<void(u16, u16, u16)>("fib", "n", "a", "b") | define {
      if_(var_("n") < 1) {
	return_;
      } else_ {
	println(var_("a"));
	fib(var_("n") - 1, var_("b"), var_("a") + var_("b"));
	return_;
      };
    };
  }
  endProgram();
 
  program_("fibonacci_recursive2");
  {
    auto fib = function_<u16(u16)>("fib") | declare;

    function_<void()>("main") | define {
      let_<u16>("N") = 20;

      for_(let_<u8>("i") = 0, var_("i") < var_("N"), ++var_("i")) {
	println(fib(var_("i")));
      };

      return_;
    };

    function_<u16(u16)>("fib", "n") | define {
      if_(var_("n") < 2) {
	return_(var_("n"));
      } else_ {
	return_(fib(var_("n") - 1) + fib(var_("n") - 2));
      };
    };
  }
  endProgram();


  program_("break_continue_demo");
  {
    function_<void()>("main") | define {
      println("Numbers from 1 to 20");
      println("skip multiples of 3");
      println("stop at 17");
      println("");

      for_(let_<u8>("i") = 1, var_("i") <= 20, ++var_("i")) {
        // Stop the loop entirely once i == 17.
        if_(var_("i") == 17) {
          println("Reached 17, stopping.");
          break_;
        };

        // Skip multiples of 3.
        if_(var_("i") % 3 == 0) {
          continue_;
        };

        println(var_("i"));
      };

      println("");
      println("Done.");

      return_;
    };
  }
  endProgram();

  program_("nested_loops");
  {
    function_<void()>("main") | define {
      for_(let_<u8>("i") = 1, var_("i") <= 10, ++var_("i")) {
	for_(let_<u8>("j") = var_("i"), var_("j") <= 10, ++var_("j")) {
	  if_(var_("i") * var_("j") > 50) {
	    continue_;
	  };
	  print('(');
	  print(var_("i"));
	  print(',');
	  print(var_("j"));
	  println(')');
	};
      };
      return_;
    };
  }
  endProgram();


  
  program_("pointers");
  {
    using Arr5 = Array<u8, 5>;
    auto foo = function_<void(ptr<u8>)>("foo") | declare;
    
    function_<void()>("main") | define {
      let_<Arr5>("arr") = Arr5{1, 2, 3, 4, 5};
      foo(&var_("arr")[0]);
      
      
      let_<ptr<u8>>("p") = &var_("arr")[0];
      for_(let_<u8>("i") = 0, var_("i") < 5, ++var_("i")) {
	println(*var_("p"));
	++var_("p");
      };

      return_;
    };

    function_<void(ptr<u8>)>("foo", "p") | define {
      *var_("p") = 69;
      return_;
    };
  }
  endProgram();

  program_("globals");
  {
    global_<string<10>>("g");

    auto foo = function_<void(string<10>)>("foo", "str") | define {
      var_("g") = var_("str");
      return_;
    };
    
    function_<void()>("main") | define {      
      var_("g") = "Hello, ";

      print(var_("g"));
      foo("World!");
      println(var_("g"));

      return_;
    };

  }
  endProgram();

  program_("io");
  {

    function_<void()>("main") | define {
      print("Enter your name: ");

      auto name = let_<string<20>>("name") = io::readLine<10>();
		      
      print("Hello, ");
      print(name);
      println("!");
      
      return_;
    };
    
  }
  endProgram();

  program_("echo");
  {
    auto readLine = io::readLine<20>.outline();
    auto strlen = strings::strlen<u8, 20>.outline();
    
    function_<void()>("main") | define {
      let_<u8>("x") = 1;
      println("Hello?");
      auto line = io::readLine<20>();
      println(line);
      println(strlen(line));
      return_;
    };
    
  }
  endProgram();


  program_("str2int");
  {
    auto readLine = io::readLine<20>.outline();
    auto str2int = io::parseInt<s16, 20>.outline();
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = readLine();
      let_<s16>("val") = str2int(line);
      println(2 * var_("val"));
      return_;
    };
    
  }
  endProgram();

  program_("pow");
  {
    auto readLine = io::readLine<20>.outline();
    auto str2int = io::parseInt<s16, 20>.outline();
    auto pow = math::pow<s16>;
    
    function_<void()>("main") | define {
      print("x = ");
      auto line1 = readLine();
      let_<s16>("x") = str2int(line1);

      print("p = ");
      auto line2 = readLine();
      let_<s16>("p") = str2int(line2);
      
      println(pow(var_("x"), var_("p")));
      return_;
    };
    
  }
  endProgram();

  
  program_("sqrt");
  {
    auto readLine = io::readLine<20>.outline();
    auto str2int = io::parseInt<s16, 20>.outline();
    auto sqrt = math::sqrt<s16>;
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = readLine();
      let_<s16>("val") = str2int(line);
      println(sqrt(var_("val")));
      return_;
    };
    
  }
  endProgram();


  program_("log");
  {
    auto readLine = io::readLine<20>.outline();
    auto str2int = io::parseInt<u16, 20>.outline();
    auto log2 = math::log2<u16>;
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = readLine();
      let_<u16>("val") = str2int(line);
      println(log2(var_("val")));
      return_;
    };
    
  }
  endProgram();

  program_("minmax");
  
  {
    auto readLine = io::readLine<20>.outline();
    auto str2int = io::parseInt<u16, 20>.outline();
    auto min = math::min<u16>;
    auto max = math::max<u16>;
    
    function_<void()>("main") | define {
      print("x = ");
      auto line1 = readLine();
      let_<u16>("val1") = str2int(line1);

      print("y = ");
      auto line2 = readLine();
      let_<u16>("val2") = str2int(line2);

      print("Min: ");
      println(min(var_("val1"), var_("val2")));
      print("Max: ");
      println(max(var_("val1"), var_("val2")));
      return_;
    };
    
  }
  endProgram();


  program_("isDigit");
  {
    auto readLine = io::readLine<20>.outline();
    auto isDigit = ascii::isDigit;
    
    function_<void()>("main") | define {
      print("Enter a character: ");
      auto line1 = readLine();

      if_(isDigit(line1[0])) {
	println("This is a digit.");
      } else_ {
	println("This is not a digit.");
      };
      
      return_;
    };
    
  }
  endProgram();

  program_("isAlpha");
  {
    auto readLine = io::readLine<20>.outline();
    auto isAlpha = ascii::isAlpha;
    
    function_<void()>("main") | define {
      print("Enter a character: ");
      auto line = readLine();

      if_(isAlpha(line[0])) {
	println("This is a letter.");
      } else_ {
	println("This is not a letter.");
      };
      
      return_;
    };
    
  }
  endProgram();


  program_("upperlower");
  {
    auto readLine = io::readLine<20>.outline();
    auto toUpper = ascii::toUpper.outline();
    auto toLower = ascii::toLower.outline();
    auto strlen = strings::strlen<u8, 20>.outline();
    
    main_() {
      print("> ");

      auto line = (let_<string<20>>("line") = readLine());
      auto len = let_<u8>("len") = strlen(line);
      for_(let_<u8>("i") = 0, var_("i") != len, ++var_("i")) {
	write(toLower(line[var_("i")]));
      };
      
      println();
      return_;
    };
    
  }
  endProgram();


  program_("strings");
  {
    auto readLine = io::readLine<20>.outline();
    auto clear = strings::clear<20>;
    auto fun = strings::find_char<20>;
    
    main_() {
      print("String 1: ");
      auto str1 = (let_<string<20>>("str1") = readLine());

      print("String 2: ");
      auto str2 = (let_<string<20>>("str2") = readLine());

      io::println(fun(str1, str2));
      return_;
    };
    
  }
  endProgram();
  
  
  program_("isAlnum");
  {
    auto readLine = io::readLine<20>.outline();
    auto isAlnum = ascii::isAlphanumeric.outline();
    
    main_() {
      print("Enter a character: ");
      auto line1 = readLine();

      if_(isAlnum(line1[0])) {
	println("This is alnum.");
      } else_ {
	println("This is not alnum.");
      };
      
      return_;
    };
    
  }
  endProgram();

  program_("screen");
  {
    using Screen = ansi::Screen<50, 50>;
    auto readLine = io::readLine<5>;
    
    function_<void()>("main") | define {

      print("Enter a char: ");
      let_<u8>("ch") = readLine()[0];
      
      auto x = let_<u8>("x");
      auto y = let_<u8>("y");

      while_(1) {
	y = 2;
	while_(y < 30) {
	  x = 5;
	  while_(x < 30) {
	    Screen::put(x, y, var_("ch"));
	    ++x;
	  };
	  ++y;
	};
	
	Screen::clear();
      };
      
      println();
	
      return_;
    };
    
  }
  endProgram();
  
  
  std::cout << generateBrainfuck("strings");
  
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
