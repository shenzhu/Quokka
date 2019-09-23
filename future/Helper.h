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

/*
template<class T>
typename std::add_rvalue_reference<T>::type decval() noexcept;

Converts any type T to a reference type, making it possible to use member functions
in decltype expressions without the need to go through constructors.
declval is commonly used in templates where acceptable template parameters may have no
constructor in common

std::declval<F>()得到F的type
std::declval<Args>()...得到Args...的type
decltype<std::declval<F>()(std::declval<Args>()...))即为F(Args...)得到的type
*/
template<typename F, typename... Args>
using ResultOf = decltype(std::declval<F>()(std::declval<Args>()...));

/*
基于前面的ResultOf struct，在外面套了一层Type，也就是说
使用ResultOfWrapper<F, Args...>::Typeh会得到F(Args...)的type
*/
template<typename F, typename... Args>
struct ResultOfWrapper {
	using Type = ResultOf<F, Args...>;
};

// Test if F can be called with Args type
template<typename F, typename... Args>
struct CanCallWith {
	/*
	template<class T, T v>
	struct integral_constant;

	std::integral_constant wraps a static constant of specified type.

	true_type -> std::integral_constant<bool, true>
	false_type -> std::integral_constant<bool, false>

	integral_constant有一个static的member value: static constant of type T with value v
	*/
	template<typename T, typename Dummy = ResultOf<T, Args...>>
	static constexpr std::true_type Check(std::nullptr_t dummy) {
		return std::true_type{};
	};

	/*
	...这种参数通常用在template中的function overload，作为SFINAE的最后
	一层匹配，保证能匹配到不会报错
	*/
	template<typename Dummy>
	static constexpr std::false_type Check(...) {
		return std::false_type{};
	};

	/*
	有两个Check function，这里传进去一个nullptr为了先去匹配第一个check
	而第一个check有一个默认的模板参数，因为这里只用了一个模板参数F去特化，
	所以编译器会自动调用ResultOf<T, Args...>去测试是否可能，若不可行则会
	trigger SFINAE机制，匹配第二个Check
	type是Check<F>(nullptr)的返回值，std::true_type或者std::false_type
	它们的::value member才是true或者false，respectively

	总的来说CanCallWith<F, Args...>::value返回的是一个true或者false的值
	*/
	using type = decltype(Check<F>(nullptr));
	static constexpr bool value = type::value;
};

/*
这两个IsFuture要连在一起看
IsFuture<Future<T>>更加特化一点，所以当使用IsFuture<Futre<T>>进行特化的时候
会优先使用第二个模板进行特化

IsFuture继承自std::false_type，它会有一个static member叫做value，值为false
另外因为在里面使用了type alias，它还有一个::Inner表明特化的类型
*/
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
	/*
	template<bool B, class T, class F>
	struct conditional;

	Provides member typedef type, which is defined as T if B is true at compile time,
	or as F if B is false

	这个定义比较复杂，先从内部看起
	typename std::conditional<
		CanCallWith<F, T&&>::value,
		ResultOfWrapper<F, T&&>,
		ResultOfWrapper<F, T&>>::type
	如果CanCallWith<F, T&&>::value为true的话，就返回ResultOfWrapper<F, T&&>,
	如果为false，那么就返回ResultOfWrapper<F, T&>
	根据上面对ResultOfWrapper的分析可以得知ResulfOfWrapper<F, T&>::Type才是F(T&)
	的type，外面套了一层

	接着再看外面的那一层std::conditional
	CanCallWith<F>只用了一个模板参数去特化，那么第二个模板参数Args...即为void，因为它是可变参数模板
	也就是说，这一个大std::conditional的作用就是首先看一下F(void)可不可以，如果可以的话
	就返回F(void)结果的Wrapper，如果F(void)不可以的话，就返回F(T&&)或者F(T&)的结果Wrapper，
	按照这个顺序
	*/
	using Arg =
		typename std::conditional<
			CanCallWith<F>::value,  // If true, F can call with void
			ResultOfWrapper<F>,
			typename std::conditional<  // No, F(void) is invalid
				CanCallWith<F, T&&>::value,  // If ture, F(T&&) is valid
				ResultOfWrapper<F, T&&>,  // Yes, F(T&&) is fine
				ResultOfWrapper<F, T&>>::type>::type;  // If all failed, resort to F(T&)

	/*
	根据上面的分析可以得知Arg其实就是一个ResultOfWrapper，而它的Type则是实际的调用结果type
	外面又用了一层IsFuture将它包裹起来，也就是说有两种情况
	如果T是一个被Future包裹的type，那么IsReturnsFuture::value为true，IsReturnsFuture::Inner为函数调用的结果type
	如果T不是一个被Future包裹的type，IsReturnsFuture::为false，IsReturnsFuture::Inner为函数调用的结果type
	*/
	using IsReturnsFuture = IsFuture<typename Arg::Type>;
	/*
	typename IsReturnsFuture::Inner表示函数调用结果的type
	Future<typename IsReturnsFuture::Inner>就是在外面加了一个Future将它包裹一下
	*/
	using ReturnFutureType = Future<typename IsReturnsFuture::Inner>;
};

// Callable specilization for void
template<typename F>
struct CallableResult<F, void> {
	/*
	同样的分析方法，先看内层的std::conditional
	如果F(Try<void>&&)调用合法的话，返回它的ResultOfWrapper，
	如果不合法，返回F(const Try<void>&)的ResultOfWrapper

	再来看外层的std::conditional，如果F(void)合法的话，直接返回它的ResultOfWrapper
	如果不合法，则按照内层的逻辑进行分析
	*/
	using Arg = 
		typename std::conditional<
			CanCallWith<F>::value,  // If true, F can be called with void
			ResultOfWrapper<F>,
			typename std::conditional<  // No, F(void) is invalid
				CanCallWith<F, Try<void>&&>::value,  // If true, F(Try<void>&&) is valid
				ResultOfWrapper<F, Try<void>&&>,  // Yes, F(Try<void>&&) is fine
				ResultOfWrapper<F, const Try<void>&>>::type>::type;  // If all failed, resort to F(const Try<void>&)

	/*
	根据上面的分析可以得知Arg其实就是一个ResultOfWrapper，而它的Type则是实际的调用结果type
	外面又用了一层IsFuture将它包裹起来，也就是说有两种情况
	如果T是一个被Future包裹的type，那么IsReturnsFuture::value为true，IsReturnsFuture::Inner为函数调用的结果type
	如果T不是一个被Future包裹的type，IsReturnsFuture::为false，IsReturnsFuture::Inner为函数调用的结果type
	*/
	using IsReturnsFuture = IsFuture<typename Arg::Type>;
	/*
	typename IsReturnsFuture::Inner表示函数调用结果的type
	Future<typename IsReturnsFuture::Inner>就是在外面加了一个Future将它包裹一下
	*/
	using ReturnFutureType = Future<typename IsReturnsFuture::Inner>;
};

}  // namespace Quokka