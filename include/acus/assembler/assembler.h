// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <string>
#include <tuple>
#include <vector>
#include <utility>
#include <stack>
#include <optional>
#include <concepts>
#include <unordered_set>

#include "acus/util/util.h"
#include "acus/core/proxy.h"
#include "acus/core/program.h"
#include "acus/core/data.h"
#include "acus/core/builder.h"
#include "acus/core/expression.h"
#include "acus/types/operators.h"
#include "acus/types/typesystem.h"
#include "acus/types/literal_fwd.h"

#define API_HEADER
#include "acus/api/api.h"

namespace acus {

// ============================================================
// Assembler
// ============================================================

class Assembler {
public:
    
  inline Assembler(): _cache(*this) {}
    
  std::string primitives(std::string const &name, API_FUNC) const;
  std::string brainfuck(std::string const &name, API_FUNC) const;

  struct ProgramBuilder;
  struct ScopeBuilder;
  struct FunctionBuilder;
  struct FunctionCallBuilder;

  ProgramBuilder program(std::string const &name, std::string const &entry, API_FUNC);
  FunctionBuilder function(std::string const &name, API_FUNC);
  ScopeBuilder scope(API_FUNC);

  void endProgram(API_FUNC);
  void endFunction(API_FUNC);
  void endScope(API_FUNC);

  Expression declareLocal(std::string const &name, types::TypeHandle type, API_FUNC);
  void declareGlobal(std::string const &name, types::TypeHandle type, API_FUNC);

  void returnFromFunction(auto const &ret, API_FUNC);
  void returnFromFunction(API_FUNC);
  void abortProgram(API_FUNC);

  FunctionCallBuilder callFunction(std::string const &functionName, API_FUNC);
  FunctionCallBuilder callFunctionPointer(auto const &functionPtr, API_FUNC);

  Expression expr(auto const &lhs, API_FUNC);
  Expression assign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression cast(auto const &lhs, types::TypeHandle toType, API_FUNC);
    
  Expression structField(auto const &obj, std::string const &field, API_FUNC);
  Expression structField(auto const &obj, int fieldIndex, API_FUNC);
  Expression dereferencePointer(auto const &ptr, API_FUNC);
  Expression arrayElement(auto const &arr, int index, API_FUNC);  
  Expression arrayElement(auto const &arr, auto const &index, API_FUNC);

  Expression unOp(UnOp op, auto const &rhs, API_FUNC);
  Expression unOpAssign(UnOp op, auto const &rhs, API_FUNC);

  Expression lnotAssign(auto const &rhs, API_FUNC);
  Expression lnot(auto const &rhs, API_FUNC);

  Expression lboolAssign(auto const &rhs, API_FUNC);
  Expression lbool(auto const &rhs, API_FUNC);

  Expression negateAssign(auto const &rhs, API_FUNC);
  Expression negate(auto const &rhs, API_FUNC);

  Expression absAssign(auto const &rhs, API_FUNC);
  Expression abs(auto const &rhs, API_FUNC);

  Expression signBitAssign(auto const &rhs, API_FUNC);
  Expression signBit(auto const &rhs, API_FUNC);
    
  Expression binOp(BinOp op, auto const &lhs, auto const &rhs, API_FUNC);
  Expression binOpAssign(BinOp op, auto const &lhs, auto const &rhs, API_FUNC);

  Expression addAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression subAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression mulAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression divAssign(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression modAssign(auto const &lhs, auto const &rhs, API_FUNC);  

  Expression add(auto const &lhs, auto const &rhs, API_FUNC);
  Expression sub(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression mul(auto const &lhs, auto const &rhs, API_FUNC);
  Expression div(auto const &lhs, auto const &rhs, API_FUNC);
  Expression mod(auto const &lhs, auto const &rhs, API_FUNC);  
  
  Expression landAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lnandAssign(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression lorAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lnorAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lxorAssign(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression lxnorAssign(auto const &lhs, auto const &rhs, API_FUNC);  
  
  Expression land(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression lnand(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression lor(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lnor(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lxor(auto const &lhs, auto const &rhs, API_FUNC);
  Expression lxnor(auto const &lhs, auto const &rhs, API_FUNC);

  Expression eqAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression neqAssign(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression ltAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression leAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression gtAssign(auto const &lhs, auto const &rhs, API_FUNC);
  Expression geAssign(auto const &lhs, auto const &rhs, API_FUNC);  

  Expression eq(auto const &lhs, auto const &rhs, API_FUNC);
  Expression neq(auto const &lhs, auto const &rhs, API_FUNC);  
  Expression lt(auto const &lhs, auto const &rhs, API_FUNC);
  Expression le(auto const &lhs, auto const &rhs, API_FUNC);
  Expression gt(auto const &lhs, auto const &rhs, API_FUNC);
  Expression ge(auto const &lhs, auto const &rhs, API_FUNC);  

  Expression addressOf(auto const &obj, API_FUNC);

  void read(auto const &rhs, API_FUNC);
  void write(auto const &val, API_FUNC);
  void print(auto const &val, API_FUNC);

  void label(std::string const &jumpLabel, API_FUNC);
  void jump(std::string const &jumpLabel, API_FUNC);
  void jumpIf(auto const &condition, std::string const &trueLabel, std::string const &falseLabel, API_FUNC);
  void unreachable(API_FUNC);
  
private:
  friend class proxy::impl::Direct;
  friend class proxy::impl::ArrayElement;
  friend class proxy::impl::StructField;
  friend class proxy::impl::DereferencedPointer;
  friend class proxy::impl::GlobalReference;
  friend class api::impl::Context;

  // program name -> brainfuck output:    
  std::unordered_map<std::string, std::string> _bf; 
  std::unordered_map<std::string, std::string> _txt; 
    
  Program _program;
  Function* _currentFunction = nullptr;
  Function::Block* _currentBlock = nullptr;
  Function::Scope* _currentScope = nullptr;
  primitive::Sequence* _currentSeq = nullptr; 
  std::stack<Cell> _ptrStack;
  DataPointer _dp;
    
  struct {
    bool begun = false;
    bool allowGlobalDeclarations = true;
    bool allowTempAssign = false;
  } _state;

  struct {
    size_t tmpID = 0;
    size_t cacheID = 0;
    size_t scopeID = 0;
  } _counters;
    
  struct MetaBlock {
    std::string name;
    std::string caller;
    std::variant<std::string, types::FunctionType const *> callee;
    types::TypeHandle returnType;
    std::optional<SlotProxy> returnSlot;
    std::string nextBlockName;
  };
  std::vector<MetaBlock> _metaBlocks;

  enum class BuiltinFunction {
    PrintUnsigned8, PrintUnsigned16,
    PrintSigned8, PrintSigned16
  };
  std::unordered_set<BuiltinFunction> _usedBuiltinFunctions;

  struct FunctionCallInfo {
    api::impl::Context API_CTX_NAME;
    std::string callee;
    std::vector<Expression> args;
  };
  std::vector<FunctionCallInfo> _deferredFunctionCallTypeChecks;

  struct LabelCheck {
    api::impl::Context API_CTX_NAME;
    std::string functionName, labelName;
  };
  std::vector<LabelCheck> _deferredLabelChecks;

  class Cache {
    struct Entry;
    using EntryPtr = std::unique_ptr<Entry>;
    using EntryVector = std::vector<EntryPtr>;
    using EntryIterator = EntryVector::iterator;

    EntryVector _entries;
    Assembler& _self;
    bool _flushing = false;
    bool _aliasWriteMode = false;
      
    struct Entry {
      SlotProxy proxy;
      Slot slot;
      bool dirty = false;
      bool pendingWrite = false;
      bool markedForDelete = false;
      Entry *parent = nullptr;
      std::vector<Entry*> children;
    };

    Entry* findEntry(SlotProxy proxy) const;
    Entry* findCachedOwner(SlotProxy proxy) const;
    Entry &findOrCreateEntry(SlotProxy proxy, bool const skipMaterialization = false);
    Entry* ensureParentEntry(SlotProxy proxy);
    void flushSubtree(SlotProxy proxy, TransferMode mode = TransferMode::Copy);
    void flushSubtree(Entry &root, bool const includeRoot, TransferMode mode = TransferMode::Copy);
    void markEntryForDelete(Entry &entry);
    void markSubtreeForDelete(SlotProxy proxy);
    void markSubtreeForDelete(Entry &root, bool const includeRoot);
    void flushAndDeleteSubtree(SlotProxy proxy);
    void flushAndDeleteSubtree(Entry &root, bool const includeRoot);
    void flushEntryIfDirty(Entry &entry, TransferMode mode);
    void deleteMarkedEntries();
    void invalidateDependencies(SlotProxy modifiedProxy);
    void flushAndClearRoots();
    void flushAndClearRoots(auto&& condition);
    void forEntireSubtree(SlotProxy root, auto&& action);
    void forEntireSubtree(Entry& root, bool const sortBeforeAction, auto&& action);      
    void writeAliasSensitive(SlotProxy dest, SlotProxy src, TransferMode mode);
    void writeAliasSensitive(SlotProxy dest, auto&& src);
    void writeDirect(SlotProxy dest, SlotProxy src, TransferMode mode);
    void writeDirect(SlotProxy dest, auto&& src);
    void writeIndirect(SlotProxy dest, auto&& assign);
      
  public:
    inline explicit Cache(Assembler &self): _self(self) {}
    Slot materialize(SlotProxy proxy);
    void write(SlotProxy dest, SlotProxy src, TransferMode mode);
    void write(SlotProxy dest, literal::Literal src);
    void write(SlotProxy dest, std::function<void(Slot )> const &writeInto);

    void freeSlotBoundary(Slot slot);
    void controlBoundary();
    void returnBoundary();
    void reset(); 
    bool empty() const; 
  }; // Cache

  Cache _cache;
  Slot materialize(SlotProxy proxy);
    
  // Diagnostics (assembler_diag.cc)
  std::string currentFunction() const;
  bool programStarted() const;
  bool declaredAsGlobal(std::string const &name) const;
  bool globalDeclarationsAllowed() const;
  bool inScope(std::string const &name) const;
  bool inCurrentScope(std::string const &name) const;
  int currentScopeDepth() const;
  
  // Normalize to RValue or LValue (assembler_rlvalue.cc)
  Expression rValue(Expression val, API_CTX) const;
  Expression rValue(std::string const &var, API_CTX) const;
  Expression rValue(SlotProxy slot, API_CTX) const;
  Expression rValue(literal::Literal val, API_CTX) const;

  Expression lValue(Expression val, API_CTX) const;
  Expression lValue(std::string const &var, API_CTX) const;  
  Expression lValue(SlotProxy slot, API_CTX) const;

  // Block management (assembler_blocks.cc)
  std::string generateUniqueBlockName();
  void beginBlock(std::string const &name);
  void endBlock();    
  void constructMetaBlocks();
  void setTargetBlock(std::string const &f, std::string const &b);
  void setNextBlock(std::string const &f, std::string const &b);
  void setNextBlock(Expression obj);
    
  // Implementation functions for public interface
  void beginProgramImpl(std::string const &name, std::string const &entry, API_CTX);
  void beginFunctionImpl(std::string const &name, types::TypeHandle type, std::vector<std::string> const &params, API_CTX);
  void beginScopeImpl(API_CTX);

  types::TypeHandle defineStructImpl(std::string const& name, std::vector<types::NameTypePair> const &fields, API_CTX);

  void callFunctionImpl(std::string const &functionName, std::optional<Expression> const &returnSlot,
                        std::vector<Expression> const &args, API_CTX);
  void callFunctionImpl(Expression functionPointer, std::optional<Expression> const &returnSlot,
                        std::vector<Expression> const &args, API_CTX);
  void returnFromFunctionImpl(std::optional<Expression> const &ret, API_CTX);
  Expression structFieldImpl(Expression obj, std::string const &field, API_CTX);
  Expression structFieldImpl(Expression obj, int fieldIndex, API_CTX);
  Expression arrayElementImpl(Expression arr, int index, API_CTX);
  Expression arrayElementImpl(Expression arr, Expression index, API_CTX);
  Expression dereferencePointerImpl(Expression ptr, API_CTX);

  Expression addressOfImpl(Expression obj, API_CTX);
  Expression assignImpl(Expression lhs, Expression rhs, API_CTX);
  Expression castImpl(Expression obj, types::TypeHandle toType, API_CTX);
    
  void jumpIfImpl(Expression condition, std::string const &trueLabel, std::string const &falseLabel, API_CTX);
  void writeImpl(Expression rhs, API_CTX); 
  void readImpl(Expression rhs, API_CTX); 
  void printImpl(Expression rhs, API_CTX);
  void writeSlot(Slot slot);
  void readSlot(Slot target);

  void printString(Expression rhs);
  void printStringConst(std::string const &str);
  void printStringSlot(Slot slot);

  void printDecimal(Expression rhs);
  void printDecimalConst(int value);
  void printDecimalSlot(Slot slot);
  void printDecimalSlotUnsigned(Slot slot, bool const destroySlot = false);
  void printDecimalSlotSigned(Slot slot);

  
  // Slot operations
  // template <typename TrueBranch, typename FalseBranch>
  // void branchOnSignBit(Slot slot, Cell const &flagCell, TrueBranch&& trueBranch, FalseBranch&& falseBranch);

  template <typename TrueBranch, typename FalseBranch>
  void branchOnSignBit(Slot slot, TrueBranch&& trueBranch, FalseBranch&& falseBranch);

  void setSlotToBool(Slot slot, bool val);

  std::optional<Slot> localSlot(std::string const &varName) const;
  std::optional<Slot> globalSlot(std::string const &varName) const;
  SlotProxy proxyFromVariableName(std::string const& name, API_CTX) const;
    
  void assignSlot(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);
  void assignSlot(Slot slot, literal::Literal val);
  void assignSlotBytewise(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);
  void assignIntegerSlot(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);

  void setSlotToValue(Slot slot, int value);
    
  void notSlot(Slot rhs);
  void boolSlot(Slot rhs);
  void negateSlot(Slot rhs);
  void absSlot(Slot rhs);
  void signBitSlot(Slot rhs);
  void printIntegerSlotDestructive(Slot valSlot);
    
  // destroyRhs permits consuming RHS values; it never transfers slot ownership.
  // The caller remains responsible for freeing its temporaries.
  void addSlotToSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void addConstToSlot(Slot lhs, int delta);
  void subSlotFromSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void subConstFromSlot(Slot lhs, int delta);

  void mulSlotByConst(Slot lhs, int factor);
  void mulSlotByConstUnsigned(Slot lhs, int factor);
  void mulSlotByConstSigned(Slot lhs, int factor);

  void mulSlotBySlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void mulSlotBySlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
  void mulSlotBySlotSigned(Slot lhs, Slot rhs, bool destroyRhs = false);

  void squareSlot(Slot slot);
  
  void branchIfSlot(Slot slot, std::string const &trueLabel, std::string const &falseLabel);
  void copySlotIntoElement(Slot srcSlot, Slot arrSlot, Slot indexSlot, TransferMode mode = TransferMode::Copy);
  void copyConstIntoElement(literal::Literal const srcSlot, Slot arrSlot, Slot indexSlot);
  void copyElementIntoSlot(Slot elementSlot, Slot arrSlot, Slot indexSlot, TransferMode mode = TransferMode::Copy);
  void rebasePointers(Slot slot, Cell depthDiff, auto &&rebase);
  void rebasePointersToCurrentFrame(Slot slot, Cell frameDepth);
  void rebasePointersToOlderFrame(Slot slot, Cell frameDepth);
  void dereferencePointerIntoSlot(Slot ptrSlot, Slot derefSlot);
  void writeSlotThroughDereferencedPointer(Slot ptrSlot, Slot srcSlot, TransferMode mode = TransferMode::Copy);
  void writeConstThroughDereferencedPointer(Slot ptrSlot, literal::Literal const value);
    
  Slot addressOfSlot(Slot slot, API_CTX);

  void divSlotByConst(Slot lhs, int denom);
  void divSlotByConst(Slot lhs, int denom, Slot modSlot);
  void divSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &modSlot = {});
  void divSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &modSlot = {});

  void modSlotByConst(Slot lhs, int denom);
  void modSlotByConst(Slot lhs, int denom, Slot divSlot);
  void modSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &divSlot = {});
  void modSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &divSlot = {});

  void divSlotBySlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void divSlotBySlot(Slot lhs, Slot rhs, Slot modSlot, bool destroyRhs = false);
  void divSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot = {}, bool const destroyRhs = false);
  void divSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot = {}, bool destroyRhs = false);

  void modSlotBySlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void modSlotBySlot(Slot lhs, Slot rhs, Slot divSlot, bool destroyRhs = false);
  void modSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot = {}, bool const destroyRhs = false);
  void modSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot = {}, bool destroyRhs = false);

  void andSlotWithConst(Slot lhs, int val);
  void andSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void nandSlotWithConst(Slot lhs, int val);
  void nandSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void orSlotWithConst(Slot lhs, int val);
  void orSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void norSlotWithConst(Slot lhs, int val);
  void norSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void xorSlotWithConst(Slot lhs, int val);
  void xorSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void xnorSlotWithConst(Slot lhs, int val);
  void xnorSlotWithSlot(Slot lhs, Slot rhs, bool destroyRhs = false);

  void slotEqualConst(Slot lhs, int val);
  void slotEqualSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void slotNotEqualConst(Slot lhs, int val);
  void slotNotEqualSlot(Slot lhs, Slot rhs, bool destroyRhs = false);

  void slotLessConst(Slot lhs, int val);
  void slotLessConstSigned(Slot lhs, int val);
  void slotLessConstUnsigned(Slot lhs, int val);

  void slotLessEqualConst(Slot lhs, int val);
  void slotLessEqualConstUnsigned(Slot lhs, int val);
  void slotLessEqualConstSigned(Slot lhs, int val);

  void slotGreaterConst(Slot lhs, int val);
  void slotGreaterConstUnsigned(Slot lhs, int val);
  void slotGreaterConstSigned(Slot lhs, int val);

  void slotGreaterEqualConst(Slot lhs, int val);
  void slotGreaterEqualConstSigned(Slot lhs, int val);
  void slotGreaterEqualConstUnsigned(Slot lhs, int val);

  void slotLessSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void slotLessSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
  void slotLessSlotSigned(Slot lhs, Slot rhs, bool destroyRhs = false);

  void slotLessEqualSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void slotLessEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
  void slotLessEqualSlotSigned(Slot lhs, Slot rhs, bool destroyRhs = false);

  void slotGreaterSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void slotGreaterSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
  void slotGreaterSlotSigned(Slot lhs, Slot rhs, bool destroyRhs = false);

  void slotGreaterEqualSlot(Slot lhs, Slot rhs, bool destroyRhs = false);
  void slotGreaterEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
  void slotGreaterEqualSlotSigned(Slot lhs, Slot rhs, bool destroyRhs = false);
    
  // Algorithms: all applied to the current DP (assembler_algorithms.cc)
  void literalBf(std::string const &bf);
  void literalBf(Cell start, std::string const &bf);
  void moveTo(Cell cell);
  void moveTo(int offset, MacroCell::Field field = MacroCell::Value0);
  void moveToOrigin();
  void moveRel(int diff);
  void switchField(MacroCell::Field field);  
  void zeroCell();
  void zeroCellPlus();
  void zeroCell(Cell target);
  void zeroCellPlus(Cell target);
  void loopOpen(std::string const &tag = defaultOpenTag());
  void loopClose(std::string const &tag = defaultCloseTag());

  template <ws::DoubleCell Offset, ws::DynamicMoveWorkspace Work>
  void moveToDynamicOffset(Offset const &offset, Work const &work, TransferMode mode);

  template <ws::DoubleCell Offset>
  void fetchFromDynamicOffset(Offset const &offset, Payload const &payload, primitive::Direction seekDir,
                              TransferMode dataTransferMode, TransferMode offsetTransferMode);
  
  void moveField(Cell from, Cell to);
  void moveField(Cell from, std::vector<Cell> const &to);
  void copyField(Cell from, Cell to, Cell tmp);
  void copyField(Cell from, std::vector<Cell> const &to, Cell tmp);
  void copyOrMoveField(TransferMode mode, Cell from, Cell to, Cell tmp);
  void copyOrMoveField(TransferMode mode, Cell from, std::vector<Cell> const &to, Cell tmp);

  // Workspaces for the implementations
  using SingleCell = ws::Workspace<ws::Data<>>;
  using DoubleCell = ws::Workspace<ws::Data<>, ws::Data<>>;
  
  template <ws::IsWorkspace W>
  using Bool16Result = typename W::template Transform<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >;

  template <ws::IsWorkspace W>
  using Data8Result = typename W::template Transform<
    ws::Replace<0, ws::Data<>>
  >;
  
  template <ws::IsWorkspace W>
  using Data16Result = typename W::template Transform<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<>>
  >;
  
  template <ws::IsWorkspace W>
  using DivModResult = typename W::template Transform<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderLow>>
  >;

  template <ws::IsWorkspace W>
  using DivMod16DigitResult = typename W::template Transform<
    ws::Replace<0, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderHigh>>,
    ws::Replace<5, ws::Prepared<ws::Role::QuotientLow>>
  >;

  template <ws::IsWorkspace W>
  using DivMod16Result = typename W::template Transform<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::QuotientHigh>>,
    ws::Replace<2, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<3, ws::Prepared<ws::Role::RemainderHigh>>
  >;

  template <ws::IsWorkspace W>
  using SignBitResult = typename W::template Transform<
    ws::Replace<0, ws::Prepared<ws::Role::SignBit>>
  >;

  template <ws::IsWorkspace W>
  using SignExtendResult = typename W::template Transform<
    ws::Replace<1, ws::Data<>>
  >;

  // inc/dec (assembler_algorithms.cc)
  SingleCell inc(size_t n = 1);
  SingleCell inc(Cell target, size_t n = 1);
  SingleCell dec(size_t n = 1);
  SingleCell dec(Cell target, size_t n = 1);

  template <ws::SingleCell W>   Data8Result<W>  inc(W const &, size_t n = 1);
  template <ws::Inc16Operand W> Data16Result<W> inc16(W const &);
  template <ws::SingleCell W>   Data8Result<W>  dec(W const &, size_t n = 1);
  template <ws::Dec16Operand W> Data16Result<W> dec16(W const &);

  template <ws::SignExtendOperand W> SignExtendResult<W> signExtend(W const &w, bool const copyLowByte = true);

  // setToValue (assembler_algorithms.{cc,tpp}
  SingleCell setToValue(int value);
  SingleCell setToValue(Cell target, int value);
  
  template <ws::SingleCell W>       Data8Result<W>  setToValue(W const &target, int value);
  template <ws::DoubleCell W>       Data16Result<W> setToValue16(W const &target, int value);
  template <ws::SingleAndScratch W> Data8Result<W>  setToValue(W const &target, int value);
  template <ws::DoubleAndScratch W> Data16Result<W> setToValue16(W const &target, int value);

  // Add (assember_add.{cc,tpp}
  SingleCell addConst(int delta);
  SingleCell addConst(Cell lhs, int delta);

  template <ws::SingleCell W>       Data8Result<W>  addConst(W const &lhs, int delta);
  template <ws::SingleAndScratch W> Data8Result<W>  addConst(W const &lhs, int delta);
  template <ws::SingleCell W>       Data8Result<W>  addDestructive(W const &lhs, SingleCell const &delta);
  template <ws::Add16Operand W>     Data16Result<W> add16Const(W const &lhs, int delta);
  template <ws::Add16Operand W>     Data16Result<W> add16Destructive(W const &lhs, DoubleCell const &delta);

  // Sub (assembler_sub.{cc,tpp})
  SingleCell subConst(int delta);
  SingleCell subConst(Cell lhs, int delta);

  template <ws::SingleCell W>       Data8Result<W>  subConst(W const &lhs, int delta);
  template <ws::SingleCell W>       Data8Result<W>  subDestructive(W const &lhs, SingleCell const &delta);
  template <ws::SingleAndScratch W> Data8Result<W>  subConst(W const &lhs, int delta);
  template <ws::Sub16Operand W>     Data16Result<W> sub16Const(W const &lhs, int delta);
  template <ws::Sub16Operand W>     Data16Result<W> sub16Destructive(W const &lhs, DoubleCell const &delta);

  // Mul (assembler_mul.{cc,tpp}).
  template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
  Result multiplyInto(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work);

  template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
  requires (Result::N == 1)
  Result multiplyInto8(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work);

  template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
  requires (Result::N == 2)
  Result multiplyInto16(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work);

  template <ws::SquareOperand W>   Data8Result<W>  squareDestructive(W const &lhs);
  template <ws::Square16Operand W> Data16Result<W> square16Destructive(W const &lhs);
  

  // DivMod (assembler_divmod.{cc,tpp})
  template <ws::DivModNum N>      DivModResult<N> divModDestructive(N const &num, SingleCell const &denom, TransferMode rhsMode);
  template <ws::DivModPrepared W> DivModResult<W> divModPreparedDestructive(W const &prep);
  template <ws::DivMod16Num N, ws::DivMod16Den D> DivMod16DigitResult<N> divMod16Digit(N const &num, D const &den);
  template <ws::DivMod16Num N, ws::DivMod16Den D> DivMod16Result<N>      divMod16Destructive(N const &num, D const &den);

  // SignBit (assembler_unary.{cc,tpp})
  template <ws::SignBitOperand W>   SignBitResult<W> signBitDestructive(W const &op);
  template <ws::SingleAndScratch W> Data8Result<W>   boolDestructive(W const &op);
  template <ws::Bool16Operand W>    Bool16Result<W>  bool16Destructive(W const &val);

  // Logical functions (assembler_logical.{cc,tpp}
  void boolDestructive(Cell target, Cell tmp);
  void notDestructive(Cell x, Cell tmp);
  void orDestructive(Cell x, Cell y);
  void orDestructive(Cell x, Cell y, Cell tmp);
  void andDestructive(Cell x, Cell y, Cell tmp);
  void xorDestructive(Cell x, Cell y, Cell tmp);
  void nandDestructive(Cell x, Cell y, Cell tmp);
  void norDestructive(Cell x, Cell y, Cell tmp);
  void xnorDestructive(Cell x, Cell y, Cell tmp);

  template <ws::SingleAndScratch W> Data8Result<W>  notDestructive(W const &op);
  template <ws::DoubleCell W>       Bool16Result<W> not16Destructive(W const &op);
  template <ws::BinaryLogic8Operand W> Data8Result<W> orDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::BinaryLogic8Operand W> Data8Result<W> andDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::BinaryLogic8Operand W> Data8Result<W> xorDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::BinaryLogic8Operand W> Data8Result<W> nandDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::BinaryLogic8Operand W> Data8Result<W> norDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::BinaryLogic8Operand W> Data8Result<W> xnorDestructive(W const &lhs, SingleCell const &rhs);

  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R>
  Bool16Result<L> logic16Destructive(L const &lhs, R const &rhs, auto &&logicOp);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> or16Destructive(L const &lhs, R const &rhs);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> and16Destructive(L const &lhs, R const &rhs);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> nor16Destructive(L const &lhs, R const &rhs);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> nand16Destructive(L const &lhs, R const &rhs);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> xor16Destructive(L const &lhs, R const &rhs);
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R> Bool16Result<L> xnor16Destructive(L const &lhs, R const &rhs);

  // Comparisons (assembler_comparisons.{cc,tpp}
  void eqDestructive(Cell x, Cell y);
  void lessDestructive(Cell x, Cell y, Cell tmp);
  void lessOrEqualDestructive(Cell x, Cell y, Cell tmp);
  void greaterDestructive(Cell x, Cell y, Cell tmp);
  void greaterOrEqualDestructive(Cell x, Cell y, Cell tmp);

  template <ws::Compare8Operand W> Data8Result<W> eqDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::Compare8Operand W> Data8Result<W> lessDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::Compare8Operand W> Data8Result<W> lessOrEqualDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::Compare8Operand W> Data8Result<W> greaterDestructive(W const &lhs, SingleCell const &rhs);
  template <ws::Compare8Operand W> Data8Result<W> greaterOrEqualDestructive(W const &lhs, SingleCell const &rhs);

  template <ws::Eq16Operand L>      Bool16Result<L> eq16Destructive(L const &lhs, DoubleCell const &rhs);
  template <ws::Compare16Operand L> Bool16Result<L> less16Destructive(L const &lhs, DoubleCell const &rhs);
  template <ws::Compare16Operand L> Bool16Result<L> lessOrEqual16Destructive(L const &lhs, DoubleCell const &rhs);
  template <ws::Compare16Operand L> Bool16Result<L> greater16Destructive(L const &lhs, DoubleCell const &rhs);
  template <ws::Compare16Operand L> Bool16Result<L> greaterOrEqual16Destructive(L const &lhs, DoubleCell const &rhs);

  // Frame Navigation (assembler_framenav.cc)
  void resetOrigin();
  void pushPtr();
  void popPtr();  
  void pushFrame();
  void popFrame();
  void seek(MacroCell::Field markerField, primitive::Direction dir, Payload const &payload, bool checkCurrent);
  void setSeekMarker();
  void resetSeekMarker();
  void moveToPreviousFrame(Payload const &payload = {});  
  void initializeArguments(primitive::DInt const currentFrameSize, primitive::DInt const paramOffset, std::vector<Expression> const &args, API_CTX);
  void prepareNextFrame(std::string const &functionName, std::vector<Expression> const &args, API_CTX);
  void prepareNextFrame(Expression fptr, std::vector<Expression> const &args, API_CTX);
  void fetchReturnData();
  void fetchReturnData(Slot returnSlot);
  void moveToPointee(Slot ptrSlot);

  // Temporaries and memory management (assembler_memory.cc)
  std::string makeFullName(std::string const &name);
  std::string makeFullGlobalName(std::string const &name);

  bool freeAllSlots(auto&& condition);
    
  void markSlotAvailable(Slot slot);
  void markSlotsAvailable(auto&& condition);
  void markSlotTemp(Slot slot);
  void freeSlot(Slot slot, bool const merge = true);
  void freeTempSlots();
  void freeTempSlot(Slot slot);
  void freeCacheSlots();
  void freeCacheSlot(Slot slot);
    
  void freeScope(Function::Scope const *scope);
  Slot allocSlot(std::string const &name, types::TypeHandle type, SlotData::Kind kind);
  void mergeAvailableSlots();
  Slot getTemp(types::TypeHandle type);
  Slot getTemp(literal::Literal val);
  Slot getCache(types::TypeHandle type);
  Slot getCache(literal::Literal val);
    
  // Global Data Synchronization (assembler_globals.cc)
  void fetchGlobal(Slot globalSlot, Slot localSlot);
  void putGlobal(Slot globalSlot, Slot localSlot, TransferMode mode);
  void putGlobal(Slot globalSlot, literal::Literal const value);

  // Code generation (assembler_codegen.cc)
  std::string builtinFunctionName(BuiltinFunction func);
  void constructBuiltinFunctions();    
  void setTargetSequence(primitive::Sequence *seq);
  primitive::Context constructContext(std::vector<Function::Block *> const &, int) const;    
  primitive::Sequence compilePrimitives(API_CTX);
  static std::string simplifyBrainfuck(std::string const &bf);
  static void mergeSequence(primitive::Sequence &seq);

  // Function call and block name checks
  void functionCallTypeCheck(types::FunctionType const *functionType, std::vector<Expression> const &args, API_CTX);
  void deferFunctionCallTypeCheck(std::string const &callee, std::vector<Expression> const &args, API_CTX);
  void deferredFunctionCallTypeChecks();
  void checkFunctionFlowValidity(Function &fn, API_CTX);

    
  void labelCheck(std::string const &functionName, std::string const &blockName, API_CTX);
  void deferLabelCheck(std::string const &f, std::string const &b, API_CTX);
  void deferredLabelChecks();

  // Unary and Binary Operators
  template <typename Operator> Expression unOpAssignImpl(Expression obj, API_CTX);
  template <typename Operator> Expression unOpImpl(Expression obj, API_CTX);
  template <typename Operator> Expression binOpAssignImpl(Expression lhs, Expression rhs, API_CTX);
  template <typename Operator> Expression binOpImpl(Expression lhs, Expression rhs, API_CTX);
  bool canDestroy(Expression const &expr);
  template <typename Operator> void binOpAssignSlot(Slot const lhs, Slot const rhs, bool destroyRhs = false);
  template <typename Operator> void binOpAssignConst(Slot const lhs, literal::Literal const rhs);

  template <typename Ret> struct UnaryOperator  { using ReturnType = Ret; };
  template <typename Ret> struct BinaryOperator { using ReturnType = Ret; };
 
#define DEFINE_UNARY_OPERATOR(name, type, ret, foldExpr, slotOp)	\
  struct name: UnaryOperator<ret> {                               \
    static ret fold(int x) { return foldExpr; }                   \
    static void applyToSlot(Assembler &self, Slot slot) {         \
      return self.slotOp(slot);                                   \
    }                                                             \
    static UnOp opType() { return type; }                         \
  };
    
  DEFINE_UNARY_OPERATOR(LogicalNot,  UnOp::Not,     bool,  !x,          notSlot);
  DEFINE_UNARY_OPERATOR(LogicalBool, UnOp::Bool,    bool,  !!x,         boolSlot);
  DEFINE_UNARY_OPERATOR(SignBit,     UnOp::SignBit, bool,  x<0,         signBitSlot);
  DEFINE_UNARY_OPERATOR(Negate,      UnOp::Neg,     int,   -x,          negateSlot);
  DEFINE_UNARY_OPERATOR(Abs,         UnOp::Abs,     int,   std::abs(x), absSlot);

#undef DEFINE_UNARY_OPERATOR

#define DEFINE_BINARY_OPERATOR(name, type, ret, foldExpr, slotOp, constOp) \
  struct name: BinaryOperator<ret> {                                    \
    static ret fold(int x, int y) { return foldExpr; }                  \
    static void applyWithSlot(Assembler &self, Slot lhs, Slot rhs, bool destroyRhs) {    \
      return self.slotOp(lhs, rhs, destroyRhs);                                     \
    }                                                                   \
    static void applyWithConst(Assembler &self, Slot lhs, int rhs) {    \
      return self.constOp(lhs, rhs);                                    \
    }                                                                   \
    static BinOp opType() { return type; }                              \
  };

  DEFINE_BINARY_OPERATOR(Add, BinOp::Add, int, x+y, addSlotToSlot,   addConstToSlot);
  DEFINE_BINARY_OPERATOR(Sub, BinOp::Sub, int, x-y, subSlotFromSlot, subConstFromSlot);
  DEFINE_BINARY_OPERATOR(Mul, BinOp::Mul, int, x*y, mulSlotBySlot,   mulSlotByConst);
  DEFINE_BINARY_OPERATOR(Div, BinOp::Div, int, util::math::div(x, y), divSlotBySlot,   divSlotByConst);
  DEFINE_BINARY_OPERATOR(Mod, BinOp::Mod, int, util::math::mod(x, y), modSlotBySlot,   modSlotByConst);

  DEFINE_BINARY_OPERATOR(And,  BinOp::And,  bool, x&&y,    andSlotWithSlot,  andSlotWithConst);
  DEFINE_BINARY_OPERATOR(Nand, BinOp::Nand, bool, !(x&&y), nandSlotWithSlot, nandSlotWithConst);
  DEFINE_BINARY_OPERATOR(Or,   BinOp::Or,   bool, x||y,    orSlotWithSlot,   orSlotWithConst);
  DEFINE_BINARY_OPERATOR(Nor,  BinOp::Nor,  bool, !(x||y), norSlotWithSlot,  norSlotWithConst);
  DEFINE_BINARY_OPERATOR(Xor,  BinOp::Xor,  bool, x!=y,    xorSlotWithSlot,  xorSlotWithConst);
  DEFINE_BINARY_OPERATOR(Xnor, BinOp::Xnor, bool, x==y,    xnorSlotWithSlot, xnorSlotWithConst);

  DEFINE_BINARY_OPERATOR(Eq,  BinOp::Eq,  bool, x==y, slotEqualSlot, slotEqualConst);
  DEFINE_BINARY_OPERATOR(Neq, BinOp::Neq, bool, x!=y, slotNotEqualSlot, slotNotEqualConst);
  DEFINE_BINARY_OPERATOR(Lt,  BinOp::Lt,  bool, x<y,  slotLessSlot, slotLessConst);
  DEFINE_BINARY_OPERATOR(Le,  BinOp::Le,  bool, x<=y, slotLessEqualSlot, slotLessEqualConst);
  DEFINE_BINARY_OPERATOR(Gt,  BinOp::Gt,  bool, x>y,  slotGreaterSlot, slotGreaterConst);
  DEFINE_BINARY_OPERATOR(Ge,  BinOp::Ge,  bool, x>=y, slotGreaterEqualSlot, slotGreaterEqualConst);

#undef DEFINE_BINARY_OPERATOR

  template <typename Operator, typename = void>
  struct BinaryOperatorAfterOperandSwap: Operator {
    static constexpr bool Allowed = false;
  };

  // Commutative Operators
#define COMMUTATIVE_OPERATOR(name)                            \
  template <typename Dummy>                                   \
  struct BinaryOperatorAfterOperandSwap<name, Dummy> : name {	\
    static constexpr bool Allowed = true;                     \
  };
  
  COMMUTATIVE_OPERATOR(Add);
  COMMUTATIVE_OPERATOR(Mul);
  COMMUTATIVE_OPERATOR(And);
  COMMUTATIVE_OPERATOR(Nand);
  COMMUTATIVE_OPERATOR(Or);
  COMMUTATIVE_OPERATOR(Nor);
  COMMUTATIVE_OPERATOR(Xor);
  COMMUTATIVE_OPERATOR(Xnor);
  COMMUTATIVE_OPERATOR(Eq);
  COMMUTATIVE_OPERATOR(Neq);
  
#undef COMMUTATIVE_OPERATOR

#define SWAPPABLE_OPERATOR(name, swapOp)                                \
  template <typename Dummy>                                             \
  struct BinaryOperatorAfterOperandSwap<name, Dummy>: name {            \
    static constexpr bool Allowed = true;                               \
    static name::ReturnType fold(int x, int y) { return swapOp::fold(x, y); } \
    static void applyWithSlot(Assembler &, Slot, Slot, bool) { std::unreachable(); } \
    static void applyWithConst(Assembler &self, Slot slot, int value) { \
      swapOp::applyWithConst(self, slot, value);                        \
    }                                                                   \
  };
    
  SWAPPABLE_OPERATOR(Lt, Gt);
  SWAPPABLE_OPERATOR(Le, Ge);
  SWAPPABLE_OPERATOR(Gt, Lt);
  SWAPPABLE_OPERATOR(Ge, Le);

  // Special case for sub:
  template <typename Dummy>						
  struct BinaryOperatorAfterOperandSwap<Sub, Dummy>: Sub {		
    static constexpr bool Allowed = true;				
    static int fold(int x, int y) { return -Sub::fold(x, y); } 
    static void applyWithSlot(Assembler &, Slot, Slot, bool) { std::unreachable(); } 
    static void applyWithConst(Assembler &self, Slot slot, int value) {
      Sub::applyWithConst(self, slot, value);
      self.negateSlot(slot);
    }									
  };

#undef SWAPPABLE_OPERATOR
    
  // General helpers (inline definitions, assembler_private.tpp)
  void loop(Cell flag, auto&& body);
  void loop(auto&& body);

    
  template <typename Primitive, typename ... Args>
  void emit(Args&& ... args);

  int getFieldIndex(int offset, int field);
  int getFieldIndex(Cell cell);
  
  template <typename... Args> requires ((std::convertible_to<Args, Cell>) && ...)
  auto getFieldIndices(Args... args);

  static std::string defaultOpenTag();
  static std::string defaultCloseTag();  
}; // Assembler

#include "../../../src/assembler/assembler_add.tpp"
#include "../../../src/assembler/assembler_sub.tpp"
#include "../../../src/assembler/assembler_mul.tpp"
#include "../../../src/assembler/assembler_divmod.tpp"
#include "../../../src/assembler/assembler_algorithms.tpp"
#include "../../../src/assembler/assembler_unary.tpp"
#include "../../../src/assembler/assembler_logical.tpp"
#include "../../../src/assembler/assembler_comparisons.tpp"


// Builder objects for programs, functions, blocks, and calls

struct Assembler::ProgramBuilder: builder::BuilderBase {

  void begin();
  ProgramBuilder(Assembler &a, std::string const &name, std::string const &entry, api::impl::Context const &ctx);
  
private:
  Assembler& _assembler;
  std::string _name;
  std::string _entry;

}; // ProgramBuilder

  
struct Assembler::FunctionBuilder: builder::BuilderBase {

  FunctionBuilder & ret(types::TypeHandle returnType) &;
  FunctionBuilder && ret(types::TypeHandle returnType) &&;  
  FunctionBuilder & param(std::string const &varName, types::TypeHandle varType) &;
  FunctionBuilder && param(std::string const &varName, types::TypeHandle varType) &&;
  
  void begin();
  FunctionBuilder(Assembler &a, std::string const &functionName, api::impl::Context const &ctx);
  
private:
  Assembler& _assembler;
  types::TypeHandle _returnType = types::null;
  std::string _functionName;
  std::vector<std::pair<std::string, types::TypeHandle>> _params;

}; // FunctionBuilder


struct Assembler::ScopeBuilder: builder::BuilderBase {

  void begin();
  ScopeBuilder(Assembler &a, api::impl::Context const &ctx);
  
private:
  Assembler& _assembler;

}; // ScopeBuilder
  
  
struct Assembler::FunctionCallBuilder: builder::BuilderBase {

  // Templates: implemented in assembler_assemblers.tpp
  FunctionCallBuilder & into(auto&& result) &;
  FunctionCallBuilder && into(auto&& result) &&;
  FunctionCallBuilder &arg(auto&& arg) &;
  FunctionCallBuilder && arg(auto&& arg) &&;
  
  void done();
  FunctionCallBuilder(Assembler &a, auto const &function, api::impl::Context const &ctx);  
  
private:
  Assembler& _assembler;
  std::variant<std::string, Expression> _function;
  std::optional<Expression> _result;
  std::vector<Expression> _args;

}; // FunctionCallBuilder

#include "acus/assembler/assembler_builders.tpp"
#include "acus/assembler/assembler_public.tpp"

} // namespace acus
