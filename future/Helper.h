#pragma once

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
struct ResutOfWrapper {
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
			CanCallWith<F>::value,
			ResutOfWrapper<F>,
			typename std::conditional<
				CanCallWith<F, T&&>::value,
				ResutOfWrapper<F, T&&>,
				ResutOfWrapper<F, T&>>::type>::type;

	using IsReturnsFuture = IsFuture<typename Arg::Type>;
	using ReturnFutureType = Future<typename IsReturnsFuture::Innter>;
};

}  // namespace Quokka