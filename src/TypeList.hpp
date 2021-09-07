#ifndef TypeListHPP_HEADER_GUARD
#define TypeListHPP_HEADER_GUARD 1

#include <algorithm>
#include <tuple>
#include <variant>

namespace ListsViaTypes{

	template <typename T, typename... Ts>
	struct Indexer;

	template <typename T, typename... Ts>
	struct Indexer<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

	template <typename T, typename U, typename... Ts>
	struct Indexer<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Indexer<T, Ts...>::value> {};
	


	template<typename ... Types>
	struct TypeList {

		using as_tuple = std::tuple<Types...>;
		using as_variant = std::variant<Types...>;

		template<template<typename ...> typename Functional>
		using apply_to_each = TypeList< Functional<Types> ... >;

		template<template<typename ...> typename Functional>
		using pass_types_as_parameters_to = Functional<Types...>;

		template<size_t N>
		using type_of_index = std::tuple_element_t<N, as_tuple>;

		template<typename T>
		static constexpr size_t get_index_of() {
			return Indexer<T, Types...>::value;
		}

		template<typename T>
		static constexpr bool contains() {
			return std::disjunction_v<std::is_same<T, Types>...>;
		}

	};

	// Declare but do not implement a Concatenate type ...
	template<typename List1, typename List2>
	struct Concatenate;

	// and implement it only when its arguments are TypeLists
	template<typename ... ListElements1, typename ... ListElements2>
	struct Concatenate < TypeList<ListElements1...>, TypeList<ListElements2...> > {

		using type = TypeList<ListElements1..., ListElements2...>;
	};

	template<typename List1, typename List2>
	using ConcatenateLists = typename Concatenate<List1, List2>::type;


	template<typename T, typename ... Ts>
	struct Reverser;

	template<typename T>
	struct Reverser<TypeList<T>> {
		using type = TypeList<T>;
	};

	template<typename T, typename ... Ts>
	struct Reverser< TypeList<T, Ts...> > {
		using type = typename ConcatenateLists< typename Reverser<TypeList<Ts...>>::type, TypeList<T> >;
	};

	template<typename T>
	using ReverseList = typename Reverser<T>::type;

}

#endif
