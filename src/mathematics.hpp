#include <array>
#include <concepts>
#include <initializer_list>
#include <memory>
#include <variant>
#include <type_traits>

namespace mathematics {

	namespace concepts {
		template<typename Scalar>
		concept RingLike = requires(Scalar x, Scalar y) {
			{ x + y } -> std::same_as<Scalar>;
			{ x - y } -> std::same_as<Scalar>;
			{ x* y } -> std::same_as<Scalar>;
			{ x += y } -> std::same_as<Scalar&>;
			{ x -= y } -> std::same_as<Scalar&>;
			{ x *= y } -> std::same_as<Scalar&>;
			{ x /= y } -> std::same_as<Scalar&>;

			{ 0 } -> std::convertible_to<Scalar>;
			{ 1 } -> std::convertible_to<Scalar>;
		};

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

		template<concepts::FieldLike Scalar, size_t Dimension, bool OnHeap>
		struct array;

		template<concepts::FieldLike Scalar, size_t Dimension>
		struct array<Scalar, Dimension, false> {
			// Default constructor gives zero'd out array.
			array() {
				data.fill(0);
			}
			array(array const&) = default;
			array(array&&) = default;
			array& operator=(array const&) = default;
			array& operator=(array&&) = default;

			array(std::initializer_list<Scalar> list) {
				for (auto i = 0; i < std::min(Dimension, list.size()); ++i)
					data[i] = std::data(list)[i];
			}

			Scalar& at(size_t n) {
				return data.at(n);
			}

			Scalar const& at(size_t n) const {
				return data.at(n);
			}

			Scalar& operator[](size_t n) noexcept {
				return data[n];
			}

			Scalar const& operator[](size_t n) const noexcept {
				return data[n];
			}

			std::array<Scalar, Dimension> data;
		};

		template<concepts::FieldLike Scalar, size_t Dimension>
		struct array<Scalar, Dimension, true> {
			using  Scalar_t = Scalar;

			// Default constructor gives zero'd out array.
			array()
				: owning_ptr(std::make_unique<std::array<Scalar,Dimension>>()){
				owning_ptr->fill(0);
			}
			array(array&&) = default;
			array& operator=(array&&) = default;
			
			// Deep-copy copy initializer and assignment
			array(array const& other)
				: owning_ptr(std::make_unique<std::array<Scalar, Dimension>>(*(other.owning_ptr))) {
			}

			array(std::initializer_list<Scalar> list)
				: owning_ptr(std::make_unique<std::array<Scalar, Dimension>>()) {
				for (auto i = 0; i < std::min(Dimension, list.size()); ++i)
					(*this)[i] = std::data(list)[i];
			}

			array& operator=(array const& other) {
				(*owning_ptr) = *(other.owning_ptr);

			}


			Scalar& at(size_t n) {
				return owning_ptr->at(n);
			}

			Scalar const& at(size_t n) const {
				return owning_ptr->at(n);
			}

			Scalar& operator[](size_t n) noexcept {
				return (*owning_ptr)[n];
			}

			Scalar const& operator[](size_t n) const noexcept {
				return (*owning_ptr)[n];
			}


			std::unique_ptr<std::array<Scalar, Dimension> > owning_ptr;
		};
	}


	template<concepts::FieldLike Scalar, size_t Dimension>
	class vector {
		static constexpr bool store_on_heap
			= sizeof(Scalar) * Dimension >= 20 * sizeof(char)
			or !std::movable<Scalar> or !std::copyable<Scalar>;

		detail::array<Scalar, Dimension, store_on_heap> storage;

	public:
		constexpr std::array<Scalar, Dimension>& underlying_array() noexcept {
			if constexpr (store_on_heap)
				return *(storage.owning_ptr);
			else
				return storage.data;
		}

		const constexpr std::array<Scalar, Dimension>& underlying_array() const noexcept {
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

		vector(std::initializer_list<Scalar> list)
			: storage(list) {
		}

		auto begin() noexcept {
			return underlying_array().begin();
		}

		auto end() noexcept {
			return underlying_array().end();
		}

		auto begin() const noexcept {
			return underlying_array().begin();
		}

		auto end() const noexcept {
			return underlying_array().end();
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

	template<typename Function, size_t Dimension, typename ...Args>
	auto elementwise_apply(Function f, vector<Args, Dimension> ... args) -> vector<decltype(f(args[0] ...)), Dimension> {
		vector<decltype(f(args[0] ...)), Dimension> result;

		for (size_t i = 0; i < Dimension; ++i)
			result[i] = f(args[i] ...);

		return result;
	}

	template<concepts::FieldLike Scalar, size_t Dimension>
	Scalar hypotenuse(vector<Scalar, Dimension> const &argument) noexcept {
		Scalar hypot = 0;

		for (auto i = 0; i < Dimension; ++i)
			hypot += argument[i] * argument[i];

		return std::sqrt(hypot);
	}

	template<concepts::RingLike Scalar, size_t Rows, size_t Columns>
	class matrix {
		static constexpr size_t linearized_size = Rows * Columns;

		static constexpr bool store_on_heap
			= sizeof(Scalar) * linearized_size >= 20 * sizeof(char)
			or !std::movable<Scalar> or !std::copyable<Scalar>;


		detail::array<Scalar, Rows* Columns, store_on_heap> storage;
	public:
		constexpr std::array<Scalar, linearized_size>& underlying_array() noexcept {
			if constexpr (store_on_heap)
				return *(storage.owning_ptr);
			else
				return storage.data;
		}

		const constexpr std::array<Scalar, linearized_size>& underlying_array() const noexcept {
			if constexpr (store_on_heap)
				return *(storage.owning_ptr);
			else
				return storage.data;
		}

		matrix() = default;
		matrix(matrix const&) = default;
		matrix(matrix&&) = default;
		matrix& operator=(matrix const&) = default;
		matrix& operator=(matrix&&) = default;

		Scalar& operator()(size_t i, size_t j) noexcept {
			return underlying_array()[i*Columns + j];
		}

		Scalar const & operator()(size_t i, size_t j) const noexcept {
			return underlying_array()[i * Columns + j];
		}

		Scalar& at(size_t i, size_t j) {
			return underlying_array().at(i * Columns + j);
		}

		Scalar const & at(size_t i, size_t j) const {
			return underlying_array().at(i * Columns + j);
		}

		matrix transpose() const {
			matrix<Scalar, Rows, Columns> transpose;

			for (auto i = 0; i < Rows; ++i)
				for (auto j = 0; j < Columns; ++j)
					transpose(j, i) = (*this)(i, j);

			return transpose;
		}
	};

	template<concepts::RingLike Scalar, size_t i, size_t j,size_t k>
	matrix<Scalar, i, k> operator*(matrix<Scalar,i,j> const &multiplicand, matrix<Scalar, j, k> const &multiplier) {
		matrix<Scalar, i, k> product;
		for (auto x = 0; x < i; ++x)
			for (auto y = 0; y < k; ++y)
				for (auto z = 0; z < j; ++z)
					product(x, y) += multiplicand(x, z) * multiplier(z, y);
		return product;
	}

	template<concepts::RingLike Scalar, size_t i, size_t j, size_t k>
	matrix<Scalar, i, k> multiply_by_transpose(matrix<Scalar, i, j> const& multiplicand, matrix<Scalar, k,j> const& multiplier) {
		matrix<Scalar, i, k> product;
		for (auto x = 0; x < i; ++x)
			for (auto y = 0; y < k; ++y)
				for (auto z = 0; z < j; ++z)
					product(x, y) += multiplicand(x, z) * multiplier(y,z);
		return product;
	}

	template<concepts::RingLike Scalar, size_t N>
	Scalar trace(matrix<Scalar, N, N> const& argument) noexcept {
		Scalar trace = 0;
		for (auto i = 0; i < N; ++i)
			trace += argument(i, i);
		return trace;
	}
}
