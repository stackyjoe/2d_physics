#pragma once

#include <bitset>

#include <fmt\format.h>

#include "TypeList.hpp"

using namespace ListsViaTypes;

template<typename T>
class tuple_of_optionals;


template<typename ...Ts>
class tuple_of_optionals<TypeList<Ts...>> {
public:
	tuple_of_optionals() = default;
	tuple_of_optionals(Ts && ... ts)  : bit_flags(0u), storage() {
		fmt::print("Constructor wooo\n");
		(emplace<Ts>(std::move(ts)),...);
	}

	~tuple_of_optionals() {
		clear();
	}

	tuple_of_optionals(tuple_of_optionals const &other) : bit_flags(other.bit_flags), storage(other.storage) { }
	tuple_of_optionals(tuple_of_optionals&& other) : bit_flags(std::move(other.bit_flags)), storage(std::move(other.storage)) {
		other.clear();
	}

	tuple_of_optionals& operator=(tuple_of_optionals const& other) {

		(emplace<Ts>(*other.get<Ts>()),...);

	}

	tuple_of_optionals& operator=(tuple_of_optionals && other) {

		(emplace<Ts>(std::move(*other.get<Ts>())), ...);

	}

	tuple_of_optionals(std::optional<Ts> const& ... opt_ts) {
		(maybe_emplace<Ts>(opt_ts), ...);
	}

	tuple_of_optionals(std::optional<Ts> &&... opt_ts) {
		(maybe_emplace<Ts>(std::move(opt_ts)), ...);
	}

	using TypeList_t = TypeList<Ts...>;

	template<typename T>
	using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;

	template<size_t N>
	using type_of_index = typename TypeList<Ts...>::template type_of_index<N>;

	void clear() {
		(maybe_destruct<Ts>(), ...);
	}

	template<typename T>
	T * get() noexcept {
		static_assert(TypeList<Ts...>::template contains<T>(), "tuple_of_optionals<Ts...>::get_component<T>() called where T is not in Ts ...");
		
		return get<index_of<T>()>();
	}

	template<typename T>
	T const * get() const noexcept {
		static_assert(TypeList<Ts...>::template contains<T>(), "tuple_of_optionals<Ts...>::get_component<T>() called where T is not in Ts ...");

		return get<index_of<T>()>();
	}

	template<size_t N>
	type_of_index<N> * get() noexcept {
		static_assert( N <= sizeof...(Ts) and 0 <= N, "tuple_of_optional::get<size_t N>() called for out-of-bounds argument N");
		if (bit_flags[N] == false) {
			return nullptr;
		}

		return get_address_of_slot<N>();
	};

	template<size_t N>
	type_of_index<N> const * get() const noexcept {
		if (bit_flags[N] == false)
			return nullptr;

		return get_address_of_slot<N>();
	};

	template<typename T>
	void maybe_emplace(std::optional<T> const & opt_t) {

		if (opt_t.has_value())
			emplace<T>(opt_t);
	}
	
	template<typename T>
	void maybe_emplace(std::optional<T> && opt_t) {
		if (opt_t.has_value())
			emplace<T>(std::move(opt_t));
	}

	template<typename T>
	void emplace(T && t) {
		maybe_destruct<T>();
		new(get_address_of_slot_for<T>()) T(std::move(t));
		bit_flags[index_of<T>()]= true;
	}

	template<typename T>
	void emplace(T const& t) {
		maybe_destruct<T>();

		new(get_address_of_slot_for<T>()) T(t);
		bit_flags[index_of<T>()] = true;
	}

	template<typename T, typename...ConstructorArgs>
	void make_in_place(ConstructorArgs &&...c_args) {
		maybe_destruct<T>();
		new(get_address_of_slot_for<T>()) T(std::forward<ConstructorArgs>(c_args) ...);
		
		bit_flags[index_of<T>()] = true;
	}

protected:
	template<size_t N>
	void maybe_destruct() {
		if (bit_flags[N]) 
			get<N>()->~type_of_index<N>();
		bit_flags[N] = false;
	}

	template<typename T>
	void maybe_destruct() noexcept(noexcept(std::declval<T>().~T())) {
		if (bit_flags[index_of<T>()])
			get<T>()->~T();
		bit_flags[index_of<T>()] = false;

	}

	template<size_t N>
	type_of_index<N> * get_address_of_slot() noexcept {

		return reinterpret_cast<type_of_index<N> *>(&std::get<N>(storage));
	}

	template<typename T>
	T * get_address_of_slot_for() noexcept {
		return get_address_of_slot<index_of<T>()>();
	}

	template<size_t N>
	type_of_index<N> const * get_address_of_slot() const noexcept {
		return reinterpret_cast<type_of_index<N> const *>(&std::get<N>(storage));
	}

	template<typename T>
	T const * get_address_of_slot_for() const noexcept {
		return get_address_of_slot<TypeList_t::template get_index_of<T>()>();
	}

	template<typename T>
	static constexpr size_t index_of() {
		return TypeList<Ts...>::template get_index_of<T>();
	}

private:
	std::bitset<sizeof...(Ts)> bit_flags;
	std::tuple< storage_t<Ts>...> storage;

};
