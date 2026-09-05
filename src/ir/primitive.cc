// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sstream>
#include <iostream>
#include "acus/ir/primitive.h"
#include "acus/core/data.h"
#include "acus/util/util.h"

using namespace acus;

std::string primitive::Sequence::dumpText(Context const &ctx) {
  std::ostringstream oss;
  for (auto const &node: nodes) {
    std::string str = node->text(ctx);
    if (not str.empty()) {
      oss << str << '\n';
    }
  }
  return oss.str();
}


std::string primitive::Sequence::dumpCode(Context const &ctx) {
  std::ostringstream oss;
  for (auto const &node: nodes) oss << node->generate(ctx);
  return oss.str();  
}


namespace acus::constants {
  using FactorPair = std::pair<uint8_t, uint8_t>;
  extern std::array<FactorPair, 129> table;
};


namespace acus::Algorithm {

  std::string movePtr(int amount) {
    char const ch = (amount > 0) ? '>' : '<';
    return std::string(std::abs(amount), ch);
  }
  
  std::string movePtr(int dest, int current) {
    return movePtr(dest - current);
  }

  std::string decrement(int n = 1) { assert(n >= 0); return std::string(n, '-'); }
  std::string increment(int n = 1) { assert(n >= 0); return std::string(n, '+'); }

  
  std::string zero()      { return "[-]"; }
  std::string zeroPlus()  { return "[+]"; }

  // Move current into target. Leaves current at 0
  std::string moveValueAssumeZero(int current, int target) {
    assert(util::allDifferent(current, target));
    
    // [->+<]
    std::ostringstream oss;
    oss << "["
	<<   decrement()
	<<   movePtr(target, current)
	<<   increment()
	<<   movePtr(current, target)
	<< "]";
    
    return oss.str();
  }

  template <typename Action>
  std::string moveToTargetAnd(int &current, int target, Action &&action) {
    std::ostringstream oss;
    oss << movePtr(target, current) << action();
    current = target;
    return oss.str();
  }

  
  // Move value of current into target, and optionally other targets as well
  template <typename... Targets>
  requires (std::is_integral_v<Targets> && ...)
  std::string moveValueAssumeZero(int current, int target, Targets ... others) {
    assert(util::allDifferent(current, target, others...));

    std::ostringstream oss;
    int const origin = current;
    
    // Move into first target
    oss << "["
        << decrement()
        << moveToTargetAnd(current, target, []{ return increment(); });

    // Move into others as well if they are supplied
    ((oss << moveToTargetAnd(current, others, []{ return increment(); })), ...);

    // Move back to origin
    oss << movePtr(origin, current)
        << "]";

    return oss.str();
  }  
  
  // Move current into target. Leaves current at 0
  template <typename... Targets>
  requires (std::is_integral_v<Targets> && ...)
  std::string moveValue(int current, int target, Targets ... others) {
    assert(util::allDifferent(current, target, others...));
    
    std::ostringstream oss;
    int const origin = current;

    // First zero all targets
    oss << moveToTargetAnd(current, target, zero);
    // Zero remaining targets
    ((oss << moveToTargetAnd(current, others, zero)), ...);    
    // Now move back to origin and move the value into all targets
    oss << movePtr(origin, current)
	<< moveValueAssumeZero(origin, target, others ...);

    return oss.str();
  }


  // Copy current to all specified targets (at least 1)
  template <typename... Targets>
  requires ((std::is_integral_v<Targets> && ...) && sizeof ... (Targets) >= 1)
  std::string copyValue(int current, int tmp, Targets... targets) {
    assert(util::allDifferent(current, tmp, targets ...));

    std::ostringstream oss;

    // Move current into all targets, including the tmp
    oss << moveValue(current, tmp, targets...) << movePtr(tmp, current)
        << moveValueAssumeZero(tmp, current) << movePtr(current, tmp);

    return oss.str();
  }
  
  std::string modify(int n) {
    if (n == 0) return "";
    return (n > 0) ? increment(std::abs(n)) : decrement(std::abs(n));
  }

  std::string modify(int val, int current, int tmp) {

    auto const [n, countBack] = [&] -> std::pair<int, bool> {
      int norm = val % 256;
      if (norm < 0) norm += 256;
      bool const countBack = norm > 128;
      return {countBack ? 256 - norm : norm, countBack};
    }();
    
    if (val == 0) return "";

    auto const naive = [&] -> std::string {
      std::ostringstream oss;
      oss << (countBack ? decrement(n) : increment(n));
      return oss.str();
    };
    
    auto const smart = [&] -> std::string {
      auto const [a, b] = constants::table[n];
      std::ostringstream oss;
      oss << movePtr(tmp, current)
	  << increment(a)
	  << "["
	  <<   decrement()
	  <<   movePtr(current, tmp)
	  <<   (countBack ? decrement(b) : increment(b))
	  <<   movePtr(tmp, current)
	  << "]"
	  << movePtr(current, tmp)
	  << modify((n - a * b) * (countBack ? -1 : 1));

      return oss.str();
    };

    std::string const smartResult = smart();
    std::string const naiveResult = naive();
    
    return smartResult.length() < naiveResult.length()
      ? smartResult
      : naiveResult;
  }
  
  std::string setToValue(int val) {
    std::ostringstream oss;
    oss << zero() << modify(val);
    return oss.str();    
  }

  std::string setToValue(int val, int current, int tmp) {
    std::ostringstream oss;
    oss << zero() << modify(val, current, tmp);
    return oss.str();    
  }


  // Store !!current back intor current
  std::string boolean(int current, int tmp) {
    assert(util::allDifferent(current, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(current, tmp)
	<< "["
	<<   movePtr(tmp, current)
	<<   "+"
	<<   movePtr(current, tmp)
	<<   zero()
	<< "]"
	<< movePtr(tmp, current)
	<< moveValueAssumeZero(tmp, current)
	<< movePtr(current, tmp);
    return oss.str();
  }

  // Store !current back into current  
  std::string notValue(int current, int tmp) {
    assert(util::allDifferent(current, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< setToValue(1)
	<< movePtr(current, tmp)
	<< "["
	<<   movePtr(tmp, current)
	<<   "-"
	<<   movePtr(current, tmp)
	<<   zero()
	<< "]"
	<< movePtr(tmp, current)
	<< moveValueAssumeZero(tmp, current)
	<< movePtr(current, tmp);
    return oss.str();
  }

  // Destructive OR (result in current, destroys other) -> boolean output
  // TODO: check if we assume boolean output when using this
  std::string orValues(int current, int other) {
    assert(util::allDifferent(current, other));
    
    std::ostringstream oss;
    oss << "["
	<<   zero()
	<<   movePtr(other, current)
	<<   setToValue(1)
	<<   movePtr(current, other)
	<< "]"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(current, other)
	<<   "+"
	<<   movePtr(other, current)
	<< "]"
	<<  movePtr(current, other);

    return oss.str();
  }

  // Destructive AND (result in current, destroys other)
  std::string andValues(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));
    
    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< setToValue(2)
	<< movePtr(current, tmp)
	<< "["
	<<   zero()
	<<   movePtr(tmp, current)
	<<   "-"
	<<   movePtr(current,tmp)
	<< "]+"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(tmp, other)
	<<   "-"
	<<   movePtr(other, tmp)
	<< "]"
	<< movePtr(tmp, other)
	<< "["
	<<   zero()
	<<   movePtr(current, tmp)
	<<   "-"
	<<   movePtr(tmp, current)
	<< "]"
	<<  movePtr(current, tmp);

    return oss.str();
  }

  // Destructive XOR (result in current, destroys other)
  std::string xorValues(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(current, tmp)
	<< "["
	<<   zero()
	<<   movePtr(tmp, current)
	<<   "-"
	<<   movePtr(current,tmp)
	<< "]"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(tmp, other)
	<<   "+"
	<<   movePtr(other, tmp)
	<< "]"
	<< movePtr(tmp, other)
	<< "["
	<<   "+"
	<<   zero()
	<<   movePtr(current, tmp)
	<<   "+"
	<<   movePtr(tmp, current)
	<< "]"
	<<  movePtr(current, tmp);

    return oss.str();
  }

  // Destructive NAND (result in current, destroys other)
  std::string nandValues(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));
      
    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< setToValue(2)
	<< movePtr(current, tmp)
	<< "["
	<<   zero()
	<<   movePtr(tmp, current)
	<<   "-"
	<<   movePtr(current,tmp)
	<< "]"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(tmp, other)
	<<   "-"
	<<   movePtr(other, tmp)
	<< "]"
	<< movePtr(tmp, other)
	<< "["
	<<   zero()
	<<   movePtr(current, tmp)
	<<   "+"
	<<   movePtr(tmp, current)
	<< "]"
	<<  movePtr(current, tmp);

    return oss.str();
  }

  // Destructive NOR (result in current, destroys other)
  std::string norValues(int current, int other) {
    assert(util::allDifferent(current, other));
    
    std::ostringstream oss;
    oss << "["
	<<   zero()
	<<   movePtr(other, current)
	<<   setToValue(1)
	<<   movePtr(current, other)
	<< "]+"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(current, other)
	<<   "-"
	<<   movePtr(other, current)
	<< "]"
	<<  movePtr(current, other);

    return oss.str();
  }


  // Add other to current, destroys other
  std::string add(int current, int other) {
    assert(util::allDifferent(current, other));

    std::ostringstream oss;
    oss << movePtr(other, current)
	<< "["
	<<   decrement()
	<<   movePtr(current, other)
	<<   increment()
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(current, other);
      
    return oss.str();
  }

  // Subtract other from current, destroys other
  std::string subtract(int current, int other) {
    assert(util::allDifferent(current, other));

    std::ostringstream oss;
    oss << movePtr(other, current)
	<< "["
	<<   decrement()
	<<   movePtr(current, other)
	<<   decrement()
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(current, other);
      
    return oss.str();
  }

  // Compute current < other and stores the result in current. Other is destroyed
  // Result: 0 if false, any nonzero if true
  std::string less(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(current, tmp)
	<< "["
	<<   "-"
	<<   movePtr(other, current)
	<<   "["
	<<     "-"
	<<     moveValueAssumeZero(other, tmp)
	<<   "]"
	<<   movePtr(tmp, other)
	<<   moveValueAssumeZero(tmp, other)
	<<   movePtr(current, tmp)
	<< "]"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(current, other)
	<<   "+"
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(current, other);
      
    return oss.str();
  }
  
  // Compute current <= other and stores the result in current. Other is destroyed
  // Result: 0 if false, any nonzero if true
  std::string lessOrEqual(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(other, tmp)
	<< "["
	<<   "-"
	<<   movePtr(tmp, other)
	<<   moveValueAssumeZero(tmp, current)
	<<   movePtr(current, tmp)
	<<   "["
	<<     "-"
	<<     moveValueAssumeZero(current, tmp)
	<<   "]"
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(tmp, other)
	<< "["
	<<   zero()
	<<   movePtr(current, tmp)
	<<   "-"
	<<   movePtr(tmp, current)
	<< "]"
	<< movePtr(current, tmp)
	<< "+";
      
    return oss.str();
  }

  // Compute current > other and stores the result in current. Other is destroyed
  // Result: 0 if false, any nonzero if true  
  std::string greater(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(other, tmp)
	<< "["
	<<   "-"
	<<   movePtr(tmp, other)
	<<   moveValueAssumeZero(tmp, current)
	<<   movePtr(current, tmp)
	<<   "["
	<<     "-"
	<<     moveValueAssumeZero(current, tmp)
	<<   "]"
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(tmp, other)
	<< "["
	<<   zero()
	<<   movePtr(current, tmp)
	<<   "+"
	<<   movePtr(tmp, current)
	<< "]"
	<< movePtr(current, tmp);
      
    return oss.str();
  }

  // Compute current >= other and stores the result in current. Other is destroyed
  // Result: 0 if false, any nonzero if true  
  std::string greaterOrEqual(int current, int other, int tmp) {
    assert(util::allDifferent(current, other, tmp));

    std::ostringstream oss;
    oss << movePtr(tmp, current)
	<< zero()
	<< movePtr(current, tmp)
	<< "["
	<<   "-"
	<<   movePtr(other, current)
	<<   "["
	<<     "-"
	<<     moveValueAssumeZero(other, tmp)
	<<   "]"
	<<   movePtr(tmp, other)
	<<   moveValueAssumeZero(tmp, other)
	<<   movePtr(current, tmp)
	<< "]"
	<< "+"
	<< movePtr(other, current)
	<< "["
	<<   zero()
	<<   movePtr(current, other)
	<<   "-"
	<<   movePtr(other, current)
	<< "]"
	<< movePtr(current, other);
      
    return oss.str();
  }


  // Equal
  std::string equal(int current, int other) {
    assert(util::allDifferent(current, other));
    
    std::ostringstream oss;
    oss << "["
    <<   "-"
    <<   movePtr(other, current)
    <<   "-"
    <<   movePtr(current, other)
    << "]"
    << "+"
    << movePtr(other, current)
    << "["
    <<   zero()
    <<   movePtr(current,other)
    <<   "-"
    <<   movePtr(other, current)
    << "]"
    << movePtr(current, other);
    
    return oss.str();
  }

  // Compare to constant value (destructive)
  std::string cmpConst(int value, int current, int tmp) {
    assert(util::allDifferent(current, tmp));
    
    std::ostringstream oss;
    oss << decrement(value)
	<< notValue(current, tmp);
    return oss.str();
  }
}


// Codegen
#define GEN(Name) std::string primitive::Name::generate(Context const &ctx) const

GEN(Comment) {
  return "";
} // TODO: implement check for BF characters and just paste verbatim

GEN(LoopOpen) {
  return "[";
}

GEN(LoopClose) {
  return "]";
}

GEN(MovePointerRelative) {
  return Algorithm::movePtr(amount.resolve(ctx));
}

GEN(ZeroCell) {
  return Algorithm::zero();
}

GEN(ZeroCellPlus) {
  return Algorithm::zeroPlus();
}

GEN(ConstructConstant) {
  if (naive) {
    return Algorithm::setToValue(value.resolve(ctx));
  } else {
    auto const [val, cur, tmp] = defer::resolve(ctx, value, current, scratch);
    return Algorithm::setToValue(val, cur, tmp);
  }
}

GEN(ChangeBy) {
  if (naive) {
    return Algorithm::modify(delta.resolve(ctx));
  } else {
    auto const [del, cur, tmp] = defer::resolve(ctx, delta, current, scratch);
    return Algorithm::modify(del, cur, tmp);
  }
}

GEN(MoveData) {
  auto const [cur, dst] = defer::resolve(ctx, current, dest);
  return Algorithm::moveValue(cur, dst);
}

GEN(CopyData) {
  auto [cur, dst, tmp] = defer::resolve(ctx, current, dest, scratch);
  return Algorithm::copyValue(cur, tmp, dst);
}

GEN(Cmp) {
  auto [val, cur, tmp] = defer::resolve(ctx, value, current, scratch);
  return Algorithm::cmpConst(val, cur, tmp);
}

GEN(Boolean) {
  auto [cur, tmp] = defer::resolve(ctx, current, scratch);
  return Algorithm::boolean(cur, tmp);
}

GEN(Not) {
  auto [cur, tmp] = defer::resolve(ctx, current, scratch);
  return Algorithm::notValue(cur, tmp);
}

GEN(Or) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return Algorithm::orValues(cur, oth);
}

GEN(And) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::andValues(cur, oth, tmp);
}

GEN(Xor) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::xorValues(cur, oth, tmp);
}

GEN(In) {
  return ",";
}

GEN(Out) {
  return ".";
}

GEN(Add) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return Algorithm::add(cur, oth);
}

GEN(Subtract) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return Algorithm::subtract(cur, oth);
}

GEN(Less) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::less(cur, oth, tmp);
}

GEN(LessOrEqual) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::lessOrEqual(cur, oth, tmp);
}

GEN(Greater) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::greater(cur, oth, tmp);
}

GEN(GreaterOrEqual) {
  auto [cur, oth, tmp] = defer::resolve(ctx, current, other, scratch);
  return Algorithm::greaterOrEqual(cur, oth, tmp);
}

GEN(Equal) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return Algorithm::equal(cur, oth);
}

// Merge Rules
#define MERGE(Name) std::shared_ptr<primitive::Node> primitive::Name::merge(Node const *other) const
#define RULE(Other, Result, ...)					\
  if ([[maybe_unused]] auto const *rhs = dynamic_cast<Other const *>(other)) { \
    return std::make_shared<Result>(__VA_ARGS__);			\
  }

MERGE(Equal) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(Boolean, Equal, *this);
  return nullptr;
}

MERGE(MovePointerRelative) {
  RULE(MovePointerRelative, MovePointerRelative, amount + rhs->amount);
  return nullptr;
}

MERGE(ZeroCell) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCell);
  return nullptr;
}

MERGE(ZeroCellPlus) {
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ZeroCell, ZeroCellPlus);
  return nullptr;
}

MERGE(LoopClose) {
  RULE(ZeroCell, LoopClose, tag);
  RULE(ZeroCellPlus, LoopClose, tag);
  return nullptr;
}

MERGE(ConstructConstant) {
  RULE(ChangeBy, ConstructConstant, value + rhs->delta, current, scratch);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(Boolean, ConstructConstant, !!value, current, scratch);
  RULE(Not, ConstructConstant, !value, current, scratch);
  RULE(Cmp, ConstructConstant, (value == rhs->value), current, scratch);
  return nullptr;
}

MERGE(ChangeBy) {
  RULE(ChangeBy, ChangeBy, delta + rhs->delta);
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  return nullptr;
}

MERGE(MoveData) {
  RULE(ZeroCell, MoveData, *this);
  RULE(ZeroCellPlus, MoveData, *this);
  return nullptr;
}

MERGE(CopyData) {
  RULE(ZeroCell, MoveData, current, dest);
  RULE(ZeroCellPlus, MoveData, current, dest);
  return nullptr;
}

MERGE(Cmp) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(Boolean, Cmp, *this);
  return nullptr;
}

MERGE(Boolean) {
  RULE(Boolean, Boolean, *this);
  RULE(Not, Not, current, scratch);
  return nullptr;
}

MERGE(Not) {
  RULE(Boolean, Not, *this);
  RULE(Not, Boolean, current, scratch);
  return nullptr;
}

MERGE(Or) {
  RULE(Boolean, Or, *this);
  return nullptr;
}

MERGE(And) {
  RULE(Boolean, And, *this);
  return nullptr;
}

MERGE(Xor) {
  RULE(Boolean, Xor, *this);
  return nullptr;
}

MERGE(Add) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  return nullptr;
}

MERGE(Subtract) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  return nullptr;
}

MERGE(Less) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(Boolean, Less, *this);
  return nullptr;
}

MERGE(LessOrEqual) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(Boolean, LessOrEqual, *this);
  return nullptr;
}

MERGE(Greater) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(Boolean, Greater, *this);
  return nullptr;
}

MERGE(GreaterOrEqual) {
  RULE(ZeroCell, ZeroCell);
  RULE(ZeroCellPlus, ZeroCellPlus);
  RULE(ConstructConstant, ConstructConstant, *rhs);
  RULE(Boolean, GreaterOrEqual, *this);
  return nullptr;
}


// Textual representation (TODO)
#define TXT(Name) std::string primitive::Name::text(Context const &ctx) const 

TXT(Comment) {
  return txt;
}

TXT(LoopOpen) {
  return "LOOP_START: " + tag;
}

TXT(LoopClose) {
  return "LOOP_END: " + tag;
}

TXT(MovePointerRelative) {
  int const n = amount.resolve(ctx);
  if (n == 0) return "";
  return ((n < 0) ? "LEFT: " : "RIGHT: ") + std::to_string(std::abs(n));
}

TXT(ZeroCell) {
  return "ZERO";
}

TXT(ZeroCellPlus) {
  return "ZERO+";
}

TXT(ConstructConstant) {
  return "CONSTANT: " + std::to_string(value.resolve(ctx));
}

TXT(ChangeBy) {
  int const n = delta.resolve(ctx);
  if (n == 0) return "";
  return ((n > 0) ? "INC: " : "DEC: ") + std::to_string(std::abs(n));
}

TXT(MoveData) {
  auto const [cur, dst] = defer::resolve(ctx, current, dest);
  if (cur == dst) return "";
  return "MOVE: " + std::to_string(dst - cur);
} 

TXT(CopyData) {
  auto const [cur, dst] = defer::resolve(ctx, current, dest);
  if (cur == dst) return "";
  return "COPY: " + std::to_string(dst - cur);
}

TXT(Cmp) {
  int const val = value.resolve(ctx);
  return "CMP: " + std::to_string(val);
} 

TXT(Boolean) {
  return "BOOL";
}

TXT(Not) {
  return "NOT";
} 

TXT(Or) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return "OR: " + std::to_string(oth - cur);
} 

TXT(And) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return "AND: " + std::to_string(oth - cur);
}

TXT(Xor) {
  auto [cur, oth] = defer::resolve(ctx, current, other);
  return "XOR: " + std::to_string(oth - cur);
}

TXT(In) {
  return "IN";
}

TXT(Out) {
  return "OUT";
}

TXT(Add) {
  return "ADD";
} 

TXT(Subtract) {
  return "SUB";
} 

TXT(Less) {
  return "LT";
} 

TXT(LessOrEqual) {
  return "LE";
} 

TXT(Greater) {
  return "GT";
} 

TXT(GreaterOrEqual) {
  return "GE";
}

TXT(Equal) {
  return "EQ";
} 
