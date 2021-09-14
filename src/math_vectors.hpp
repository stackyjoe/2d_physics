#include <array>
#include <concepts>
#include <memory>
#include <variant>
#include <type_traits>

namespace mathematics {

	namespace concepts {
		template<typename Scalar>
		concept FieldLike = requires(Scalar x, Scalar y) {
			{ x + y } -> std::same_as<Scalar>;
			{ x - y } -> std::same_as<Scalar>;
			{ x* y } -> std::same_as<Scalar>;
			{ x / y } -> std::same_as<Scalar>;
			{ x += y } -> std::same_as<Scalar&>;
			{ x -= y } -> std::same_as<Scalar&>;
			{ x *= y } -> std::same_as<Scalar&>;
			{ x /= y } -> std::same_as<Scalar&>;

			{ 0 } -> std::convertible_to<Scalar>;
			{ 1 } -> std::convertible_to<Scalar>;
		};

		template<typename Vector>
		concept VectorSpaceLike = requires(typename Vector::Scalar_t alpha, Vector x, Vector y) {
			{ x + y } -> std::same_as<Vector>;
			{ x - y } -> std::same_as<Vector>;
			{ x += y } -> std::same_as<Vector&>;
			{ x -= y } -> std::same_as<Vector&>;
			{ x *= alpha } -> std::same_as<Vector&>;
			{ 0 } -> std::convertible_to<Vector>;

			{ alpha* x } -> std::same_as<Vector>;
		};
	}

	namespace detail {

		template<concepts::FieldLike Value, size_t Dimension, bool OnHeap>
		struct array;

		template<concepts::FieldLike Value, size_t Dimension>
		struct array<Value, Dimension, false> {
			// Default constructor gives zero'd out array.
			array() {
				data.fill(0);
			}
			array(array const&) = default;
			array(array&&) = default;
			array& operator=(array const&) = default;
			array& operator=(array&&) = default;

			Value& at(size_t n) {
				return data.at(n);
			}

			Value const& at(size_t n) const {
				return data.at(n);
			}

			Value& operator[](size_t n) noexcept {
				return data[n];
			}

			Value const& operator[](size_t n) const noexcept {
				return data[n];
			}

			std::array<Value, Dimension> data;
		};

		template<concepts::FieldLike Value, size_t Dimension>
		struct array<Value, Dimension, true> {
			using  Scalar_t = Value;

			// Default constructor gives zero'd out array.
			array()
				: owning_ptr(std::make_unique<std::array<Value,Dimension>>()){
				owning_ptr->fill(0);
			}
			array(array&&) = default;
			array& operator=(array&&) = default;
			
			// Deep-copy copy initializer and assignment
			array(array const& other)
				: owning_ptr(std::make_unique<std::array<Value, Dimension>>(*(other.owning_ptr))) {
			}
			array& operator=(array const& other) {
				(*owning_ptr) = *(other.owning_ptr);

			}


			Value& at(size_t n) {
				return owning_ptr->at(n);
			}

			Value const& at(size_t n) const {
				return owning_ptr->at(n);
			}

			Value& operator[](size_t n) noexcept {
				return (*owning_ptr)[n];
			}

			Value const& operator[](size_t n) const noexcept {
				return (*owning_ptr)[n];
			}


			std::unique_ptr<std::array<Value, Dimension> > owning_ptr;
		};
	}


	template<concepts::FieldLike Scalar, size_t Dimension>
	class vector {
		static constexpr bool store_on_heap
			= sizeof(Scalar) * Dimension >= 20 * sizeof(char)
			or !std::movable<Scalar> or !std::copyable<Scalar>;

		detail::array<Scalar, Dimension, store_on_heap> storage;

	public:
		std::array<Scalar, Dimension>& underlying_array() noexcept {
			if constexpr (store_on_heap)
				return *(storage.owning_ptr);
			else
				return storage.data;
		}


		vector() = default;
		vector(vector const&) = default;
		vector(vector&&) = default;
		vector& operator=(vector const&) = default;
		vector& operator=(vector&&) = default;

		auto begin() noexcept {
			return underlying_array().begin();
		}

		auto end() noexcept {
			return underlying_array().end();
		}

		auto begin() const noexcept {
			return underlying_array().cbegin();
		}

		auto end() const noexcept {
			return underlying_array().cend();
		}


		Scalar& at(size_t n) {
			return storage.at(n);
		}

		Scalar const& at(size_t n) const {
			return storage.at(n);
		}

		Scalar& operator[](size_t n) noexcept {
			return storage[n];
		}

		Scalar const& operator[](size_t n) const noexcept {
			return storage[n];
		}

		static vector zero() noexcept {
			return vector();
		}

		vector operator+(vector const& summand) const noexcept(noexcept(std::declval<Scalar>() + std::declval<Scalar>())) {
			vector sum;

			for (size_t i = 0; i < Dimension; ++i) {
				sum[i] = (*this)[i] + summand[i];
			}

			return sum;
		}

		vector operator-(vector const& subtrahend) const noexcept(noexcept(std::declval<Scalar>() - std::declval<Scalar>())) {
			vector difference;

			for (size_t i = 0; i < Dimension; ++i) {
				difference[i] = (*this)[i] - subtrahend[i];
			}

			return difference;
		}

		vector& operator*=(Scalar const& multiplicand) noexcept(noexcept(std::declval<Scalar>()* std::declval<Scalar>())) {
			for (size_t i = 0; i < Dimension; ++i) {
				(*this)[i] *= multiplicand;
			}

			return *this;
		}

		vector & operator+=(vector const& summand) noexcept(noexcept(std::declval<Scalar&>() += std::declval<Scalar>())) {
			for (size_t i = 0; i < Dimension; ++i)
				(*this)[i] += summand[i];
			return *this;
		}

		vector & operator-=(vector const& subtrahend) noexcept(noexcept(std::declval<Scalar&>() -= std::declval<Scalar>())) {
			for (size_t i = 0; i < Dimension; ++i)
				(*this)[i] -= subtrahend[i];
			return *this;
		}

	};

	template<concepts::FieldLike Scalar, size_t Dimension>
	vector<Scalar, Dimension> operator*(Scalar const& multiplier, vector<Scalar, Dimension> const& multiplicand) {
		vector<Scalar, Dimension> product = multiplicand;
		product *= multiplier;

		return product;
	}


}
