#pragma once

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

namespace Quokka {

template<typename T>
class Future;

template<typename T>
class Promise;

template<typename T>
class Try;

template<typename T>
struct TryWrapper;

template<typename F, typename... Args>
using ResultOf = decltype(std::declval<F>()(std::declval<Args>()...));

template<typename F, typename... Args>
struct ResultOfWrapper {
	using Type = ResultOf<F, Args...>;
};

// Test if F can be called with Args type
template<typename F, typename... Args>
struct CanCallWith {
	template<typename T, typename Dummy = ResultOf<T, Args...>>
	static constexpr std::true_type Check(std::nullptr_t dummy) {
		return std::true_type{};
	};

	template<typename Dummy>
	static constexpr std::false_type Check(...) {
		return std::false_type{};
	};

	using type = decltype(Check<F>(nullptr));
	static constexpr bool value = type::value;
};

template<typename T>
struct IsFuture : std::false_type {
	using Inner = T;
};

template<typename T>
struct IsFuture<Future<T>> : std::true_type {
	using Inner = T;
};

template<typename F, typename T>
struct CallableResult {
	using Arg =
		typename std::conditional<
			CanCallWith<F>::value,  // If true, F can call with void
			ResultOfWrapper<F>,
			typename std::conditional<  // No, F(void) is invalid
				CanCallWith<F, T&&>::value,  // If ture, F(T&&) is valid
				ResultOfWrapper<F, T&&>,  // Yes, F(T&&) is fine
				ResultOfWrapper<F, T&>>::type>::type;  // If all failed, resort to F(T&)

	using IsReturnsFuture = IsFuture<typename Arg::Type>;
	using ReturnFutureType = Future<typename IsReturnsFuture::Innter>;
};

// Callable specilization for void
template<typename F>
struct CallableResult<F, void> {
	using Arg = 
		typename std::conditional<
			CanCallWith<F>::value,  // If true, F can be called with void
			ResultOfWrapper<F>,
			typename std::conditional<  // No, F(void) is invalid
				CanCallWith<F, Try<void>&&>::value,  // If true, F(Try<void>&&) is valid
				ResultOfWrapper<F, Try<void>&&>,  // Yes, F(Try<void>&&) is fine
				ResultOfWrapper<F, const Try<void>&>>::type>::type;  // If all failed, resort to F(const Try<void>&)

	using IsReturnsFuture = IsFuture<typename Arg::Type>;
	using ReturnFutureType = Future<typename IsReturnsFuture::Inner>;
};

}  // namespace Quokka