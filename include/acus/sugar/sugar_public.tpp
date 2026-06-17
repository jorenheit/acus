
template <typename T> requires impl::IsSugarType<T>
Expr::Expr(T const &val): _expr(val.toLiteral()) {}

template <typename T> requires impl::IsSugarType<T>
Expr let(std::string const &varName) {
  return __assembler.declareLocal(varName, impl::getTypeHandle<T>());
}

template <typename T>
Expr Expr::cast() const {
  static_assert(std::is_base_of_v<impl::SugarType, T>, "Expression can only be cast to types from the sugar API");
  return __assembler.cast(get(), impl::getTypeHandle<T>());
}

void println(auto&& arg) {
  print(std::forward<decltype(arg)>(arg));
  print('\n');
}


template <typename Signature>
class FunctionHandle {
  std::string _functionName;

public:
  using Ret = typename impl::FunctionType<Signature>::ReturnType;
  using ArgumentTypes = typename impl::FunctionType<Signature>::ArgumentTypes;

  FunctionHandle(std::string const &name);
  auto operator()(auto&&...args) const;
  std::string const &functionName() const;
  types::TypeHandle returnType() const;

private:
  Expr callWithReturn(auto&& ... args) const;
  void callWithoutReturn(auto&& ... args) const;
};


template <typename Signature>
FunctionHandle<Signature>::FunctionHandle(std::string const &name):
  _functionName(name)
{}

template <typename Signature>
auto FunctionHandle<Signature>::operator()(auto&&...args) const {
  if constexpr (std::is_same_v<Ret, void>) {
    return callWithoutReturn(std::forward<decltype(args)>(args)...);
  } else {
    return callWithReturn(std::forward<decltype(args)>(args)...);
  }
}

template <typename Signature>
std::string const &FunctionHandle<Signature>::functionName() const { return _functionName; }

template <typename Signature>  
types::TypeHandle FunctionHandle<Signature>::returnType() const { return impl::getTypeHandle<Ret>(); }

template <typename Signature>    
Expr FunctionHandle<Signature>::callWithReturn(auto&& ... args) const {
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");
  std::string const retName = "__sugar_ret_" + std::to_string(impl::LabelCount::count++);
  auto ret = __assembler.declareLocal(retName, returnType());
  auto builder = __assembler.callFunction(_functionName).into(ret);
  (builder.arg(Expr{args}.get()), ...);
  builder.done();
  return ret;
}

template <typename Signature>
void FunctionHandle<Signature>::callWithoutReturn(auto&& ... args) const {
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");      
  auto builder = __assembler.callFunction(_functionName);
  (builder.arg(Expr{args}.get()), ...);
  builder.done();
}

template <typename Signature>
FunctionHandle<Signature> function_fwd(std::string const &name) {
  return { name };
}

template <typename Signature>
FunctionHandle<Signature> call(std::string const &name) {
  return { name };
}

template <typename Signature, typename ... ArgNames>
FunctionHandle<Signature> function(std::string const &name, ArgNames&& ... args) {
  using Sig = impl::FunctionType<Signature>;
  using Ret = typename Sig:: ReturnType;
  using ArgumentTypes = typename Sig::ArgumentTypes;
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");

  auto builder = __assembler.function(name);
  builder.ret(impl::getTypeHandle<Ret>());

  [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    (builder.param(std::forward<ArgNames>(args),
		   impl::getTypeHandle<std::tuple_element_t<Is, ArgumentTypes>>()), ... );
  }(std::index_sequence_for<ArgNames...>{});    

  builder.begin();
  return FunctionHandle<Signature>(name);
}

template <typename Signature, typename ... ArgNames>
FunctionHandle<Signature> function(FunctionHandle<Signature> const &caller, ArgNames&& ... args) {

  using ArgumentTypes = typename FunctionHandle<Signature>::ArgumentTypes;
  static_assert(std::tuple_size_v<ArgumentTypes> == sizeof ... (args),
		"Function signature does not match number of arguments.");

  auto builder = __assembler.function(caller.functionName());
  builder.ret(caller.returnType());

  [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    (builder.param(std::forward<ArgNames>(args),
		   impl::getTypeHandle<std::tuple_element_t<Is, ArgumentTypes>>()), ... );
  }(std::index_sequence_for<ArgNames...>{});    

  builder.begin();
  return FunctionHandle<Signature>(caller.functionName());
}


template <typename Init, typename Condition, typename Increment, typename Body>
void forLoop(Init&& init, Condition&& condition, Increment&& inc, Body&& body) {
  std::string const compareLabel = "__sugar_label__" + std::to_string(impl::LabelCount::count++);
  std::string const loopLabel = "__sugar_label__" + std::to_string(impl::LabelCount::count++);
  std::string const breakLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);

  __assembler.scope().begin();
  init();
  __assembler.label(compareLabel);
  __assembler.jumpIf(condition(), loopLabel, breakLabel);
  __assembler.label(loopLabel);
  body();
  inc();
  __assembler.jump(compareLabel);
  __assembler.label(breakLabel);
  __assembler.endScope();
}


template <typename Condition, typename Body>
void whileLoop(Condition&& condition, Body&& body) {
  std::string const compareLabel = "__sugar_label__" + std::to_string(impl::LabelCount::count++);
  std::string const startLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);
  std::string const breakLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);

  __assembler.scope().begin();
  __assembler.label(compareLabel);
  __assembler.jumpIf(condition(), startLabel, breakLabel);
  __assembler.label(startLabel);
  body();
  __assembler.jump(compareLabel);
  __assembler.label(breakLabel);
  __assembler.endScope();
}

template <typename Condition, typename Then, typename Else>
void ifCondition(Condition&& cond, Then&& thenBody, Else&& elseBody) {
  std::string const thenLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);
  std::string const elseLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);
  std::string const breakLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);

  __assembler.jumpIf(cond(), thenLabel, elseLabel);
  __assembler.label(thenLabel);
  __assembler.scope().begin();
  thenBody();
  __assembler.endScope();
  __assembler.jump(breakLabel);
  __assembler.label(elseLabel);
  __assembler.scope().begin();
  elseBody();
  __assembler.endScope();
  __assembler.label(breakLabel);
}


template <typename Condition, typename Then>
void ifCondition(Condition&& cond, Then&& thenBody) {
  std::string const thenLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);
  std::string const breakLabel = "__sugar_label_" + std::to_string(impl::LabelCount::count++);

  __assembler.jumpIf(cond(), thenLabel, breakLabel);
  __assembler.label(thenLabel);
  __assembler.scope().begin();
  thenBody();
  __assembler.endScope();
  __assembler.label(breakLabel);
}
