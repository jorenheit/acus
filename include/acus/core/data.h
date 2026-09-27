// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace acus {

// ============================================================
// MacroCell
// ============================================================

  struct MacroCell {
    enum Field {
      FrameMarker,
      SeekMarker,
      Value0,
      Value1,
      Scratch0,
      Scratch1,
      Flag,
      Payload0,
      Payload1,
    
      FieldCount
    };

    static constexpr MacroCell::Field FirstField =
      static_cast<MacroCell::Field>(0);
  };

// ============================================================
// RuntimePointer
// ============================================================

  struct RuntimePointer {
    enum Field {
      FrameDepth,
      Offset,
      Size // number of logical cells used by pointer
    };
  };

// ============================================================
// Cell
// ============================================================

  struct Cell {
    int offset = 0;
    MacroCell::Field field = MacroCell::Value0;
    operator int() const { return offset; }
  };

// ============================================================
// DataPointer
// ============================================================

  class DataPointer {
    Cell _current;

  public:
    Cell const &current() const { return _current; }
    MacroCell::Field field() const { return _current.field; }
    int offset() const { return _current.offset; }
  
    void moveRelative(int logicalCells) { _current.offset += logicalCells; }
    void set(int offset) { _current.offset = offset; }
    void set(MacroCell::Field field) { _current.field = field; }
    void set(int offset, MacroCell::Field field) { // = MacroCell::Value0) {
      _current.offset = offset;
      _current.field = field;
    }
    void set(Cell cell) { set(cell.offset, cell.field); }
  };

// ============================================================
// Payload
// ============================================================

  class Payload {
  public:  
    enum class Width { None, Single, Double };

  private:
    std::vector<Width> units;

  public:
    template <typename ... Args>
    Payload(Args ... args);
  
    int size() const { return units.size(); }
    operator bool() const { return units.size() > 0; }
    Width width(int index) const;
  
  private:
    void addPairs() {}

    template <typename... Rest>
    void addPairs(int count, Width width, Rest... rest);
  };

// ============================================================
// Temps
// ============================================================

  template <size_t N>
  struct Temps {
    std::array<Cell, N> _cells;

    constexpr Temps() = delete;

    template <typename ... Cells> requires (sizeof...(Cells) == N)
    constexpr Temps(Cells... cells);

    template <size_t I>
    constexpr Cell const &get() const;

    template <size_t ... Is>
    constexpr auto select() const;

    template <typename ... Args> requires (sizeof...(Args) > 0)
    static constexpr Temps select(Args ... args);
  };


  enum class TransferMode {
    Copy,
    Move
  };

  namespace ws {

    namespace impl {
      struct RegionBase {};

      template <size_t N_, typename CellSpec_>
      struct Region: RegionBase {
        static constexpr size_t N = N_;
        using CellSpec = CellSpec_;
      };

    } // impl

    template <int Value = -1>
    struct Data: impl::Region<1, Data<Value>> {
      static constexpr int value = Value;
      static constexpr bool known = Value != -1;
      static_assert(Value == -1 || (Value >= 0 && Value <= 255));
    };
    
    struct Scratch:      impl::Region<1, Scratch> {};
    struct Clobber:   impl::Region<1, Clobber> {};
    struct Untouched: impl::Region<1, Untouched> {};
    
    template <size_t N> struct DataCells:      impl::Region<N, Data<>> {};
    template <size_t N> struct ScratchCells:      impl::Region<N, Scratch> {};
    template <size_t N> struct ClobberCells:   impl::Region<N, Clobber> {};
    template <size_t N> struct UntouchedCells: impl::Region<N, Untouched> {};

    enum class Role {
      ResultLow,
      ResultHigh,
      SignBit,
      NumeratorLow,
      NumeratorHigh,
      DenominatorLow,
      DenominatorHigh,
      QuotientLow,
      QuotientHigh,
      RemainderLow,
      RemainderHigh
    };
    
    template <Role R> struct Prepared: impl::Region<1, Prepared<R>> {};
    
    namespace impl {

      template <typename T>
      concept IsRegion = std::is_base_of_v<impl::RegionBase, T>;

      
      template <IsRegion Used, IsRegion Required>
      struct Satisfies: std::false_type {};

      // Valid conversions

      // Identical regions -> OK
      template <IsRegion T>
      struct Satisfies<T, T>: std::true_type {};

      // Expect any Data, get Scratch -> OK
      template <>
      struct Satisfies<Scratch, Data<>>: std::true_type {};

      // Expect any Data, get specific data -> OK
      template <int V> requires (V != -1)
      struct Satisfies<Data<V>, Data<>> : std::true_type {};

      // Expect any Data, get some prepared value -> OK
      template <Role R>
      struct Satisfies<Prepared<R>, Data<>> : std::true_type {};

      // Clobber contains an arbitrary byte, so it is also valid wherever
      // arbitrary Data<> is sufficient. It is not valid for known Data<V>.
      template <>
      struct Satisfies<Clobber, Data<>> : std::true_type {};

      // Expect Clobber, anything works
      template <IsRegion R> requires (not std::is_same_v<R, Clobber>)
      struct Satisfies<R, Clobber>: std::true_type {};

      // Expect Untouched, anything works
      template <IsRegion R> requires (not std::is_same_v<R, Untouched>)
      struct Satisfies<R, Untouched>: std::true_type {};



      
      class WorkspaceBase {};

      template <typename T>
      concept IsWorkspace = std::is_base_of_v<impl::WorkspaceBase, std::remove_cvref_t<T>>;

      
      template <typename ... Args>
      struct Expanded {
        static constexpr size_t N = sizeof ... (Args);
      };

      template <typename T, size_t N, typename... Pack>
      struct Repeat {
        using Type = typename Repeat<T, N - 1, Pack..., T>::Type;
      };

      template <typename T, typename... Pack>
      struct Repeat<T, 0, Pack...> {
        using Type = ws::impl::Expanded<Pack...>;
      };

      template <typename A, typename B>
      struct Concat;

      template <typename... A, typename... B>
      struct Concat<Expanded<A...>, Expanded<B...>> {
        using Type = Expanded<A..., B...>;
      };

      template <typename... Args>
      struct Expand;

      template <>
      struct Expand<> {
        using Result = Expanded<>;
      };

      template <typename First, typename... Rest>
      requires std::is_base_of_v<ws::impl::RegionBase, First>
      struct Expand<First, Rest...> {
      private:
        using Head = typename Repeat<typename First::CellSpec, First::N>::Type;
        using Tail = typename Expand<Rest...>::Result;

      public:
        using Result = typename Concat<Head, Tail>::Type;
        static constexpr size_t N = Result::N;
      };      

      template <typename Available, typename Required>
      struct CompatibleExpanded;

// Required list exhausted: success.
      template <typename... Available>
      struct CompatibleExpanded<Expanded<Available...>, Expanded<>>
        : std::true_type
      {};

// Available exhausted while requirements remain: failure.
      template <typename Required, typename... Rest>
      struct CompatibleExpanded<Expanded<>, Expanded<Required, Rest...>>
        : std::false_type
      {};

// Compare one cell and recurse.
      template <typename A, typename... As, typename R, typename... Rs>
      struct CompatibleExpanded<Expanded<A, As...>, Expanded<R, Rs...>>
        : std::bool_constant<Satisfies<A, R>::value &&
                             CompatibleExpanded<Expanded<As...>, Expanded<Rs...>>::value>
      {};

      template <typename Available, typename Required>
      inline constexpr bool CompatibleLayouts = CompatibleExpanded<Available, Required>::value;

      template <size_t I, typename ExpandedLayout>
      struct At_;

      template <size_t I, typename... Cells>
      struct At_<I, Expanded<Cells...>> {
        static_assert(I < sizeof...(Cells), "Workspace cell index out of range");
        using Type = std::tuple_element_t<I, std::tuple<Cells...>>;
      };

      template <size_t I, typename ExpandedLayout>
      using At = typename At_<I, ExpandedLayout>::Type;

// Find the first cell at or after Start whose available state satisfies
// Required. Returns npos if no such cell exists.
      static constexpr size_t npos = static_cast<size_t>(-1);

      template <typename ExpandedLayout, IsRegion Required, size_t Start>
      consteval size_t firstSatisfyingCell() {
        if constexpr (Start >= ExpandedLayout::N) {
          return npos;
        }
        else if constexpr (Satisfies<At<Start, ExpandedLayout>, Required>::value) {
          return Start;
        }
        else {
          return firstSatisfyingCell<ExpandedLayout, Required, Start + 1>();
        }
      }

      template <typename Layout, typename Required, size_t First, size_t... I>
      consteval bool rangeSatisfiesImpl(std::index_sequence<I...>) {
        return ( Satisfies<At<First + I, Layout>, Required>::value && ... );
      }

      template <typename Layout, typename Required, size_t First, size_t Last>
      consteval bool rangeSatisfies() {
        static_assert(First <= Last);
        static_assert(Last < Layout::N);
        return rangeSatisfiesImpl<Layout, Required, First>(std::make_index_sequence<Last - First + 1>{});
      }


    } // impl

    template <typename W>
    concept IsWorkspace = impl::IsWorkspace<W>;

    template <impl::IsWorkspace Workspace, impl::IsRegion Required, size_t Start = 0>
    consteval size_t firstSatisfyingCell() {
      using W = std::remove_cvref_t<Workspace>;
      return impl::firstSatisfyingCell<typename W::Layout, Required, Start>();
    }

    template <impl::IsWorkspace Workspace, size_t Start = 0>
    consteval size_t firstScratchCell() {
      return firstSatisfyingCell<Workspace, Scratch, Start>();
    }

    template <typename Workspace, typename Required, size_t Start = 0>
    concept HasSatisfyingCell = impl::IsRegion<Required> &&
    (firstSatisfyingCell<Workspace, Required, Start>() != impl::npos);

    template <typename Workspace, size_t Start = 0>
    concept HasScratchCell = (firstScratchCell<Workspace, Start>() != impl::npos);
    
    template <size_t Index>
    struct At {
      static constexpr size_t index = Index;
      std::string name;
    };
        
    template <size_t N>
    class View {
      using Cells = std::array<Cell, N>;
      using Names = std::array<std::string, N>;
      using Reserved = std::array<bool, N>;

      Cells _cells;
      Names const _initialNames;
      Names _names;
      Reserved const _reserved;

      template <typename ... Args>
      requires (sizeof ... (Args) <= N)
      static Names makeNameArray(Args&& ... names) {

        Names result;
        [[maybe_unused]] size_t index = 0;
	
        ([&]<typename Arg>(size_t &current, Arg const &arg) -> void {
          if constexpr (std::is_convertible_v<std::remove_cvref_t<Arg>, std::string>) {
            result[current++] = arg;
          } else {
            assert(Arg::index > current);
            current = Arg::index;
            result[current++] = arg.name;
          };
        }(index, names), ...);
	  
        return result;
      }

      void validateNames(Names const &names) const {
        for (size_t idx = 0; idx != N; ++idx) {
          assert((names[idx].empty() || not _reserved[idx]) &&
                 "Cannot name a Untouched cell");
        }
      }

      void setName(size_t idx, std::string const &name) {
        assert(idx < N);
        assert((name.empty() || not _reserved[idx]) &&
               "Cannot name a Untouched cell");
        _names[idx] = name;
      }

      template <typename ... WsArgs>
      friend class Workspace;
      
      template <typename ... Args>
      View(Cells const &cells, Reserved const &reserved, Args&& ... names):
        _cells(cells),
        _initialNames(makeNameArray(names...)),
        _names(_initialNames),
        _reserved(reserved)
      {
        validateNames(_names);
      }

      size_t indexOf(std::string const &name) const {
        for (size_t idx = 0; idx != N; ++idx) {
          if (_names[idx] == name) return idx;
        }
        assert(false && "Invalid name");
        std::unreachable();
      }

      template <size_t M>
      friend class View;
      
    public:
      template <size_t M> 
      View<N> &operator=(View<M> const &other) requires (M < N) {
        assert(_cells[0] == other._cells[0]);
        for (size_t idx = 0; idx != M; ++idx) {
          // Preserve existing names where the smaller view leaves a cell unnamed.
          // The destination view keeps the accessibility contract of the
          // workspace from which it was originally constructed.
          std::string const &otherName = other._names[idx];
          if (not otherName.empty()) {
            setName(idx, otherName);
          }
        }
        return *this;
      }

      View<N> &operator=(View<N> const &other) {
        if (this == &other) return *this;
        assert(_cells[0] == other._cells[0]);
        validateNames(other._names);
        _names = other._names;
        return *this;
      }
      
      operator Cell() const requires (N == 1) {
        assert(not _reserved[0] && "Cannot access a Untouched cell");
        return _cells[0];
      }

      template <size_t Index>
      View &rename(std::string const &name) {
        static_assert(Index < N);
        setName(Index, name);
        return *this;
      }

      View &rename(std::string const &oldName, std::string const &newName) {
        setName(indexOf(oldName), newName);
        return *this;
      }

      template <typename ... Args>
      View &renameAll(Args&& ... names) {
        Names renamed = makeNameArray(names...);
        validateNames(renamed);
        _names = std::move(renamed);
        return *this;
      }

      View &reset() {
        _names = _initialNames;
        return *this;
      }
      
      Cell const &operator[](size_t idx) const {
        assert(idx < N);
        assert(not _reserved[idx] && "Cannot access a Untouched cell");
        return _cells[idx];
      }

      template <typename E>
      requires std::is_enum_v<E>
      Cell const &operator[](E e) const {
        return (*this)[static_cast<size_t>(e)];
      }
      
      Cell const &operator[](std::string const &name) const {
        return _cells[indexOf(name)];
      }
    }; // View
    

    template <typename ... Args>
    requires (std::is_base_of_v<impl::RegionBase, Args> && ...)
    struct Layout {};

    // Compile-time description of one cell-type replacement used by
    // Workspace::Transform. Transformations operate on the expanded
    // physical-cell layout, so a replacement always describes exactly
    // one cell.
    template <size_t Index, impl::IsRegion Region>
    struct Replace {
      static_assert(Region::N == 1,
                    "Workspace replacements must describe exactly one cell");
      static constexpr size_t index = Index;
      using Type = Region;
    };
    
    template <typename ... Args>
    class Workspace: impl::WorkspaceBase {
    public:
      using Layout = impl::Expand<Args ...>::Result;
      static constexpr size_t N = Layout::N;
      using Array = std::array<Cell, N>;
      using Reserved = std::array<bool, N>;
    private:
      template <typename ... Ts>
      requires (std::is_base_of_v<impl::RegionBase, Ts> && ...)
      friend auto promise(Cell const &start, ws::Layout<Ts ...>) -> Workspace<Ts ...>;


      template <typename ... Ts>
      requires (std::is_base_of_v<impl::RegionBase, Ts> && ...)
      friend auto promise(int offset, ws::Layout<Ts ...> l) -> Workspace<Ts ...>;
      
      template <typename ...>
      friend class Workspace;

      static Array makeArray(Cell const &start) {
        return [&]<size_t ... I>(std::index_sequence<I...>) -> Array {
          return {
            Cell {
              start.offset,
              static_cast<MacroCell::Field>(start.field + I)
            } ...
          };
        }(std::make_index_sequence<N>{});
      }

      static constexpr Reserved  makeReservedArray() {
        return [&]<size_t ... I>(std::index_sequence<I...>) -> Reserved {
          return {
            std::is_same_v<impl::At<I, Layout>, ws::Untouched> ...
          };
        }(std::make_index_sequence<N>{});
      }
      
      Cell _start;
      Array _cells;
      
      Workspace(Cell const &start) requires (N > 1) :
        _start(start),
        _cells(makeArray(start))
      {}

      template <typename T, size_t Index, size_t Count>
      static consteval bool has() {
        if constexpr (Count == 0) {
          return true;
        } else if constexpr (Index >= N || Count > N - Index) {
          return false;
        } else {
          return impl::rangeSatisfies<Layout, T, Index, Index + Count - 1>();
        }
      }

      template <typename T, size_t Index, size_t Count>
      static constexpr bool Has = has<T, Index, Count>();

      template <size_t Index, typename Original, typename... Replacements>
      struct ReplacementAt {
        using Type = Original;
      };

      template <size_t Index, typename Original, typename First, typename... Rest>
      struct ReplacementAt<Index, Original, First, Rest...> {
        using Type = std::conditional_t<
          First::index == Index,
          typename First::Type,
          typename ReplacementAt<Index, Original, Rest...>::Type
        >;
      };

      template <typename... Replacements>
      static consteval bool uniqueReplacementIndices() {
        constexpr std::array<size_t, sizeof...(Replacements)> indices{Replacements::index...};
        for (size_t i = 0; i < indices.size(); ++i) {
          for (size_t j = i + 1; j < indices.size(); ++j) {
            if (indices[i] == indices[j]) return false;
          }
        }
        return true;
      }

      template <typename... Replacements, size_t... I>
      static auto transformedType(std::index_sequence<I...>)
        -> std::type_identity<Workspace<
          typename ReplacementAt<I, impl::At<I, Layout>, Replacements...>::Type...
        >>;
      
    public:

      template <typename ... Regions>
      requires (impl::IsRegion<Regions> && ...)
      static constexpr bool Satisfies = impl::CompatibleLayouts<Layout, typename Workspace<Regions ...>::Layout>;
      
      template <size_t Index, size_t N = 1>
      static constexpr bool Data = Has<ws::Data<>, Index, N>;

      template <size_t Index, int Value>
      static constexpr bool KnownData = Has<ws::Data<Value>, Index, 1>;
      
      template <size_t Index, size_t N = 1>
      static constexpr bool Scratch = Has<ws::Scratch, Index, N>;

      template <size_t Index, size_t N = 1>
      static constexpr bool Clobber = Has<ws::Clobber, Index, N>;

      template <size_t Start = 0>
      static constexpr size_t ScratchOffset =
        impl::firstSatisfyingCell<Layout, ws::Scratch, Start>();

      template <typename... Replacements>
      struct TransformedType {
        static_assert(((Replacements::index < N) && ...),
                      "Workspace replacement index out of range");
        static_assert(uniqueReplacementIndices<Replacements...>(),
                      "Workspace Transform contains duplicate replacement indices");

        using Type = std::conditional_t<
          sizeof...(Replacements) == 0,
          Workspace<Args...>,
          typename decltype(
            transformedType<Replacements...>(std::make_index_sequence<N>{})
          )::type
        >;
      };

      template <typename... Replacements>
      using Transform = typename TransformedType<Replacements...>::Type;

      // Reify a transformed static contract over the same physical cells.
      // This is intentionally explicit: unlike the normal converting
      // constructor, a transformation may strengthen the contract (for
      // example Data<> -> Prepared<Role>) after an algorithm has established
      // that postcondition.
      template <typename... Replacements>
      Transform<Replacements...> transformed() const {
        return Transform<Replacements...>{_start};
      }

      template <impl::IsWorkspace Other>
      requires Other::template Satisfies<Args ...>
      Workspace(Other const &other):
        _start(other.start()),
        _cells(makeArray(_start))
      {}

      Workspace(Cell const &single) requires (N == 1) :
        _start(single),
        _cells({single})
      {}
        
      Cell const &start() const {
        return _start;
      }

      operator Cell() const requires (N == 1) {
        return _start;
      }

      Array const &cells() const {
        return _cells;
      }

      template <std::size_t M>
      auto cells() const requires (M <= N) {
        return [&]<std::size_t... I>(std::index_sequence<I...>) {
          return std::array<Cell, M>{_cells[I]...};
        }(std::make_index_sequence<M>{});
      }

      template <typename ... NameArgs>
      View<N> view(NameArgs&& ... args) const {
        return {_cells, makeReservedArray(), std::forward<NameArgs>(args)...};
      }
      
      Cell const &operator[](size_t idx) const {
        assert(idx < N);
        return _cells[idx];
      }

      template <typename E>
      requires std::is_enum_v<E>
      Cell const &operator[](E e) const {
        return (*this)[static_cast<size_t>(e)];
      }
      
    }; // Workspace


    // Workspace capability concepts.  Each concept describes what an
    // algorithm may assume about the concrete workspace passed by the caller;
    // no weakening conversion is performed. Refinement is written explicitly
    // so constrained overload resolution can prefer the stronger algorithm.
    template <typename W>
    concept SingleCell =
      impl::IsWorkspace<W> && W::template Data<0>;

    template <typename W>
    concept DoubleCell =
      SingleCell<W> && W::template Data<0, 2>;
    
    template <typename W>
    concept SingleAndScratch =
      SingleCell<W> && W::template ScratchOffset<1> != impl::npos;

    template <typename W>
    concept DoubleAndScratch =
      DoubleCell<W> && SingleAndScratch<W> &&
      W::template ScratchOffset<2> != impl::npos;

    template <typename W>
    concept Inc16Operand =
      DoubleCell<W> && W::template Satisfies<
        DataCells<2>,
        Scratch,
        Untouched,
        Scratch
      >;

    template <typename W>
    concept Add16Operand =
      DoubleAndScratch<W> && W::template Satisfies<
        DataCells<2>,
        ScratchCells<3>
      >;

    template <typename W>
    concept Compare16Lhs =
      DoubleCell<W> && W::template Scratch<2, 2>;

    template <typename W>
    concept Dec16Operand = Inc16Operand<W>;

    template <typename W>
    concept Sub16Operand = Add16Operand<W>;

    template <typename W>
    concept DivModNum =
      SingleCell<W> && W::template Satisfies<
        Data<>,
        Clobber,
        ScratchCells<5>
      >;

    template <typename W>
    concept DivMod16Num =
      DoubleCell<W> && W::template Satisfies<
        DataCells<2>,
        ScratchCells<5>
      >;

    template <typename W>
    concept DivMod16Den =
      DoubleCell<W> && W::template Satisfies<
        DataCells<2>,
        ScratchCells<3>
      >;
    
    template <typename W>
    concept DivModPrepared =
      impl::IsWorkspace<W> && W::template Satisfies<
        Prepared<Role::NumeratorLow>,
        Prepared<Role::DenominatorLow>,
        ScratchCells<4>
      >;

    template <typename W>
    concept SignBitOperand =
      SingleCell<W> && W::template Satisfies<
        Data<>,
        ScratchCells<3>
      >;

    template <typename ... Args>
    requires (std::is_base_of_v<impl::RegionBase, Args> && ...)
    auto promise(Cell const &start, Layout<Args ...>) -> Workspace<Args ...> {
      return {start};
    }

    template <typename ... Args>
    requires (std::is_base_of_v<impl::RegionBase, Args> && ...)
    auto promise(int offset, Layout<Args ...> l) -> Workspace<Args ...> {
      return promise(Cell{offset, MacroCell::Value0}, l);
    }
    
    using Clean8Layout = Layout<Data<>, Clobber, ScratchCells<5>>;
    using Clean16Layout = Layout<Data<>, Data<>, ScratchCells<5>>;
    
    inline auto promiseClean8(Cell const &value0) {
      assert(value0.field == MacroCell::Value0);
      return promise(value0, Clean8Layout{});
    }

    inline auto promiseClean8(int offset) {
      return promiseClean8(Cell{offset, MacroCell::Value0});
    }

    inline auto promiseClean16(Cell const &value0) {
      assert(value0.field == MacroCell::Value0);
      return promise(value0, Clean16Layout{});
    }

    inline auto promiseClean16(int offset) {
      return promiseClean16(Cell{offset, MacroCell::Value0});
    }
    
  } // namespace ws


  
#include "acus/core/data.tpp"

} // namespace acus
