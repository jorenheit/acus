// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"
#include "acus/sugar/std.h"
using namespace acus;
using namespace sugar;
using namespace sugar::io;


int main() try {

  program("test");
  {
    using Point = Struct<"Point",
      Field<"x", u8>,
      Field<"y", u8>
      >;

    using Vec2 = Array<u8, 2>;

    
    auto hello = function_<void(string<10>)>("hello") | declare;
    auto foo = function_<void(s8)>("foo") | declare;
    
    function_<void()>("main") | define {
      for_(let<u8>("i") = 0, var("i") < 10, ++var("i")) {
	hello("you");
      };
      
      let<u8>("x") = 5;
      println(-var("x"));
      if_(var("x") < 10) {
	println("small");
      } else_ {
	println("big");
      };

      var("x") += 30;
      println(var("x"));
      if_(var("x") < 10) {
	println("small");
      };

      let<Point>("p") = Point{'Q','Z'};
      var("p").field("x") = 'X';
      var("p").field("y") = 'Y';
      println(var("p"));

      foo(s8{-200});

      let<Vec2>("v") = Vec2{'X', s8{'Y'}};
      println(var("v"));

      let<u8>("i") = 0;
      while_(var("i") < 10) {
	println(var("i"));
	if_(var("i") == 7) {
	  break_;
	};
	++var("i");
      };
      
      return_;
    };

    function_< void(string<10>) >("hello", "str") | define {
      print("Hello, ");
      println(var("str"));
      return_;
    };

    function_< void(s8) >("foo", "x") | define {
      println(var("x"));
      return_;
    };
    
  }
  endProgram();


  program("fibonacci");
  {
    function_<void()>("main") | define {

      let<u8>("N") = 10;

      let<u16>("a") = 0;
      let<u16>("b") = 1;

      for_(let<u8>("i") = 0, var("i") < var("N"), ++var("i")) {
	println(var("a"));

	let<u16>("next") = var("a") + var("b");
	var("a") = var("b");
	var("b") = var("next");
      };

      return_;
    };
  }
  endProgram();


  program("fibonacci_recursive");
  {
    auto fib = function_<void(u16, u16, u16)>("fib") | declare;

    function_<void()>("main") | define {
      let<u8>("N") = 20;
      fib(var("N"), 0, 1);
      return_;
    };

    function_<void(u16, u16, u16)>("fib", "n", "a", "b") | define {
      if_(var("n") < 1) {
	return_;
      } else_ {
	println(var("a"));
	fib(var("n") - 1, var("b"), var("a") + var("b"));
	return_;
      };
    };
  }
  endProgram();
 
  program("fibonacci_recursive2");
  {
    auto fib = function_<u16(u16)>("fib") | declare;

    function_<void()>("main") | define {
      let<u16>("N") = 20;

      for_(let<u8>("i") = 0, var("i") < var("N"), ++var("i")) {
	println(fib(var("i")));
      };

      return_;
    };

    function_<u16(u16)>("fib", "n") | define {
      if_(var("n") < 2) {
	return_(var("n"));
      } else_ {
	return_(fib(var("n") - 1) + fib(var("n") - 2));
      };
    };
  }
  endProgram();


  program("break_continue_demo");
  {
    function_<void()>("main") | define {
      println("Numbers from 1 to 20");
      println("skip multiples of 3");
      println("stop at 17");
      println("");

      for_(let<u8>("i") = 1, var("i") <= 20, ++var("i")) {
        // Stop the loop entirely once i == 17.
        if_(var("i") == 17) {
          println("Reached 17, stopping.");
          break_;
        };

        // Skip multiples of 3.
        if_(var("i") % 3 == 0) {
          continue_;
        };

        println(var("i"));
      };

      println("");
      println("Done.");

      return_;
    };
  }
  endProgram();

  program("nested_loops");
  {
    function_<void()>("main") | define {
      for_(let<u8>("i") = 1, var("i") <= 10, ++var("i")) {
	for_(let<u8>("j") = var("i"), var("j") <= 10, ++var("j")) {
	  if_(var("i") * var("j") > 50) {
	    continue_;
	  };
	  print('(');
	  print(var("i"));
	  print(',');
	  print(var("j"));
	  println(')');
	};
      };
      return_;
    };
  }
  endProgram();


  
  program("pointers");
  {
    using Arr5 = Array<u8, 5>;
    auto foo = function_<void(ptr<u8>)>("foo") | declare;
    
    function_<void()>("main") | define {
      let<Arr5>("arr") = Arr5{1, 2, 3, 4, 5};
      foo(&var("arr")[0]);
      
      
      let<ptr<u8>>("p") = &var("arr")[0];
      for_(let<u8>("i") = 0, var("i") < 5, ++var("i")) {
	println(*var("p"));
	++var("p");
      };

      return_;
    };

    function_<void(ptr<u8>)>("foo", "p") | define {
      *var("p") = 69;
      return_;
    };
  }
  endProgram();

  program("globals");
  {
    global<string<10>>("g");

    auto foo = function_<void(string<10>)>("foo", "str") | define {
      var("g") = var("str");
      return_;
    };
    
    function_<void()>("main") | define {      
      var("g") = "Hello, ";

      print(var("g"));
      foo("World!");
      println(var("g"));

      return_;
    };

  }
  endProgram();

  program("io");
  {

    function_<void()>("main") | define {
      print("Enter your name: ");

      auto name = let<string<20>>("name") = io::readLine<10>();
		      
      print("Hello, ");
      print(name);
      println("!");
      
      return_;
    };
    
  }
  endProgram();

  program("echo");
  {
    auto readLine = io::readLine<20>.outline();
    auto strlen = io::strlen<u8>.outline();
    
    function_<void()>("main") | define {
      for (int i = 0; i != 10; ++i) {
	//auto line = readLine();
		auto line = io::readLine<20>();
	println(line);
	//println(strlen(line));
	println(io::strlen<>(line));
      }
      
      return_;
    };
    
  }
  endProgram();
  
  
  std::cout << generateBrainfuck("echo");
  
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
