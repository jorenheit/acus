

template <typename T> requires std::convertible_to<T, std::string>
Expr::Expr(T const &str, SUGAR_LOC):
  _expr(literal::string(str)),
  _loc(LOC_FWD)
{}

template <typename T> requires impl::IsSugarType<T>
Expr::Expr(T const &val, SUGAR_LOC):
  _expr(val.toLiteral()),
  _loc(LOC_FWD)
{}

template <typename T> requires impl::IsSugarType<T>
Expr let(std::string const &varName, SUGAR_LOC) {
  return Expr{__assembler.declareLocal(varName, impl::getTypeHandle<T>(), LOC_FWD), LOC_FWD};
}

template <typename T> requires impl::IsSugarType<T>
void global(std::string const &varName, SUGAR_LOC) {
  __assembler.declareGlobal(varName, impl::getTypeHandle<T>(), LOC_FWD);
}


template <typename T>
Expr Expr::cast(SUGAR_LOC) const {
  static_assert(std::is_base_of_v<impl::SugarType, T>,
		"Expression can only be cast to types from the sugar API");

  return Expr{__assembler.cast(get(), impl::getTypeHandle<T>(), LOC_FWD), LOC_FWD};
}

void println(auto&& arg, SUGAR_LOC) {
  print(std::forward<decltype(arg)>(arg), LOC_FWD);
  print('\n', LOC_FWD);
}


template <typename Signature>
class FunctionHandle;

template <typename Ret, typename ... Args>
class FunctionHandle<Ret(Args...)> {
  std::string _functionName;

  template <typename T>
  using ExprConstRef = Expr const &;
  
public:
  using ReturnType = Ret;
  using ArgumentTypes = std::tuple<Args...>;
  static constexpr size_t ArgCount = sizeof...(Args);

  static_assert(impl::IsSugarType<ReturnType>, "Return value must be void or a type from the sugar API");
  static_assert(impl::IsTupleOfSugarTypes<ArgumentTypes>, "Argument types must all be types from the sugar API");
  
  FunctionHandle(std::string const &name);


  auto operator()(ExprConstRef<Args>... args, SUGAR_FUNC) const;

  std::string const &functionName() const;
  types::TypeHandle returnType() const;

private:
  Expr callWithReturn(SUGAR_LOC, auto&& ... args) const;
  void callWithoutReturn(SUGAR_LOC, auto&& ... args) const;
};


template <typename Ret, typename ... Args>
FunctionHandle<Ret(Args...)>::FunctionHandle(std::string const &name):
  _functionName(name)
{}

template <typename Ret, typename ... Args>
auto FunctionHandle<Ret(Args...)>::operator()(ExprConstRef<Args>... args, SUGAR_LOC) const {

  if constexpr (std::is_same_v<Ret, void>) {
    return callWithoutReturn(LOC_FWD, std::forward<decltype(args)>(args)...);
  } else {
    return callWithReturn(LOC_FWD, std::forward<decltype(args)>(args)...);
  }
}

template <typename Ret, typename ... Args>
std::string const &FunctionHandle<Ret(Args...)>::functionName() const { return _functionName; }

template <typename Ret, typename ... Args>
types::TypeHandle FunctionHandle<Ret(Args...)>::returnType() const { return impl::getTypeHandle<Ret>(); }

template <typename Ret, typename ... Args>
Expr FunctionHandle<Ret(Args...)>::callWithReturn(SUGAR_LOC, auto&& ... args) const {
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");
  std::string const retName = impl::nextLabel();

  auto ret = __assembler.declareLocal(retName, returnType(), LOC_FWD);
  auto builder = __assembler.callFunction(_functionName, LOC_FWD).into(ret);
  (builder.arg(Expr(args, LOC_FWD).get()), ...);
  builder.done();
  return Expr{ret, LOC_FWD};
}

template <typename Ret, typename ... Args>
void FunctionHandle<Ret(Args...)>::callWithoutReturn(SUGAR_LOC, auto&& ... args) const {
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");      
  auto builder = __assembler.callFunction(_functionName, LOC_FWD);
  (builder.arg(Expr(args, LOC_FWD).get()), ...);
  builder.done();
}

template <typename Signature>
auto function_(std::string const &functionName, auto&& ... argNames) {
  static_assert(sizeof ... (argNames) == FunctionHandle<Signature>::ArgCount || sizeof ... (argNames) == 0,
		"Invalid number of argument names in function definition.");
  return FunctionBuilder<Signature>{functionName, std::forward<decltype(argNames)>(argNames) ...};
}

template <typename Ret, typename ... Args>
class FunctionBuilder<Ret(Args...)> {

  template <typename>
  using ArgName = std::string const&;  

  using HandleType = FunctionHandle<Ret(Args...)>;
  HandleType _handle;
  std::array<std::string, sizeof ... (Args)> _argNames;
  std::array<types::TypeHandle, sizeof ... (Args)> _argTypes;
  
public:

  FunctionBuilder(std::string const &functionName) requires (sizeof ... (Args) > 0):
    FunctionBuilder(HandleType{functionName})
  {}

  FunctionBuilder(HandleType const &handle) requires (sizeof ... (Args) > 0):
    _handle(handle)
  {}
  
  FunctionBuilder(std::string const &functionName, ArgName<Args>... argNames):
    FunctionBuilder(HandleType{functionName}, argNames...)
  {}
  
  FunctionBuilder(HandleType const &handle, ArgName<Args>... argNames):
    _handle(handle),
    _argNames{argNames ...},
    _argTypes{impl::getTypeHandle<Args>() ...}
  {}

  FunctionBuilder &operator=(FunctionBuilder const &) = delete;
  FunctionBuilder &operator=(FunctionBuilder&&) = delete;

  HandleType &operator|(impl::FunctionDeclaration const &decl) && {
    return _handle;
  }
  
  HandleType &operator|(impl::FunctionDefinition const &body) && {
    
    auto builder = __assembler.function(_handle.functionName(), body.loc());
    builder.ret(_handle.returnType());
    for (size_t i = 0; i != sizeof...(Args); ++i) {
      builder.param(_argNames[i], _argTypes[i]);
    }
    builder.begin();
    
    body();
    endFunction(body.loc());
    return _handle;
  }
};



template <typename Init, typename Condition, typename Increment>
class ForBuilder {
  std::source_location _loc;
  Init _init;
  Condition _condition;
  Increment _increment;

public:
  ForBuilder(Init init, Condition condition, Increment increment, SUGAR_LOC):
    _loc(LOC_FWD),
    _init(std::move(init)),
    _condition(std::move(condition)),
    _increment(std::move(increment))
  {}

  template <typename Body>
  void operator<<(Body&& body) {
    std::string const compareLabel  = impl::nextLabel();
    std::string const loopLabel     = impl::nextLabel(); 
    std::string const continueLabel = impl::nextLabel(); 
    std::string const breakLabel    = impl::nextLabel(); 

    impl::ControlStack::push(continueLabel, breakLabel);
    
    __assembler.scope(_loc).begin();
    {
      _init();
    
      __assembler.label(compareLabel, _loc);
      __assembler.jumpIf(_condition(), loopLabel, breakLabel, _loc);
      __assembler.label(loopLabel, _loc);
    
      body();
    
      __assembler.label(continueLabel, _loc);
    
      _increment();
    
      __assembler.jump(compareLabel, _loc);
      __assembler.label(breakLabel, _loc);
    }
    __assembler.endScope(_loc);

    impl::ControlStack::pop();
  }

  
};

template <typename Condition>
class WhileBuilder {
  std::source_location _loc;
  Condition _condition;

public:
  WhileBuilder(Condition condition, SUGAR_LOC):
    _loc(LOC_FWD),
    _condition(std::move(condition))
  {}

  template <typename Body>
  void operator<<(Body&& body) {
    std::string const compareLabel = impl::nextLabel();
    std::string const startLabel   = impl::nextLabel();
    std::string const breakLabel   = impl::nextLabel();

    impl::ControlStack::push(startLabel, breakLabel);
    
    __assembler.scope(_loc).begin();
    {
      __assembler.label(compareLabel, _loc);
      __assembler.jumpIf(_condition(), startLabel, breakLabel, _loc);
      __assembler.label(startLabel, _loc);
      body();
      __assembler.jump(compareLabel, _loc);
      __assembler.label(breakLabel, _loc);
    }
    __assembler.endScope(_loc);

    impl::ControlStack::pop();
  }
};


template <typename Condition, typename Then>
class IfBuilderStage2;

template <typename Condition>
class IfBuilder {
  std::source_location _loc;
  Condition _condition;

public:
  IfBuilder(Condition condition, SUGAR_LOC):
    _loc(loc),
    _condition(std::move(condition))
  {}

  template <typename Then>
  auto operator<<(Then&& thenBody) {
    return IfBuilderStage2<Condition, std::decay_t<Then>>{
      std::move(_condition),
      std::forward<Then>(thenBody),
      _loc
    };
  }
};

template <typename Condition, typename Then>
class IfBuilderStage2 {
  std::source_location _loc;
  Condition _condition;
  Then _thenBody;
  bool _consumed = false;

public:
  IfBuilderStage2(Condition condition, Then then, SUGAR_LOC):
    _loc(LOC_FWD),
    _condition(std::move(condition)),
    _thenBody(std::move(then))
  {}

  IfBuilderStage2(IfBuilderStage2 const&) = delete;
  IfBuilderStage2& operator=(IfBuilderStage2 const&) = delete;

  IfBuilderStage2(IfBuilderStage2&& other) noexcept
    : _loc(other._loc),
      _condition(std::move(other._condition)),
      _thenBody(std::move(other._thenBody)),
      _consumed(other._consumed)
  {
    other._consumed = true;
  }


  ~IfBuilderStage2() noexcept(false) {
    if (_consumed) return;
    _consumed = true;
    
    // No else body was presented -> execute with only the then-body
    std::string const thenLabel  = impl::nextLabel();
    std::string const breakLabel = impl::nextLabel();

    __assembler.jumpIf(_condition(), thenLabel, breakLabel, _loc);
    __assembler.label(thenLabel, _loc);
    __assembler.scope(_loc).begin();
    _thenBody();
    __assembler.endScope(_loc);
    __assembler.label(breakLabel, _loc);
  }

  
  template <typename Else>
  void operator<<(Else&& elseBody) {
    _consumed = true;
    
    std::string const thenLabel  = impl::nextLabel();
    std::string const elseLabel  = impl::nextLabel();
    std::string const breakLabel = impl::nextLabel();

    __assembler.jumpIf(_condition(), thenLabel, elseLabel, _loc);
    __assembler.label(thenLabel, _loc);
    __assembler.scope(_loc).begin();
    _thenBody();
    __assembler.endScope(_loc);
    __assembler.jump(breakLabel, _loc);
    __assembler.label(elseLabel, _loc);
    __assembler.scope(_loc).begin();
    elseBody();
    __assembler.endScope(_loc);
    __assembler.label(breakLabel, _loc);
  }

  
};


namespace impl {

  template <typename Init, typename Condition, typename Increment>
  auto makeForLoop(Init&& init, Condition&& condition, Increment&& increment, SUGAR_LOC) {
    return ForBuilder{std::forward<Init>(init),
		      std::forward<Condition>(condition),
		      std::forward<Increment>(increment),
		      LOC_FWD};
  }

  template <typename Condition>
  auto makeWhileLoop(Condition&& condition, SUGAR_LOC) {
    return WhileBuilder{std::forward<Condition>(condition), LOC_FWD};
  }

  template <typename Condition>
  auto makeIfStatement(Condition&& condition, SUGAR_LOC) {
    return IfBuilder{std::forward<Condition>(condition), LOC_FWD};
  }

} // impl

