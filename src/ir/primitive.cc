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


} // namespace acus::Algorithm


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

GEN(Inline) {
  return code;
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

GEN(In) {
  return ",";
}

GEN(Out) {
  return ".";
}

// Merge Rules
#define MERGE(Name) std::shared_ptr<primitive::Node> primitive::Name::merge(Node const *other) const
#define RULE(Other, Result, ...)                                        \
  if ([[maybe_unused]] auto const *rhs = dynamic_cast<Other const *>(other)) { \
    return std::make_shared<Result>(__VA_ARGS__);                       \
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


// Textual representation (TODO)
#define TXT(Name) std::string primitive::Name::text(Context const &ctx) const

TXT(Comment) { return txt; }

TXT(Inline) {
  return "INLINE(" + code + ")";
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

TXT(In) {
  return "IN";
}

TXT(Out) {
  return "OUT";
}
