// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar.h"
using namespace acus;
using namespace sugar;


int main() try {

  program("test");
  {
    using Point = Struct<"Point",
      Field<"x", u8>,
      Field<"y", u8>
      >;

    using Vec2 = Array<u8, 2>;

    
    auto hello = function_fwd<void(string<10>)>("hello");
    auto foo = function_fwd<void(s8)>("foo");
    
    function<void()>("main");
    {
      for_(let<u8>("i") = 0, var("i") < 10, ++var("i"), {
	  hello("you");
	})

      let<u8>("x") = 5;
      println(var("x"));
      if_(var("x") < 10, {
	  println("small");
	},{
	  println("big");
	})

      var("x") += 30;
      println(var("x"));
      if_(var("x") < 10, {
	  println("small");
	})

      let<Point>("p") = Point{'Q','Z'};
      var("p").field("x") = 'X';
      var("p").field("y") = 'Y';
      println(var("p"));

      foo(s8{-200});

      let<Vec2>("v") = Vec2{'X', s8{'Y'}};
      println(var("v"));

      let<u8>("i") = 0;
      while_(var("i") < 5, {
	  println(var("i"));
	  ++var("i");
	});
      
      return_();
    }
    endFunction();

    function< void(string<10>) >("hello", "str");
    {
      print("Hello, ");
      println(var("str"));
      return_();
    }
    endFunction();

    function< void(s8) >("foo", "x");
    {
      println(var("x"));
      return_();
    }
    endFunction();
    
  }
  endProgram();


  program("fibonacci");
  {
    function<void()>("main");
    {
      let<u8>("N") = 20;

      let<u16>("a") = 0;
      let<u16>("b") = 1;
      
      for_(let<u8>("i") = 0, var("i") < var("N"), ++var("i"), {
	  println(var("a"));

	  let<u16>("next") = var("a") + var("b");
	  var("a") = var("b");
	  var("b") = var("next");
	});

      return_();
    }
    endFunction();
  }
  endProgram();


  program("fibonacci_recursive");
  {
    auto fib = function_fwd<void(u16, u16, u16)>("fib");

    function<void()>("main");
    {
      let<u8>("N") = 20;
      fib(var("N"), 0, 1);
      return_();
    }
    endFunction();

    function<void(u16, u16, u16)>("fib", "n", "a", "b");
    {
      if_(var("n") < 1, {
	  return_();
	},{
	  println(var("a"));

	  fib(var("n") - 1, var("b"), var("a") + var("b"));
	  return_();
	});
    }
    endFunction();
  }
  endProgram();
 
  program("fibonacci_recursive2");
  {
    auto fib = function_fwd<u16(u16)>("fib");

    function<void()>("main");
    {
      let<u16>("N") = 20;

      for_(let<u8>("i") = 0, var("i") < var("N"), ++var("i"), {
	  println(fib(var("i")));
	});

      return_();
    }
    endFunction();

    function<u16(u16)>("fib", "n");
    {
      if_(var("n") < 2, {
	  return_(var("n"));
	},{
	  return_(fib(var("n") - 1) + fib(var("n") - 2));
	});
    }
    endFunction();

  }
  endProgram();
  std::cout << generateBrainfuck("fibonacci");
  
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
