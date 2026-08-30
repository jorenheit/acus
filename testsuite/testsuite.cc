// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <unordered_map>
#include <utility>
#include <vector>

// ============================================================
// Minimal Brainfuck compiler test runner
//
// This version runs each test in a subprocess by default.
// That isolates global/static state and makes cross-test
// interference much easier to avoid and debug.
//
// Typical usage:
//   - add tests to the vector in main()
//   - each test returns the generated Brainfuck program
//   - parent process launches this same executable with
//     --run-test <index> for each test case
// ============================================================

namespace bftest {

  struct RunOptions {
    std::size_t maxSteps = -1UL; //50'000'000;
    bool errorOnPointerUnderflow = false;
  };

  struct RunResult {
    std::string output;
    bool halted = false;
    bool success = false;
    std::string message;
    std::size_t steps = 0;
  };

  inline std::vector<std::size_t> buildJumpTable(std::string const &program) {
    std::vector<std::size_t> jump(program.size(), static_cast<std::size_t>(-1));
    std::vector<std::size_t> stack;
    stack.reserve(program.size() / 4);

    for (std::size_t i = 0; i < program.size(); ++i) {
      if (program[i] == '[') {
	stack.push_back(i);
      } else if (program[i] == ']') {
	if (stack.empty()) {
	  throw std::runtime_error("Unmatched ']' at pc=" + std::to_string(i));
	}
	auto open = stack.back();
	stack.pop_back();
	jump[open] = i;
	jump[i] = open;
      }
    }

    if (!stack.empty()) {
      throw std::runtime_error("Unmatched '[' at pc=" + std::to_string(stack.back()));
    }

    return jump;
  }

  inline RunResult runBrainfuck(std::string const &program,
				std::string const &input = "",
				RunOptions opts = {}) {
    RunResult result;

    std::vector<std::size_t> jump;
    try {
      jump = buildJumpTable(program);
    } catch (std::exception const &e) {
      result.message = e.what();
      return result;
    }

    std::unordered_map<long long, std::uint8_t> tape;
    tape.reserve(4096);

    auto cell = [&](long long ptr) -> std::uint8_t& {
      return tape[ptr];
    };

    long long ptr = 0;
    std::size_t pc = 0;
    std::size_t inputPos = 0;

    while (pc < program.size()) {
      if (result.steps++ >= opts.maxSteps) {
	result.message = "Step limit exceeded";
	result.halted = false;
	result.success = false;
	return result;
      }

      switch (program[pc]) {
      case '>':
	++ptr;
	break;
      case '<':
	--ptr;
	if (opts.errorOnPointerUnderflow && ptr < 0) {
	  result.message = "Pointer moved below 0";
	  return result;
	}
	break;
      case '+':
	++cell(ptr);
	break;
      case '-':
	--cell(ptr);
	break;
      case '.':
	result.output.push_back(static_cast<char>(cell(ptr)));
	break;
      case ',':
	cell(ptr) = inputPos < input.size()
	  ? static_cast<std::uint8_t>(input[inputPos++])
	  : 0;
	break;
      case '[':
	if (cell(ptr) == 0) {
	  pc = jump[pc];
	}
	break;
      case ']':
	if (cell(ptr) != 0) {
	  pc = jump[pc];
	}
	break;
      default:
	break;
      }
      ++pc;
    }

    result.halted = true;
    result.success = true;
    result.message = "ok";
    return result;
  }

  struct TestCase {
    std::string name;
    std::function<std::string()> buildProgram;
    std::string expectedOutput;
    std::string input;
    bool requireHalt = true;
    std::size_t maxSteps = -1UL; //50'000'000;
  };

  struct TestOutcome {
    std::string name;
    bool passed = false;
    std::string expectedOutput;
    std::string actualOutput;
    bool expectedHalt = true;
    bool actualHalt = false;
    std::string message;
    std::size_t steps = 0;
  };

  inline std::string hexEncode(std::string const &s) {
    static constexpr char digits[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(s.size() * 2);
    for (unsigned char c : s) {
      out.push_back(digits[c >> 4]);
      out.push_back(digits[c & 0x0F]);
    }
    return out;
  }

  inline std::string hexDecode(std::string const &s) {
    auto nibble = [](char c) -> int {
      if (c >= '0' && c <= '9') return c - '0';
      if (c >= 'A' && c <= 'F') return c - 'A' + 10;
      if (c >= 'a' && c <= 'f') return c - 'a' + 10;
      throw std::runtime_error("Invalid hex digit");
    };

    if (s.size() % 2 != 0) {
      throw std::runtime_error("Invalid hex length");
    }

    std::string out;
    out.reserve(s.size() / 2);
    for (std::size_t i = 0; i < s.size(); i += 2) {
      out.push_back(static_cast<char>((nibble(s[i]) << 4) | nibble(s[i + 1])));
    }
    return out;
  }

  inline bool startsWith(std::string const &s, std::string const &prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
  }

  inline TestOutcome runTest(TestCase const &tc) {
    TestOutcome out;
    out.name = tc.name;
    out.expectedOutput = tc.expectedOutput;
    out.expectedHalt = tc.requireHalt;

    try {
      std::string bf = tc.buildProgram();
      RunResult rr = runBrainfuck(bf, tc.input, RunOptions{.maxSteps = tc.maxSteps});

      out.actualOutput = rr.output;
      out.actualHalt = rr.halted;
      out.message = rr.message;
      out.steps = rr.steps;

      bool haltOk = (tc.requireHalt == rr.halted);
      bool outputOk = tc.requireHalt
	? (tc.expectedOutput == rr.output)
	: startsWith(rr.output, tc.expectedOutput);
      out.passed = haltOk && outputOk;
    } catch (std::exception const &e) {
      out.actualOutput.clear();
      out.actualHalt = false;
      out.message = std::string("Exception: ") + e.what();
      out.passed = false;
    } catch (...) {
      out.actualOutput.clear();
      out.actualHalt = false;
      out.message = "Unknown exception";
      out.passed = false;
    }

    return out;
  }

  inline void emitSingleTestResult(TestOutcome const &r) {
    std::cout << "BFT_RESULT_BEGIN\n";
    std::cout << "name=" << hexEncode(r.name) << "\n";
    std::cout << "passed=" << (r.passed ? 1 : 0) << "\n";
    std::cout << "expectedOutput=" << hexEncode(r.expectedOutput) << "\n";
    std::cout << "actualOutput=" << hexEncode(r.actualOutput) << "\n";
    std::cout << "expectedHalt=" << (r.expectedHalt ? 1 : 0) << "\n";
    std::cout << "actualHalt=" << (r.actualHalt ? 1 : 0) << "\n";
    std::cout << "message=" << hexEncode(r.message) << "\n";
    std::cout << "steps=" << r.steps << "\n";
    std::cout << "BFT_RESULT_END\n";
  }

  inline std::string readAllFromFd(int fd) {
    std::string out;
    char buffer[4096];
    while (true) {
      ssize_t n = ::read(fd, buffer, sizeof(buffer));
      if (n == 0) break;
      if (n < 0) throw std::runtime_error("Failed to read from child pipe");
      out.append(buffer, static_cast<std::size_t>(n));
    }
    return out;
  }

  inline TestOutcome parseSingleTestResult(std::string const &blob) {
    std::istringstream iss(blob);
    std::string line;
    std::unordered_map<std::string, std::string> kv;
    bool sawBegin = false;
    bool sawEnd = false;

    while (std::getline(iss, line)) {
      if (line == "BFT_RESULT_BEGIN") {
	sawBegin = true;
	continue;
      }
      if (line == "BFT_RESULT_END") {
	sawEnd = true;
	break;
      }
      auto pos = line.find('=');
      if (pos != std::string::npos) {
	kv.emplace(line.substr(0, pos), line.substr(pos + 1));
      }
    }

    if (!sawBegin || !sawEnd) {
      throw std::runtime_error("Could not parse child test result");
    }

    TestOutcome r;
    r.name = hexDecode(kv.at("name"));
    r.passed = (kv.at("passed") == "1");
    r.expectedOutput = hexDecode(kv.at("expectedOutput"));
    r.actualOutput = hexDecode(kv.at("actualOutput"));
    r.expectedHalt = (kv.at("expectedHalt") == "1");
    r.actualHalt = (kv.at("actualHalt") == "1");
    r.message = hexDecode(kv.at("message"));
    r.steps = static_cast<std::size_t>(std::stoull(kv.at("steps")));
    return r;
  }

  inline TestOutcome runSingleInSubprocess(std::vector<TestCase> const &tests,
					   std::size_t index,
					   char const *selfPath) {
    if (index >= tests.size()) {
      throw std::runtime_error("Test index out of range");
    }

    int pipeFds[2];
    if (::pipe(pipeFds) != 0) {
      throw std::runtime_error("pipe() failed");
    }

    pid_t pid = ::fork();
    if (pid < 0) {
      ::close(pipeFds[0]);
      ::close(pipeFds[1]);
      throw std::runtime_error("fork() failed");
    }

    if (pid == 0) {
      ::close(pipeFds[0]);
      if (::dup2(pipeFds[1], STDOUT_FILENO) < 0) {
	std::perror("dup2 stdout failed");
	std::_Exit(121);
      }
      if (::dup2(pipeFds[1], STDERR_FILENO) < 0) {
	std::perror("dup2 stderr failed");
	std::_Exit(122);
      }
      ::close(pipeFds[1]);

      std::string indexStr = std::to_string(index);
      char *const argv[] = {
	const_cast<char *>(selfPath),
	const_cast<char *>("--run-test"),
	indexStr.data(),
	nullptr
      };
      ::execv(selfPath, argv);
      std::perror("execv failed");
      std::_Exit(123);
    }

    ::close(pipeFds[1]);
    std::string childOutput = readAllFromFd(pipeFds[0]);
    ::close(pipeFds[0]);

    int status = 0;
    if (::waitpid(pid, &status, 0) < 0) {
      throw std::runtime_error("waitpid() failed");
    }

    TestOutcome parsed;
    try {
      parsed = parseSingleTestResult(childOutput);
    } catch (std::exception const &e) {
      parsed.name = tests[index].name;
      parsed.expectedOutput = tests[index].expectedOutput;
      parsed.expectedHalt = tests[index].requireHalt;
      parsed.actualHalt = false;
      parsed.passed = false;
      parsed.message = std::string("Child crashed or produced unreadable output: ") + e.what() +
	"\n--- child output ---\n" + childOutput;
    }

    if (WIFSIGNALED(status)) {
      parsed.passed = false;
      parsed.actualHalt = false;
      parsed.message += std::string(parsed.message.empty() ? "" : "\n") +
	"Child terminated by signal " + std::to_string(WTERMSIG(status));
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      parsed.passed = false;
      parsed.message += std::string(parsed.message.empty() ? "" : "\n") +
	"Child exited with code " + std::to_string(WEXITSTATUS(status));
    }

    return parsed;
  }

  inline int runAllIsolated(std::vector<TestCase> const &tests,
			    char const *selfPath,
			    bool failFast = false) {
    std::size_t passed = 0;

    for (std::size_t i = 0; i < tests.size(); ++i) {
      TestOutcome r = runSingleInSubprocess(tests, i, selfPath);
      
      std::cout << (r.passed ? "[PASS] " : "[FAIL] ") << r.name;
      if (!r.passed) {
	std::cout << "\n"
		  << "  expected output: " << std::quoted(r.expectedOutput) << "\n"
		  << "  actual output:   " << std::quoted(r.actualOutput) << "\n"
		  << "  expected halt:   " << (r.expectedHalt ? "yes" : "no") << "\n"
		  << "  actual halt:     " << (r.actualHalt ? "yes" : "no") << "\n"
		  << "  message:         " << r.message << "\n"
		  << "  steps:           " << r.steps;
      }
      std::cout << "\n";

      if (r.passed) {
	++passed;
      } else if (failFast) {
	break;
      }
    }

    std::cout << "\nSummary: " << passed << "/" << tests.size() << " passed\n";
    return passed == tests.size() ? EXIT_SUCCESS : EXIT_FAILURE;
  }

} // namespace bftest

inline bftest::TestCase expectOutput(std::string name,
                                     std::string expectedOutput,
                                     std::function<std::string()> buildProgram,
                                     std::size_t maxSteps = -1UL /*50'000'000*/) {
  return bftest::TestCase{
    .name = std::move(name),
    .buildProgram = std::move(buildProgram),
    .expectedOutput = std::move(expectedOutput),
    .input = "",
    .requireHalt = true,
    .maxSteps = maxSteps,
  };
}

inline bftest::TestCase expectLoopingOutput(std::string name,
                                            std::string expectedPrefixOutput,
                                            std::function<std::string()> buildProgram,
                                            std::size_t maxSteps = 2'000'000) {
  return bftest::TestCase{
    .name = std::move(name),
    .buildProgram = std::move(buildProgram),
    .expectedOutput = std::move(expectedPrefixOutput),
    .input = "",
    .requireHalt = false,
    .maxSteps = maxSteps,
  };
}

#include "acus.h"
using namespace acus::api;

#define CAT(c1, c2) (((int)c1) | ((int)(c2 << 8)))

#define TEST_BEGIN Assembler c; c.program("test", "main").begin();
#define TEST_END c.endProgram(); return c.brainfuck("test");

static std::vector<bftest::TestCase> buildTests() {
  std::vector<bftest::TestCase> tests;

tests.push_back(expectOutput("Global cache internal boundary",
                             "T", [] {
#include "tests/global_cache_internal_boundary.cc"
                             }));

tests.push_back(expectOutput("Scope cache entry must flush on endScope",
                             "abcd", [] {
#include "tests/scope_cache_entry_must_flush_on_endscope.cc"
                             }));
 
tests.push_back(expectOutput("Global cache path-dependent join",
                             "I", [] {
#include "tests/global_cache_path_dependent_join.cc"
                             }));

tests.push_back(expectOutput("Comparison LE unsigned max edge",
                             "BBBB", [] {
#include "tests/comparison_le_unsigned_max_edge.cc"
                             }));

tests.push_back(expectOutput("Literal div/mod zero folding",
                             "ABCD", [] {
#include "tests/literal_div_mod_zero.cc"
                             }));

tests.push_back(expectOutput("Logical negative constants",
                             "BAB", [] {
#include "tests/logical_negative_constants.cc"
                             })); 

tests.push_back(expectOutput("Dynamic array u16 index",
                             "Z", [] {
#include "tests/dynamic_array_u16_index.cc"
                             }));

tests.push_back(expectOutput("Dynamic array scaled offset overflow",
                             "ZZCD", [] {
#include "tests/dynamic_array_scaled_offset_overflow.cc"
                             }));

tests.push_back(expectOutput("Unsigned widening clears high byte",
                             "66", [] {
#include "tests/unsigned_widening_clears_high_byte.cc"
                             }));
 
tests.push_back(expectOutput("Global Variables 1",
			       "AF", []() {
#include "tests/global_variables_1.cc"
			       }));

  tests.push_back(expectOutput("Return Value i8",
			       "X", []() {
#include "tests/return_value_i8.cc"
			       }));

  tests.push_back(expectOutput("Global Variable Return",
			       "GG", []() {
#include "tests/global_variable_return.cc"
			       }));

  tests.push_back(expectOutput("Assign Return-Value to Global",
			       "YY",[]() {
#include "tests/assign_return_to_global.cc"
			       }));

  tests.push_back(expectOutput("Return i8 Array",
			       "ABCDABCD", []() {
#include "tests/return_i8_array.cc"
			       }));

  tests.push_back(expectOutput("Return i16 Array",
			       "ABCDEFGH", []() {
#include "tests/return_i16_array.cc"
			       }));

  tests.push_back(expectOutput("i8 Function Arguments",
			       "AB", []() {
#include "tests/function_arg_i8.cc"
			       }));

  tests.push_back(expectOutput("i16 Function Arguments",
			       "AB", []() {
#include "tests/function_arg_i16.cc"
			       }));
  
  tests.push_back(expectOutput("i8 Array Function Argument",
			       "ABCDABCD", []() {
#include "tests/function_arg_i8_array.cc"
			       }));

  tests.push_back(expectOutput("i16 Array Function Argument",
			       "ABCDABCD", []() {
#include "tests/function_arg_i16_array.cc"
			       }));
  
  tests.push_back(expectOutput("Global i8 Function Argument",
			       "GHG", []() {
#include "tests/function_arg_i8_global.cc"
			       }));

  tests.push_back(expectOutput("Mixed Function Arguments",
			       "ABCDEF", []() {
#include "tests/function_arg_mixed.cc"
			       }));

  tests.push_back(expectOutput("Nested Function Calls",
			       "A", []() {
#include "tests/function_call_nested.cc"
			       }));


  tests.push_back(expectLoopingOutput("Recursive Function Calls",
				      "ABC", []() {
#include "tests/recursive_call_with_arguments.cc"
				      }));

  tests.push_back(expectOutput("Function Call with Variable and Constants",
			       "AZBY", []() {
#include "tests/function_call_with_constants.cc"
			       }));

  tests.push_back(expectOutput("Nested Function Call with Return Value",
			       "Z", []() {
#include "tests/function_call_nested_with_return.cc"
			       }));
  
  tests.push_back(expectOutput("Function Call with Anonymous Array",
			       "ABCD", []() {
#include "tests/function_call_with_anonymous_array.cc"
			       }));

  tests.push_back(expectOutput("Function Call with Nested Anonymous Array",
			       "ABCD", []() {
#include "tests/function_call_with_anonymous_nested_array.cc"
			       }));

  tests.push_back(expectOutput("Shadowing Local Variables in Nested Scopes",
			       "ABXCDA", []() {
#include "tests/shadowing_local_variables.cc"
			       }));

  tests.push_back(expectOutput("WriteOut Temp i8",
			       "A", []() {
#include "tests/writeout_with_temporary_cell.cc"
			       }));

  tests.push_back(expectOutput("WriteOut Temp i16",
			       "AB", []() {
#include "tests/writeout_with_temporary_cell_i16.cc"
			       }));

  tests.push_back(expectOutput("WriteOut Temp Array",
			       "ABCD", []() {
#include "tests/writeout_with_temporary_array.cc"
			       }));

  tests.push_back(expectOutput("Passing String Variable of Same Size",
			       "Hello World", []() {
#include "tests/passing_string_as_variable_same_size.cc"
			       }));

  tests.push_back(expectOutput("Passing String Anonymously of Same Size",
			       "Hello World", []() {
#include "tests/passing_string_anonymous_same_size.cc"
			       }));

  tests.push_back(expectOutput("Passing String Variable of Smaller Size",
			       "Hello World", []() {
#include "tests/passing_string_variable_smaller_size.cc"
			       }));

  tests.push_back(expectOutput("Passing String Anonymous of Smaller Size",
			       "Hello World", []() {
#include "tests/passing_string_anonymous_smaller_size.cc"
			       }));

  tests.push_back(expectOutput("i8 to i16",
			       std::string("AA") + '\0', []() {
#include "tests/i8_to_i16_conversion.cc"
			       }));

  tests.push_back(expectOutput("Reading and Writing Struct Fields",
			       "ABAB", []() {
#include "tests/struct_field_read_write.cc"
			       }));

  tests.push_back(expectOutput("Mutating Global Struct",
			       "ABAC", []() {
#include "tests/reading_and_writing_from_global_struct.cc"
			       }));

 tests.push_back(expectOutput("Global Array Across Call",
                             "ABCD", []() {
#include "tests/global_array_across_call.cc"
                             }));
  
  tests.push_back(expectOutput("Return Value to Field",
			       "AZ", []() {
#include "tests/return_function_value_into_struct_field.cc"
			       }));

  tests.push_back(expectOutput("Passing Struct to Function",
			       "ABABXYAB", []() {
#include "tests/passing_struct_to_function.cc"
			       }));

  tests.push_back(expectOutput("Return Struct from Function",
			       "QR", []() {
#include "tests/return_struct_from_function.cc"
			       }));

  tests.push_back(expectOutput("Assign Anonymous Struct",
			       "AB", []() {
#include "tests/assign_anonymous_struct.cc"
			       }));

  tests.push_back(expectOutput("Print Anonymous Struct",
			       "AB", []() {
#include "tests/print_anonymous_struct.cc"
			       }));
  
  tests.push_back(expectOutput("Assign Anonymous To Dynamic Array Element",
			       "ABZD", []() {
#include "tests/assign_anonymous_to_dynamic_array_element.cc"
			       }));  

  tests.push_back(expectOutput("Assign Runtime Value To Dynamic Array Element",
			       "AYCD", []() {
#include "tests/assign_runtime_value_to_dynamic_array_element.cc"
			       }));

  tests.push_back(expectOutput("Dynamic Nested Array Element Write",
			       "ABXD", []() {
#include "tests/dynamic_nested_array_element_write.cc"
			       }));

  tests.push_back(expectOutput("Dynamic Array Of Structs Field Write",
			       "ABQRCD", []() {
#include "tests/dynamic_array_of_structs_field_write.cc"
			       }));

  tests.push_back(expectOutput("Return Value Into Dynamic Array Slot",
			       "ABCZ", []() {
#include "tests/return_value_into_dynamic_array_slot.cc"
			       }));

  tests.push_back(expectOutput("Return Value Into Dynamic Nested Struct Field",
			       "MNOQ", []() {
#include "tests/return_value_into_dynamic_nested_struct_field.cc"
			       })); 


tests.push_back(expectOutput("Read Dynamic Array Element Into Local",
                             "C", []() {
#include "tests/read_dynamic_array_element_into_local.cc"
                             }));

 tests.push_back(expectOutput("Assign Dynamic Array Element From Dynamic Array Element",
                             "ABAD", []() {
#include "tests/assign_dynamic_array_element_from_dynamic_array_element.cc"
                             }));

 tests.push_back(expectOutput("Self Assign Dynamic Array Element",
                             "ABCD", []() {
#include "tests/self_assign_dynamic_array_element.cc"
                             }));

 tests.push_back(expectOutput("Dynamic Array Element Boundary First",
                             "ZBCD", []() {
#include "tests/dynamic_array_element_boundary_first.cc"
                             }));
 
 tests.push_back(expectOutput("Dynamic Array Element Boundary Last",
                             "ABCZ", []() {
#include "tests/dynamic_array_element_boundary_last.cc"
                             }));

tests.push_back(expectOutput("Struct Field Array Dynamic Write",
                             "AQCD", []() {
#include "tests/struct_field_array_dynamic_write.cc"
                             }));

 tests.push_back(expectOutput("Read Dynamic Array Of Structs Field Into Local",
                             "C", []() {
#include "tests/read_dynamic_array_of_structs_field_into_local.cc"
                             }));

 tests.push_back(expectOutput("Pass Dynamic Array Element As Argument",
                             "C", []() {
#include "tests/pass_dynamic_array_element_as_argument.cc"
                             }));

 tests.push_back(expectOutput("Reuse Dynamic Array Element After Call",
                             "X", []() {
#include "tests/reuse_dynamic_array_element_after_call.cc"
                             }));

 tests.push_back(expectOutput("Static And Dynamic Nested Array Access",
                             "ABQD", []() {
#include "tests/static_and_dynamic_nested_array_access.cc"
                             }));

 tests.push_back(expectOutput("Reading from a global pointer inside function",
                             "GHX", []() {
#include "tests/reading_global_pointer_from_function.cc"
                             }));

 tests.push_back(expectOutput("Reading and writing through a local pointer",
			      "XXYX", []() {
#include "tests/reading_and_writing_local_pointer.cc"
			      }));

tests.push_back(expectOutput("Reading from a local pointer inside function",
                             "XX", []() {
#include "tests/reading_local_pointer_from_function.cc"
                             }));

 tests.push_back(expectOutput("Writing through a local pointer inside function",
			      "YY", []() {
#include "tests/writing_local_pointer_from_function.cc"
			      }));

 tests.push_back(expectOutput("Writing through a global pointer inside function",
			      "HH", []() {
#include "tests/writing_global_pointer_from_function.cc"
			      }));

 tests.push_back(expectOutput("Array of local pointers",
			      "ABXY", []() {
#include "tests/array_of_local_pointers.cc"
			      }));

 tests.push_back(expectOutput("Array of passed pointers",
			      "ABXY", []() {
#include "tests/array_of_passed_pointers.cc"
			      }));

 tests.push_back(expectOutput("Writing through two passed pointers",
                             "ABXY", []() {
#include "tests/writing_two_passed_pointers.cc"
                             }));

 tests.push_back(expectOutput("Reading from an array of passed pointers",
                             "ABAB", []() {
#include "tests/reading_array_of_passed_pointers.cc"
                             }));

 tests.push_back(expectOutput("Writing through an array of passed pointers",
                             "ABXY", []() {
#include "tests/writing_array_of_passed_pointers.cc"
                             }));

 tests.push_back(expectOutput("Reading through a global pointer locally",
                             "GG", []() {
#include "tests/reading_global_pointer_locally.cc"
                             }));

 tests.push_back(expectOutput("Writing through a global pointer locally",
                             "GH", []() {
#include "tests/writing_global_pointer_locally.cc"
                             }));

 tests.push_back(expectOutput("Reading and writing through a struct pointer field",
                             "AX", []() {
#include "tests/reading_and_writing_struct_pointer_field.cc"
                             }));

tests.push_back(expectOutput("Pointer to array element",
                             "BAXC", []() {
#include "tests/pointer_to_array_element.cc"
                             }));

tests.push_back(expectOutput("Struct with two pointer fields",
                             "ABXY", []() {
#include "tests/struct_with_two_pointer_fields.cc"
                             }));

 tests.push_back(expectOutput("Array of structs with pointer field",
			      "ABXY", []() {
#include "tests/array_of_structs_with_pointer_field.cc"
			      }));

 tests.push_back(expectOutput("Pointer passed through two calls",
			      "AX", []() {
#include "tests/pointer_passed_through_two_calls.cc"
			      }));

 tests.push_back(expectOutput("Pointer To Pointer Across Frames",
                             "A", []() {
#include "tests/pointer_to_pointer_across_frames.cc"
                             }));

 tests.push_back(expectOutput("Dereferenced Struct Rebases Pointer Field",
                             "A", []() {
#include "tests/dereferenced_struct_rebases_pointer_field.cc"
                             }));

 tests.push_back(expectOutput("Pointer Write Rebases To Older Frame",
                             "A", []() {
#include "tests/pointer_write_rebases_to_older_frame.cc"
                             }));

 tests.push_back(expectOutput("Struct Write Rebases Pointer Field To Older Frame",
                             "A", []() {
#include "tests/struct_write_rebases_pointer_field_to_older_frame.cc"
                             }));

 tests.push_back(expectOutput("Mixed named and pointer global access",
			      "AXX", []() {
#include "tests/mixed_named_and_pointer_global_access.cc"
			      })); 

tests.push_back(expectOutput("Cache Dynamic Array Field Write",
                             "AXC", []() {
#include "tests/cache_dynamic_array_field_write.cc"
                             }));

tests.push_back(expectOutput("Cache Nested Dynamic Array Write",
                             "ABXD", []() {
#include "tests/cache_nested_dynamic_array_write.cc"
                             }));

tests.push_back(expectOutput("Cache Direct Relative Gap Struct Array",
                             "AQCD", []() {
#include "tests/cache_direct_relative_gap_struct_array.cc"
                             }));

tests.push_back(expectOutput("Cache Source After Destination Preparation",
                             "BBC", []() {
#include "tests/cache_source_after_destination_preparation.cc"
                             }));

tests.push_back(expectOutput("Cache Dynamic Self Assignment",
                             "ABC", []() {
#include "tests/cache_dynamic_self_assignment.cc"
                             }));

tests.push_back(expectOutput("Cache Alias Read Flushes Dirty Dynamic Entry",
                             "AXC", []() {
#include "tests/cache_alias_read_flushes_dirty_dynamic_entry.cc"
                             }));

tests.push_back(expectOutput("Cache Alias Write Struct Field After Cached Read",
                             "BX", []() {
#include "tests/cache_alias_write_struct_field_after_cached_read.cc"
                             }));

tests.push_back(expectOutput("Cache Pointer Reassignment Invalidates Deref",
                             "AB", []() {
#include "tests/cache_pointer_reassignment_invalidates_deref.cc"
                             }));

tests.push_back(expectOutput("Cache Pointer Arithmetic Invalidates Deref",
                             "AB", []() {
#include "tests/cache_pointer_arithmetic_invalidates_deref.cc"
                             }));

tests.push_back(expectOutput("Cache Pointer Write After Arithmetic Struct Array",
                             "AXC", []() {
#include "tests/cache_pointer_write_after_arithmetic_struct_array.cc"
                             }));

tests.push_back(expectOutput("Cache Global Struct Field Call Boundary",
                             "AB", []() {
#include "tests/cache_global_struct_field_call_boundary.cc"
                             }));

tests.push_back(expectOutput("Cache Source From Pointer To Dynamic Destination",
                             "BBC", []() {
#include "tests/cache_source_from_pointer_to_dynamic_destination.cc"
                             }));

tests.push_back(expectOutput("Cache Dynamic Source To Pointer Destination",
                             "BBBC", []() {
#include "tests/cache_dynamic_source_to_pointer_destination.cc"
                             }));

tests.push_back(expectOutput("Cache Index Change Invalidates Clean Dynamic Entry",
                             "AC", []() {
#include "tests/cache_index_change_invalidates_clean_dynamic_entry.cc"
                             }));

tests.push_back(expectOutput("Cache Dirty Dynamic Entry Before Index Change",
                             "XBC", []() {
#include "tests/cache_dirty_dynamic_entry_before_index_change.cc"
                             }));

tests.push_back(expectOutput("Cache Nested Dirty Entry Before Inner Index Change",
                             "ABXD", []() {
#include "tests/cache_nested_dirty_entry_before_inner_index_change.cc"
                             }));

tests.push_back(expectOutput("Cache Cross Index Copy",
                             "CBCD", []() {
#include "tests/cache_cross_index_copy.cc"
                             }));

tests.push_back(expectOutput("Cache Runtime Same Index Copy",
                             "ABC", []() {
#include "tests/cache_runtime_same_index_copy.cc"
                             }));

tests.push_back(expectOutput("Cache Alias Write Invalidates Clean Structural Entry",
                             "BX", []() {
#include "tests/cache_alias_write_invalidates_clean_structural_entry.cc"
                             }));

tests.push_back(expectOutput("Cache Alias Write After Dirty Structural Entry",
                             "X", []() {
#include "tests/cache_alias_write_after_dirty_structural_entry.cc"
                             }));

tests.push_back(expectOutput("Cache Pointer Argument Sees Dirty Dynamic Entry",
                             "X", []() {
#include "tests/cache_pointer_argument_sees_dirty_dynamic_entry.cc"
                             }));

tests.push_back(expectOutput("Cache Pointer Read After Dirty Global Field",
                             "X", []() {
#include "tests/cache_pointer_read_after_dirty_global_field.cc"
                             }));

tests.push_back(expectOutput("Cache Alias Write Then Structural Owner Write",
                             "XY", []() {
#include "tests/cache_alias_write_then_structural_owner_write.cc"
                             }));

tests.push_back(expectOutput("Cache Structural Write Then Alias Write Same Owner",
                             "XY", []() {
#include "tests/cache_structural_write_then_alias_write_same_owner.cc"
                             }));
 
 tests.push_back(expectOutput("Logical NOT",
			      "BABABABABB", []() {
#include "tests/logical_not.cc"
			      }));

tests.push_back(expectOutput("Logical BOOL",
			     "ABABABABBA", []() {
#include "tests/logical_bool.cc"
			     }));

 tests.push_back(expectOutput("Logical AND",
                             "BABABABABA", []() {
#include "tests/logical_and.cc"
                             }));

tests.push_back(expectOutput("Logical NAND",
                             "ABABABABAB", []() {
#include "tests/logical_nand.cc"
                             }));

tests.push_back(expectOutput("Logical OR",
                             "BBBBBBBBBA", []() {
#include "tests/logical_or.cc"
                             }));

tests.push_back(expectOutput("Logical NOR",
                             "AAAAAAAAAB", []() {
#include "tests/logical_nor.cc"
                             }));

tests.push_back(expectOutput("Logical XOR",
                             "ABABABABAA", []() {
#include "tests/logical_xor.cc"
                             }));

 tests.push_back(expectOutput("Logical XNOR",
			      "BABABABABB", []() {
#include "tests/logical_xnor.cc"
			      })); 

 tests.push_back(expectOutput("Comparison LT",
			      "BABABAAAABABB", []() {
#include "tests/comparison_less_than.cc"
			      }));

 tests.push_back(expectOutput("Comparison LE",
			      "BABABABABBABB", []() {
#include "tests/comparison_less_equal.cc"
			      }));

  tests.push_back(expectOutput("Comparison GT",
			      "ABABABABAABBA", []() {
#include "tests/comparison_greater_than.cc"
			      }));

 tests.push_back(expectOutput("Comparison GE",
			      "ABABABBBBABBA", []() {
#include "tests/comparison_greater_equal.cc"
			      }));

 tests.push_back(expectOutput("Comparison EQ",
			      "AAAAAABABAABA", []() {
#include "tests/comparison_equal.cc"
			      }));

 tests.push_back(expectOutput("Comparison NEQ",
			      "BBBBBBABABBBB", []() {
#include "tests/comparison_not_equal.cc"
			      }));

 tests.push_back(expectOutput("Binary Operand Swapping",
                             std::string(38, 'B'), []() {
#include "tests/binary_operand_swapping.cc"
                             }));

 tests.push_back(expectOutput("Pointer Addition Operand Swapping",
                             "CC", []() {
#include "tests/pointer_addition_operand_swapping.cc"
                             }));

tests.push_back(expectOutput("Signed Negate",
                             "ABCDEFF",
                             [] {
#include "tests/signed_negate.cc"
                             }));

tests.push_back(expectOutput("Signed Negate Assign",
                             "ABCDEXBHH",
                             [] {
#include "tests/signed_negate_assign.cc"
                             }));
 
 tests.push_back(expectOutput("Signed i8 Add/Sub/Neg",
			      "ABCDEF", []() {
#include "tests/signed_i8_add_sub_neg.cc"
			      }));

tests.push_back(expectOutput("Signed i16 Add/Sub/Neg",
                             "ABABAB", []() {
#include "tests/signed_i16_add_sub_neg.cc"
                             }));

tests.push_back(expectOutput("Signed Widening s8 to s16",
                             "ABBDBA", []() {
#include "tests/signed_widening_s8_to_s16.cc"
                             }));

tests.push_back(expectOutput("Signed Mixed Width s8/s16",
                             "ABAB", []() {
#include "tests/signed_mixed_width_s8_s16.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Mul Const",
                             "ABCDEF",
                             [] {
#include "tests/signed_s8_mul_const.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Mul Vars",
                             "ABCDEF",
                             [] {
#include "tests/signed_s8_mul_vars.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Mul Const",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_s16_mul_const.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Mul Vars",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_s16_mul_vars.cc"
                             }));

tests.push_back(expectOutput("Signed Mul Widening",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_mul_widening.cc"
                             }));

tests.push_back(expectOutput("Signed Mul Literal Folding",
                             "ABCDEF",
                             [] {
#include "tests/signed_mul_literal_folding.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Unary Ops",
                             "ABCDEFGHIJKL",
                             [] {
#include "tests/signed_s8_unary_ops.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Unary Ops",
                             "ABCDEFGHIJKLMNOPQQRRSS",
                             [] {
#include "tests/signed_s16_unary_ops.cc"
                             }));
 
tests.push_back(expectOutput("Signed s8 Div/Mod Slots",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_s8_div_mod_slots.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Div/Mod Consts",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_s8_div_mod_consts.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Div/Mod Assign",
                             "ABCDEFGH",
                             [] {
#include "tests/signed_s8_div_mod_assign.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Div/Mod Slots",
                             "ABCDEFGHIJKLMNOP",
                             [] {
#include "tests/signed_s16_div_mod_slots.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Div/Mod Consts",
                             "ABCDEFGHIJKLMNOP",
                             [] {
#include "tests/signed_s16_div_mod_consts.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Div/Mod Assign",
                             "ABCDEFGHIJKLMNOP",
                             [] {
#include "tests/signed_s16_div_mod_assign.cc"
                             }));
 
tests.push_back(expectOutput("Signed Div/Mod Literal Folding",
                             "ABCDEFGHIJKLMNOP",
                             [] {
#include "tests/signed_div_mod_literal_folding.cc"
                             }));

tests.push_back(expectOutput("Signed s8 Comparison Slots",
                             "BABAABABBBAB",
                             [] {
#include "tests/signed_s8_comparison_slots.cc"
                             }));

tests.push_back(expectOutput("Signed s16 Comparison Slots",
                             "BABAABABBBAB",
                             [] {
#include "tests/signed_s16_comparison_slots.cc"
                             }));

tests.push_back(expectOutput("Signed Comparison Consts",
                             "BABABABABABA",
                             [] {
#include "tests/signed_comparison_consts.cc"
                             }));

tests.push_back(expectOutput("Signed Comparison Widening",
                             "BABAABBABBAB",
                             [] {
#include "tests/signed_comparison_widening.cc"
                             }));

tests.push_back(expectOutput("Signed Comparison Assign",
                             "BABABBABBABB",
                             [] {
#include "tests/signed_comparison_assign.cc"
                             }));

tests.push_back(expectOutput("Signed Comparison Literal Folding",
                             "BABABABABBAB",
                             [] {
#include "tests/signed_comparison_literal_folding.cc"
                             }));
 
 tests.push_back(expectOutput("Integer Addition: 8 bit",
			      "ADG", []() {
#include "tests/integer_addition_i8.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction: 8 bit",
			      "GDA", []() {
#include "tests/integer_subtraction_i8.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction: 16 bit",
			      "GADAAA", []() {
#include "tests/integer_subtraction_i16.cc"
                             }));

 tests.push_back(expectOutput("Integer Subtraction Constants: 8 bit",
			      "GDA", []() {
#include "tests/integer_subtraction_i8_constants.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction Constants: 16 bit",
			      "GADAAA", []() {
#include "tests/integer_subtraction_i16_constants.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction Array Elements: 8 bit",
			      "GDA", []() {
#include "tests/integer_subtraction_array_elements_i8.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction Array Elements: 16 bit",
			      "GADAAA", []() {
#include "tests/integer_subtraction_array_elements_i16.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction Struct Fields: 8 bit",
			      "GDA", []() {
#include "tests/integer_subtraction_struct_fields_i8.cc"
			      }));

 tests.push_back(expectOutput("Integer Subtraction Struct Fields: 16 bit",
			      "GADAAA", []() {
#include "tests/integer_subtraction_struct_fields_i16.cc"
			      }));

tests.push_back(expectOutput("Integer Subtraction: 16 bit borrow",
                             "ACCBEA", []() {
#include "tests/integer_subtraction_i16_borrow.cc"
                             }));

 tests.push_back(expectOutput("Integer Subtraction Constants: 16 bit borrow",
			      "ACCBEA", []() {
#include "tests/integer_subtraction_i16_borrow_constants.cc"
			      })); 

 tests.push_back(expectOutput("Integer Addition: 16 bit carry",
			      "EACBAC", []() {
#include "tests/integer_addition_i16_carry.cc"
			      }));

 tests.push_back(expectOutput("Integer Addition Constants: 16 bit carry",
			      "EACBAC", []() {
#include "tests/integer_addition_i16_carry_constants.cc"
			      }));

 tests.push_back(expectOutput("Integer Addition: 16 bit no carry",
                             "ACBC", []() {
#include "tests/integer_addition_i16_spurious_carry.cc"
                             }));

 tests.push_back(expectOutput("Integer Addition: 16 bit no carry full",
                             "ACBCCC", []() {
#include "tests/integer_addition_i16_spurious_carry_full.cc"
                             }));
tests.push_back(expectOutput("Address Of Struct Field",
                             "BAX", []() {
#include "tests/address_of_struct_field.cc"
                             }));

tests.push_back(expectOutput("Address Of Struct Field In Array",
                             "baXc", []() {
#include "tests/address_of_array_struct_field.cc"
                             }));

tests.push_back(expectOutput("Pointer To Pointer: Local",
                             "AZZ", []() {
#include "tests/pointer_to_pointer_local.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: i8 Array",
                             "ACBB", []() {
#include "tests/pointer_arithmetic_i8_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: i16 Array",
                             "AaCcBb", []() {
#include "tests/pointer_arithmetic_i16_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: i16 Array Variable Offset",
                             "AaCcBb", []() {
#include "tests/pointer_arithmetic_i16_array_variable.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Struct Array",
                             "ACB", []() {
#include "tests/pointer_arithmetic_struct_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Nested Array",
                             "ACB", []() {
#include "tests/pointer_arithmetic_nested_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Write After Arithmetic: i8 Array",
                             "ABXD", []() {
#include "tests/pointer_write_after_arithmetic_i8_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Write After Arithmetic: i16 Array",
                             "AaXxCc", []() {
#include "tests/pointer_write_after_arithmetic_i16_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Write After Arithmetic: Struct Array",
                             "AXC", []() {
#include "tests/pointer_write_after_arithmetic_struct_array.cc"
                             }));

tests.push_back(expectOutput("Pointer Aliasing: Same Target",
                             "XX", []() {
#include "tests/pointer_aliasing_same_target.cc"
                             }));

tests.push_back(expectOutput("Pointer Copy Independence After Arithmetic",
                             "AB", []() {
#include "tests/pointer_copy_independence_after_arithmetic.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Zero And Roundtrip i8",
                             "AAA", []() {
#include "tests/pointer_arithmetic_zero_roundtrip_i8.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Zero And Roundtrip i16",
                             "AaAaAa", []() {
#include "tests/pointer_arithmetic_zero_roundtrip_i16.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Struct Array Variable Offset",
                             "ACB", []() {
#include "tests/pointer_arithmetic_struct_array_variable.cc"
                             }));

tests.push_back(expectOutput("Pointer Arithmetic: Nested Array Variable Offset",
                             "ACB", []() {
#include "tests/pointer_arithmetic_nested_array_variable.cc"
                             }));

tests.push_back(expectOutput("Pointer To Pointer Arithmetic: Array Of Pointers",
                             "ACB", []() {
#include "tests/pointer_to_pointer_array_arithmetic.cc"
                             })); 

tests.push_back(expectOutput("Integer Multiplication i8",
                             "AM", []() {
#include "tests/integer_multiplication_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Multiplication i16",
                             "\"\"DD", []() {
#include "tests/integer_multiplication_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Multiplication Mixed i16/i8",
                             "\"\"DD", []() {
#include "tests/integer_multiplication_mixed_i16_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Division i8",
                             "!#", []() {
#include "tests/integer_division_i8.cc"
                             }));

tests.push_back(expectOutput("Integer casts",
			     "-20|236|65535|-1|44|-1|255|255|32767|-32768|",
			     []() {
#include "tests/integer_casts.cc"
			     }));
 
tests.push_back(expectOutput("Integer Division Mixed i16/i8",
                             "AACC", []() {
#include "tests/integer_division_mixed_i16_i8.cc"
                             }));
 
tests.push_back(expectOutput("Integer Modulo i8",
                             " !", []() {
#include "tests/integer_modulo_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Modulo Mixed i16/i8",
                             "AABB", []() {
#include "tests/integer_modulo_mixed_i16_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Division Edge Cases i8",
                             "ABCD", []() {
#include "tests/integer_division_edgecases_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Division Edge Cases i16",
                             "AABBCCDD", []() {
#include "tests/integer_division_edgecases_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Modulo Edge Cases i8",
                             "ABCDE", []() {
#include "tests/integer_modulo_edgecases_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Modulo Edge Cases i16",
                             "AABBCCDDEE", []() {
#include "tests/integer_modulo_edgecases_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Multiplication Edge Cases i8",
                             "ABCD", []() {
#include "tests/integer_multiplication_edgecases_i8.cc"
                             }));

tests.push_back(expectOutput("Integer Multiplication Edge Cases i16",
                             "AABBCCDD", []() {
#include "tests/integer_multiplication_edgecases_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Literal RHS i8",
                             "ABCDEABCDE", []() {
#include "tests/integer_literals_i8.cc"
                             }));

 tests.push_back(expectOutput("Function Pointer Direct",
			      "foo2foo1", []() {
#include "tests/function_pointer_direct.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Assign",
			      "AB", []() {
#include "tests/function_pointer_assign.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Args",
			      "LARB", []() {
#include "tests/function_pointer_args.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Return Value",
			      "BA", []() {
#include "tests/function_pointer_return_value.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Returned Pointer",
			      "TF", []() {
#include "tests/function_pointer_returned_pointer.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Array Static",
			      "BA", []() {
#include "tests/function_pointer_array_static.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Array Dynamic",
			      "AB", []() {
#include "tests/function_pointer_array_dynamic.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Struct Field",
			      "AB", []() {
#include "tests/function_pointer_struct_field.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Returned By Pointer Call",
			      "SATB", []() {
#include "tests/function_pointer_returned_by_pointer_call.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Passed Through Frames",
			      "Z", []() {
#include "tests/function_pointer_passed_through_frames.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Array Dynamic Loop",
			      "ABAB", []() {
#include "tests/function_pointer_array_dynamic_loop.cc"
			      }));

 tests.push_back(expectOutput("Function Pointer Second Order Dispatch",
			      "XY", []() {
#include "tests/function_pointer_second_order_dispatch.cc"
			      }));

 // THESE TAKE VERY LONG:

 tests.push_back(expectOutput("Integer Division i16",
                             "AACC", []() {
#include "tests/integer_division_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Modulo i16",
                             "  !!", []() {
#include "tests/integer_modulo_i16.cc"
                             }));

tests.push_back(expectOutput("Integer Literal RHS i16",
                             "AABBCCDDEEAABBCCDDEE", []() {
#include "tests/integer_literals_i16.cc"
                             }));
 
tests.push_back(expectOutput("Integer Decimal Print Literals",
                             "0 7 255 -1 -3 -128 0 42 65535 -1 -300 -32768",
                             [] {
#include "tests/integer_decimal_print.cc"
                             }));

tests.push_back(expectOutput("Integer Decimal Print Variables",
                             "255 -3 65535 -300 127 -128 32767 -32768",
                             [] {
#include "tests/integer_decimal_print_variables.cc"
                             })); 

 return tests;
}

int main(int argc, char **argv) {
  auto tests = buildTests();

  if (argc == 3 && std::strcmp(argv[1], "--run-test") == 0) {
    std::size_t index = static_cast<std::size_t>(std::stoull(argv[2]));
    if (index >= tests.size()) {
      std::cerr << "Test index out of range\n";
      return 2;
    }
    auto outcome = bftest::runTest(tests[index]);
    bftest::emitSingleTestResult(outcome);
    return outcome.passed ? 0 : 1;
  }

  return bftest::runAllIsolated(tests, argv[0]);
}
