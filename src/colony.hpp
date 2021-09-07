#pragma once

#include <bitset>

#include "TypeList.hpp"

using namespace ListsViaTypes;

namespace detail {
	template<size_t N, typename T>
	struct array_of_optionals {
		array_of_optionals(T&& ... ts) { (emplace<Ts>(std::move(ts)), ...); }

		~tuple_of_optionals() { clear(); }

		array_of_optionals(array_of_optionals const& other) : bit_flags(other.bit_flags), storage(other.storage) { }
		array_of_optionals(array_of_optionals&& other) : bit_flags(std::move(other.bit_flags)), storage(std::move(other.storage)) { }

		constexpr size_t length() const noexcept {
			return N;
		}

	protected:

		template<size_t M>
		T* get_address_of_slot() noexcept {
			return reinterpret_cast<T*>(&std::get<M>(storage));
		}

		template<size_t M>
		T* get() noexcept {
			if (bit_flags[M] == false)
				return nullptr;

			return get_address_of_slot<M>();
		};

		template<size_t M>
		void maybe_destruct() {
			if(bit_flags[M] == true)
				get<N>()->~T();
		}

		template<size_t M>
		void maybe_emplace(std::optional<T>&& opt_t) {
			if (opt_t.has_value())
				emplace<M>(std::move(opt_t));
		}

		template<size_t M>
		void emplace(T&& t) {
			maybe_destruct<M>();
			new(get_address_of_slot<N>()) T(std::move(t));
			bit_flags[M] = true;
		}

		template<size_t M>
		T const* get() const noexcept {
			if (bit_flags[M] == false)
				return nullptr;

			return get_address_of_slot<M>();
		};

		T* operator[](std::ptrdiff_t M) noexcept {
			if (N >= M or or N < 0 or bit_flags[M] == false)
				return nullptr;

			return get_address_of_slot<0>() + M;
		}

	private:

		std::bitset<N> bit_field;
		std::aligned_storage_t<N * sizeof(T), alignas(T)> storage;
	};
	
}



template<size_t, typename T>
struct colony {

};