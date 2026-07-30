// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "acus/assembler/assembler.h"

#define ACUS_SUGAR_LOOP_UNROLL_LIMIT 10
#include "acus/sugar/sugar.h"

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

    
    auto hello = function_<void(String<10>)>("hello") | declare;
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
      put(var_("p"));
      println();

      foo(s8{-200});

      let_<s8>("z") = -1;
      let_<Vec2>("v") = Vec2{'X', 'Y'};
      put(var_("v"));
      println();

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

    function_< void(String<10>) >("hello", "str") | define {
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
    auto foo = function_<void(Ptr<u8>)>("foo") | declare;
    
    function_<void()>("main") | define {
      let_<Arr5>("arr") = Arr5{1, 2, 3, 4, 5};
      foo(&var_("arr")[0]);
      
      
      let_<Ptr<u8>>("p") = &var_("arr")[0];
      for_(let_<u8>("i") = 0, var_("i") < 5, ++var_("i")) {
	println(*var_("p"));
	++var_("p");
      };

      return_;
    };

    function_<void(Ptr<u8>)>("foo", "p") | define {
      *var_("p") = 69;
      return_;
    };
  }
  endProgram();

  program_("globals");
  {
    global_<String<10>>("g");

    auto foo = function_<void(String<10>)>("foo", "str") | define {
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

    using MyString = String<10>;
    function_<void()>("main") | define {
      print("Enter your name: ");

      auto name = let_<String<20>>("name") = io::read_line<MyString>();
		      
      print("Hello, ");
      print(name);
      println("!");
      
      return_;
    };
    
  }
  endProgram();

  program_("echo");
  {
    using MyString = String<20>;
    
    auto read_line = io::read_line<MyString>.outline();
    auto strlen = MyString::length.outline();
    
    function_<void()>("main") | define {
      let_<u8>("x") = 1;
      println("Hello?");
      auto line = read_line();
      println(line);
      println(strlen(line));
      return_;
    };
    
  }
  endProgram();


  program_("str2int");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    
    main_() {
      print("Enter a number: ");
      auto line = read_line();
      let_<u16>("val") = str2int(line);
      println(var_("val"));
      println(MyString::to_int<u8>("123"));      
      return_;
    };
    
  }
  endProgram();

  program_("int2str");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto int2str   = string::from_int<u16>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    
    main_() {
      print("Enter a number: ");
      auto line = read_line();
      let_<u16>("val") = str2int(line);
      println(var_("val"));
      println(int2str(var_("val")));
      return_;
    };
    
  }
  endProgram();

  
  program_("pow");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<s16>.outline();
    auto pow = math::pow<s16>;
    
    function_<void()>("main") | define {
      print("x = ");
      auto line1 = read_line();
      let_<s16>("x") = str2int(line1);

      print("p = ");
      auto line2 = read_line();
      let_<s16>("p") = str2int(line2);
      
      println(pow(var_("x"), var_("p")));
      return_;
    };
    
  }
  endProgram();


  program_("abs");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<s16>.outline();
    auto abs = math::abs<s16>;
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = read_line();
      let_<s16>("val") = str2int(line);
      println(abs(var_("val")));
      return_;
    };
    
  }
  endProgram();
  
  program_("sqrt");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<s16>.outline();
    auto sqrt = math::sqrt<s16>;
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = read_line();
      let_<s16>("val") = str2int(line);
      println(sqrt(var_("val")));
      return_;
    };
    
  }
  endProgram();


  program_("log");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto log2 = math::log2<u16>;
    
    function_<void()>("main") | define {
      print("Enter a number: ");
      auto line = read_line();
      let_<u16>("val") = str2int(line);
      println(log2(var_("val")));
      return_;
    };
    
  }
  endProgram();

  program_("minmax");
  
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto min = math::min<u16>;
    auto max = math::max<u16>;
    
    function_<void()>("main") | define {
      print("x = ");
      auto line1 = read_line();
      let_<u16>("val1") = str2int(line1);

      print("y = ");
      auto line2 = read_line();
      let_<u16>("val2") = str2int(line2);

      print("Min: ");
      println(min(var_("val1"), var_("val2")));
      print("Max: ");
      println(max(var_("val1"), var_("val2")));
      return_;
    };
    
  }
  endProgram();

  program_("clamp");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto clamp = math::clamp<u16>;
    
    function_<void()>("main") | define {
      print("x = ");
      let_<u16>("x") = str2int(read_line());

      print("min = ");
      let_<u16>("min") = str2int(read_line());

      print("max = ");
      let_<u16>("max") = str2int(read_line());
      
      print("Result:  ");
      println(clamp(var_("x"), var_("min"), var_("max")));
      return_;
    };
    
  }
  endProgram();
  

  program_("gcd");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto gcd = math::gcd<u16>;
    
    function_<void()>("main") | define {
      print("a = ");
      auto line1 = read_line();
      let_<u16>("a") = str2int(line1);

      print("b = ");
      auto line2 = read_line();
      let_<u16>("b") = str2int(line2);

      print("gcd =  ");
      println(gcd(var_("a"), var_("b")));
      return_;
    };
    
  }
  endProgram();
  

  program_("isDigit");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto is_digit = ascii::is_digit;
    
    function_<void()>("main") | define {
      print("Enter a character: ");
      auto line1 = read_line();

      if_(is_digit(line1[0])) {
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
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto isAlpha = ascii::is_alpha;
    
    function_<void()>("main") | define {
      print("Enter a character: ");
      auto line = read_line();

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
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto toUpper = ascii::to_upper.outline();
    auto toLower = ascii::to_lower.outline();
    auto strlen = MyString::length.outline();
    
    main_() {
      print("> ");

      auto line = (let_<MyString>("line") = read_line());
      auto len = let_<u8>("len") = strlen(line);
      for_(let_<u8>("i") = 0, var_("i") != len, ++var_("i")) {
	put(toLower(line[var_("i")]));
      };
      
      println();
      return_;
    };
    
  }
  endProgram();


  program_("strings");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u16>.outline();
    auto clear = algorithm::clear<MyString>;
    auto find_char = MyString::find_char.outline();
    
    main_() {
      print("String 1: ");
      auto str1 = (let_<MyString>("str1") = read_line());

      print("String 2: ");
      auto str2 = (let_<MyString>("str2") = read_line());

      println(find_char(str1, str2[0]));
      return_;
    };
    
  }
  endProgram();
  
  
  program_("isAlnum");
  {
    using MyString = String<20>;
    auto read_line = io::read_line<MyString>.outline();
    auto isAlnum = ascii::is_alphanumeric.outline();
    
    main_() {
      print("Enter a character: ");
      auto line1 = read_line();

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
    using MyString = String<5>;
    using Screen = ansi::Screen<50, 50>;
    auto read_line = io::read_line<MyString>.outline();    
    
    function_<void()>("main") | define {

      print("Enter a char: ");
      let_<u8>("ch") = read_line()[0];
      
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
  

  program_("arrays");
  {
    using MyString = String<10>;
    using Vec = Array<MyString, 5>;
    using Mat = Array<Vec, 3>;
    
    auto read_line = io::read_line<MyString>.outline();
    auto fill_vec = Vec::fill;
    auto sort_vec = Vec::sort;
    auto vec_contains = Vec::contains;
    auto clear_vec = algorithm::clear<Vec>;
    auto clear_mat = algorithm::clear<Mat>;

    auto print_array = function_<void(Vec)>("print_array", "arr")  | define {
      print('[');
      for (size_t i = 0; i != Vec::Size; ++i) {
	print(var_("arr")[i]);
	if (i < Vec::Size - 1) {
	  print(", ");
	}
      }
      println(']');
      return_;
    };


    auto print_array2 = function_<void(Mat)>("print_array2", "arr")  | define {
      println('[');
      for (size_t i = 0; i != Mat::Size; ++i) {
	print("  ");
	print_array(var_("arr")[i]);
      }
      println(']');
      return_;
    };

    
    main_() {
      print("Enter 5 strings: ");
      let_<Vec>("vec");
      for (size_t i = 0; i != Vec::Size; ++i) {
	var_("vec")[i] = read_line();
      }

      let_<Mat>("mat");
      for (size_t i = 0; i != Mat::Size; ++i) {
	var_("mat")[i] = var_("vec");
      }

      print("First string starts with 'al': ");
      println(MyString::starts_with<MyString>(var_("vec")[0], "al"));
      print_array2(var_("mat"));
      sort_vec(var_("mat")[0]);
      print_array2(var_("mat"));
      clear_mat(var_("mat"));
      print_array2(var_("mat"));
      
      return_;
    };
  }
  endProgram();


  program_("structs");
  {
    using MyString = String<10>;
    using Person = Struct<"Person",
      Field<"name", MyString>,
      Field<"age", u8>
      >;
    using PersonArray = Array<Person, 5>;
  
    auto read_line = io::read_line<MyString>.outline();
    auto str2int = MyString::to_int<u8>.outline();
    auto clear = algorithm::clear<PersonArray>;
    auto find = PersonArray::find.outline();

    auto print_person = function_<void(Person)>("print_person", "person")  | define {
      print('{');
      print(var_("person").field("name"));
      print(", ");
      print(var_("person").field("age"));
      println('}');
      return_;
    };
    
    auto print_array = function_<void(PersonArray)>("print_array", "arr")  | define {
      println('[');
      for (size_t i = 0; i != PersonArray::Size; ++i) {
	print("  ");
	print_person(var_("arr")[i]);
      }
      println(']');
      return_;
    };

    
    main_() {
      let_<PersonArray>("arr");
      let_<Person>("person");
      print("Enter name: ");
      var_("person").field("name") = read_line();
      print("Enter age: ");
      var_("person").field("age") = str2int(read_line());

      for (int i = 0; i != PersonArray::Size; ++i) {
	var_("arr")[i] = var_("person");
      }

      var_("arr")[0].field("name") = "pinda";

      print_array(var_("arr"));
      println(find(var_("arr"), var_("person")));
      clear(var_("arr"));
      print_array(var_("arr"));
      return_;
    };
  }
  endProgram();

  
  std::cout << generateBrainfuck("str2int");
  
 } catch (std::exception const &e) {
  std::cerr << e.what() << '\n';
 }
