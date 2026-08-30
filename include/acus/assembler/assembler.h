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
    template <typename TrueBranch, typename FalseBranch>
    void branchOnSignBit(Slot slot, Cell const &flagCell, TrueBranch&& trueBranch, FalseBranch&& falseBranch);
    void setSlotToBool(Slot slot, bool val);

    std::optional<Slot> localSlot(std::string const &varName) const;
    std::optional<Slot> globalSlot(std::string const &varName) const;
    SlotProxy proxyFromVariableName(std::string const& name, API_CTX) const;
    
    void assignSlot(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);
    void assignSlot(Slot slot, literal::Literal val);
    void assignSlotBytewise(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);
    void assignIntegerSlot(Slot dest, Slot src, TransferMode mode = TransferMode::Copy);

    void notSlot(Slot rhs);
    void boolSlot(Slot rhs);
    void negateSlot(Slot rhs);
    void absSlot(Slot rhs);
    void signBitSlot(Slot rhs);
    void printIntegerSlotDestructive(Slot valSlot);
    
    void addSlotToSlot(Slot lhs, Slot rhs);
    void addConstToSlot(Slot lhs, int delta);
    void subSlotFromSlot(Slot lhs, Slot rhs);
    void subConstFromSlot(Slot lhs, int delta);

    void mulSlotByConst(Slot lhs, int factor);
    void mulSlotByConstUnsigned(Slot lhs, int factor);
    void mulSlotByConstSigned(Slot lhs, int factor);

    void mulSlotBySlot(Slot lhs, Slot rhs);
    void mulSlotBySlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
    void mulSlotBySlotSigned(Slot lhs, Slot rhs);

    void divSlotByConst(Slot lhs, int denom);
    void divSlotByConst(Slot lhs, int denom, Slot modSlot);
    void divSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &modSlot = {});
    void divSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &modSlot = {});

    void modSlotByConst(Slot lhs, int denom);
    void modSlotByConst(Slot lhs, int denom, Slot divSlot);
    void modSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &divSlot = {});
    void modSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &divSlot = {});

    void divSlotBySlot(Slot lhs, Slot rhs);
    void divSlotBySlot(Slot lhs, Slot rhs, Slot modSlot);
    void divSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot = {}, bool const destroyRhs = false);
    void divSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot = {});

    void modSlotBySlot(Slot lhs, Slot rhs);
    void modSlotBySlot(Slot lhs, Slot rhs, Slot divSlot);
    void modSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot = {}, bool const destroyRhs = false);
    void modSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot = {});

    void andSlotWithConst(Slot lhs, int val);
    void andSlotWithSlot(Slot lhs, Slot rhs);
    void nandSlotWithConst(Slot lhs, int val);
    void nandSlotWithSlot(Slot lhs, Slot rhs);
    void orSlotWithConst(Slot lhs, int val);
    void orSlotWithSlot(Slot lhs, Slot rhs);
    void norSlotWithConst(Slot lhs, int val);
    void norSlotWithSlot(Slot lhs, Slot rhs);
    void xorSlotWithConst(Slot lhs, int val);
    void xorSlotWithSlot(Slot lhs, Slot rhs);
    void xnorSlotWithConst(Slot lhs, int val);
    void xnorSlotWithSlot(Slot lhs, Slot rhs);

    void slotEqualConst(Slot lhs, int val);
    void slotEqualSlot(Slot lhs, Slot rhs);
    void slotNotEqualConst(Slot lhs, int val);
    void slotNotEqualSlot(Slot lhs, Slot rhs);

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

    void slotLessSlot(Slot lhs, Slot rhs);
    void slotLessSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
    void slotLessSlotSigned(Slot lhs, Slot rhs);

    void slotLessEqualSlot(Slot lhs, Slot rhs);
    void slotLessEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
    void slotLessEqualSlotSigned(Slot lhs, Slot rhs);

    void slotGreaterSlot(Slot lhs, Slot rhs);
    void slotGreaterSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
    void slotGreaterSlotSigned(Slot lhs, Slot rhs);

    void slotGreaterEqualSlot(Slot lhs, Slot rhs);
    void slotGreaterEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs = false);
    void slotGreaterEqualSlotSigned(Slot lhs, Slot rhs);
  
    void branchIfSlot(Slot slot, std::string const &trueLabel, std::string const &falseLabel);
    void copySlotIntoElement(Slot srcSlot, Slot arrSlot, Slot indexSlot, TransferMode mode = TransferMode::Copy);
    void copyConstIntoElement(literal::Literal const srcSlot, Slot arrSlot, Slot indexSlot);
    void copyElementIntoSlot(Slot elementSlot, Slot arrSlot, Slot indexSlot, TransferMode mode = TransferMode::Copy);
    void dereferencePointerIntoSlot(Slot ptrSlot, Slot derefSlot);
    void writeSlotThroughDereferencedPointer(Slot ptrSlot, Slot srcSlot, TransferMode mode = TransferMode::Copy);
    void writeConstThroughDereferencedPointer(Slot ptrSlot, literal::Literal const value);
    
    Slot addressOfSlot(Slot slot, API_CTX);
  
    // Algorithms: all applied to the current DP (assembler_algorithms.cc)
    void moveTo(Cell cell);
    void moveTo(int offset, MacroCell::Field field = MacroCell::Value0);
    void moveToOrigin();
    void moveRel(int diff);
    void switchField(MacroCell::Field field);  
    void zeroCell();
    void zeroCellPlus();
    void loopOpen(std::string const &tag = defaultOpenTag());
    void loopClose(std::string const &tag = defaultCloseTag());

    void moveToDynamicOffset(Cell offsetLow, Cell offsetHigh, TransferMode mode = TransferMode::Copy);
    void fetchFromDynamicOffset(Cell offsetLow, Cell offsetHigh, Payload const &payload, primitive::Direction seekDir,
				TransferMode dataTransferMode, TransferMode offsetTransferMode);
  
    void moveField(Cell dest);
    void copyField(Cell dest, Temps<1>);
    void copyOrMoveField(TransferMode ode, Cell dest, Temps<1>);
    
    void setToValue(int value);
    void setToValue(int value, Temps<1>);
    void setToValue16(int value, Cell high);    
    void setToValue16(int value, Cell high, Temps<1>);

    void inc();
    void dec();
    void inc16(Cell high, Temps<2>);
    void dec16(Cell high, Temps<2>);
  
    void signBitDestructive(Temps<3>);
    void signBitConstructive(Cell result, Temps<4>);
    
    void negateDestructive(Temps<2>);
    void negateConstructive(Cell result, Temps<2>);
    void negate16Destructive(Cell high, Temps<6>);
    void negate16Constructive(Cell high, Cell result, Temps<7>);
    
    void addConst(int delta);
    void addConstAndCarry(int delta, Cell carry, Temps<3>);
    void add16Const(int delta, Cell high, Temps<4>);
    void addDestructive(Cell other);
    void addConstructive(Cell result, Cell other, Temps<2>);
    void add16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void add16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell otherLow, Cell otherHigh, Temps<6>);
    void addAndCarryDestructive(Cell carry, Cell other, Temps<3>);
    void addAndCarryConstructive(Cell result, Cell carry, Cell other, Temps<4>);

    void subConst(int delta);
    void subConstAndCarry(int delta, Cell carry, Temps<3>);
    void sub16Const(int delta, Cell high, Temps<4>);
    void subDestructive(Cell other);
    void subConstructive(Cell result, Cell other, Temps<2>);
    void sub16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void sub16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell otherLow, Cell otherHigh, Temps<6>);
    void subAndCarryDestructive(Cell carry, Cell other, Temps<3>);
    void subAndCarryConstructive(Cell result, Cell carry, Cell other, Temps<4>);

    void mulConst(int factor, Temps<3>);
    void mul16Const(int factor, Cell high, Temps<8>);
    void mulDestructive(Cell other, Temps<3>);
    void mulConstructive(Cell result, Cell factor, Temps<4>);
    void mul16Destructive(Cell high, Cell factorLow, Cell factorHigh, Temps<9>);
    void mul16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell factorLow, Cell factorHigh, Temps<11>);

    void divModConst(int denom, Cell modResult, Temps<5>);
    void divMod16Const(int denom, Cell high, Cell modResultLow, Cell modResultHigh, Temps<8>);
    void divModDestructive(Cell denom, Cell modResult, Temps<5>);
    void divModConstructive(Cell result, Cell denom, Cell modResult, Temps<6>);
    void divMod16Destructive(Cell high, Cell denomLow, Cell denomHigh, Cell modResultLow, Cell modResultHigh, Temps<8>);
    void divMod16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell denomLow, Cell denomHigh, Cell modResultLow, Cell modResultHigh, Temps<12>);

    void boolDestructive(Temps<1>);
    void boolConstructive(Cell result, Temps<1>);
    void bool16Destructive(Cell high, Temps<1>);
    void bool16Constructive(Cell high, Cell result, Temps<2>);
  
    void notDestructive(Temps<1>);
    void notConstructive(Cell result, Temps<1>);
    void not16Destructive(Cell high, Temps<1>);
    void not16Constructive(Cell high, Cell result, Temps<2>);

    void orDestructive(Cell other, Temps<1>);
    void orConstructive(Cell result, Cell other, Temps<2>);
    void or16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1>);
    void or16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4>);

    void norDestructive(Cell other, Temps<1>);
    void norConstructive(Cell result, Cell other, Temps<2>);
    void nor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1>);
    void nor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4>);
  
    void andDestructive(Cell other, Temps<1>);
    void andConstructive(Cell result, Cell other, Temps<2>);
    void and16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1>);
    void and16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4>);

    void nandDestructive(Cell other, Temps<1>);
    void nandConstructive(Cell result, Cell other, Temps<2>);
    void nand16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1>);
    void nand16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4>);

    void xorDestructive(Cell other, Temps<2>);
    void xorConstructive(Cell result, Cell other, Temps<3>);
    void xor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<2>);
    void xor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<5>);

    void xnorDestructive(Cell other, Temps<2>);
    void xnorConstructive(Cell result, Cell other, Temps<3>);
    void xnor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<2>);
    void xnor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<5>);

    void compareToConstDestructive(int value, Temps<1>);
    void compareToConstConstructive(int value, Cell result, Temps<1>);    
    void compare16ToConstDestructive(int value, Cell high, Temps<1>);
    void compare16ToConstConstructive(int value, Cell high, Cell result, Temps<2>);    

    void eqDestructive(Cell other, Temps<1>);
    void eqConstructive(Cell result, Cell other, Temps<1>);
    void eq16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1>);
    void eq16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4>);
  
    void lessDestructive(Cell other, Temps<2>);
    void lessConstructive(Cell result, Cell other, Temps<3>);
    void less16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void less16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8>);

    void lessOrEqualDestructive(Cell other, Temps<2>);
    void lessOrEqualConstructive(Cell result, Cell other, Temps<3>);
    void lessOrEqual16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void lessOrEqual16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8>);

    void greaterDestructive(Cell other, Temps<2>);
    void greaterConstructive(Cell result, Cell other, Temps<3>);
    void greater16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void greater16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8>);

    void greaterOrEqualDestructive(Cell other, Temps<2>);
    void greaterOrEqualConstructive(Cell result, Cell other, Temps<3>);
    void greaterOrEqual16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4>);
    void greaterOrEqual16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8>);

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
    template <typename Operator> void binOpAssignSlot(Slot const lhs, Slot const rhs);
    template <typename Operator> void binOpAssignConst(Slot const lhs, literal::Literal const rhs);

    template <typename Ret> struct UnaryOperator  { using ReturnType = Ret; };
    template <typename Ret> struct BinaryOperator { using ReturnType = Ret; };
 
#define DEFINE_UNARY_OPERATOR(name, type, ret, foldExpr, slotOp)	\
    struct name: UnaryOperator<ret> {					\
      static ret fold(int x) { return foldExpr; }			\
      static void applyToSlot(Assembler &self, Slot slot) {		\
	return self.slotOp(slot);					\
      }									\
      static UnOp opType() { return type; }				\
    };
    
    DEFINE_UNARY_OPERATOR(LogicalNot,  UnOp::Not,     bool,  !x,          notSlot);
    DEFINE_UNARY_OPERATOR(LogicalBool, UnOp::Bool,    bool,  !!x,         boolSlot);
    DEFINE_UNARY_OPERATOR(SignBit,     UnOp::SignBit, bool,  x<0,         signBitSlot);
    DEFINE_UNARY_OPERATOR(Negate,      UnOp::Neg,     int,   -x,          negateSlot);
    DEFINE_UNARY_OPERATOR(Abs,         UnOp::Abs,     int,   std::abs(x), absSlot);

#undef DEFINE_UNARY_OPERATOR

#define DEFINE_BINARY_OPERATOR(name, type, ret, foldExpr, slotOp, constOp) \
    struct name: BinaryOperator<ret> {					\
      static ret fold(int x, int y) { return foldExpr; }		\
      static void applyWithSlot(Assembler &self, Slot lhs, Slot rhs) { \
	return self.slotOp(lhs, rhs);					\
      }									\
      static void applyWithConst(Assembler &self, Slot lhs, int rhs) { \
	return self.constOp(lhs, rhs);					\
      }									\
      static BinOp opType() { return type; }				\
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
#define COMMUTATIVE_OPERATOR(name)				\
    template <typename Dummy>					\
    struct BinaryOperatorAfterOperandSwap<name, Dummy> : name {	\
      static constexpr bool Allowed = true;			\
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

#define SWAPPABLE_OPERATOR(name, swapOp)				\
    template <typename Dummy>						\
    struct BinaryOperatorAfterOperandSwap<name, Dummy>: name {		\
      static constexpr bool Allowed = true;				\
      static name::ReturnType fold(int x, int y) { return swapOp::fold(x, y); } \
      static void applyWithSlot(Assembler &, Slot, Slot) { std::unreachable(); } \
      static void applyWithConst(Assembler &self, Slot slot, int value) { \
	swapOp::applyWithConst(self, slot, value);			\
      }									\
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
      static void applyWithSlot(Assembler &, Slot, Slot) { std::unreachable(); } 
      static void applyWithConst(Assembler &self, Slot slot, int value) {
	Sub::applyWithConst(self, slot, value);
	self.negateSlot(slot);
      }									
    };

#undef SWAPPABLE_OPERATOR
    
    // General helpers (inline definitions, assembler_private.tpp)
    template <typename Primitive, typename ... Args>
    void emit(Args&& ... args);

    int getFieldIndex(int offset, int field);
    int getFieldIndex(Cell cell);
  
    template <typename... Args> requires ((std::convertible_to<Args, Cell>) && ...)
    auto getFieldIndices(Args... args);

    static std::string defaultOpenTag();
    static std::string defaultCloseTag();  
  };


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
