#pragma once

#include <cassert>
#include <exception>
#include <stdexcept>
#include <iostream>

namespace Quokka {

template<typename T>
class Try {

	enum class State {
		None,
		Exception,
		Value
	};

public:

	Try() :
		state_(State::None) {
	}

	Try(const T& t) :
		state_(State::Value),
		value_(t) {
	}

	Try(T&& t) :
		state_(State::Value),
		value_(std::move(t)) {
	}

	Try(std::exception_ptr e) :
		state_(State::Exception),
		exception_(std::move(e)) {
	}

	// Move constructor
	Try(Try<T>&& t) :
		state_(t.state_) {
		if (state_ == State::Value) {
			new (&value_) T(std::move(t.value_));
		}
		else if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(std::move(t.exception_));
		}
	}

	// Move assignment operator
	Try<T>& operator=(Try<T>&& t) {
		if (this == &t) {
			return *this;
		}

		this->~Try();

		state_ = t.state_;
		if (state_ == State::Value) {
			new (&value_) T(std::move(t.value_));
		}
		else if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(std::move(t.exception_));
		}

		return *this;
	}

	// Copy constructor
	Try(const Try<T>& t) :
		state_(t.state_) {
		if (state_ == State::Value) {
			new (&value_) T(t.value_);
		}
		else if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(t.exception_);
		}
	}

	// Copy assignment operator
	Try<T>& operator=(const Try<T>& t) {
		if (this == &t) {
			return *this;
		}

		this->~Try();

		state_ = t.state_;
		if (state_ == State::Exception) {
			new (&value_) T(t.value_);
		}
		else if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(t.exception_);
		}

		return *this;
	}

	~Try() {
		if (state_ == State::Exception) {
			exception_.~exception_ptr();
		}
		else if (state_ == State::Value) {
			value_.~T();
		}
	}

	// Implicit conversion
	operator const T& () const & {
		return value();
	}

	operator T& () & {
		return value();
	}

	operator T && () && {
		return std::move(value());
	}

	// Get value
	const T& value() const & {
		check();
		return value_;
	}

	T& value() & {
		check();
		return value_;
	}

	T&& value() && {
		check();
		return std::move(value_);
	}

	// Get exception
	const std::exception_ptr& exception() const & {
		if (!hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return exception_;
	}

	std::exception_ptr& exception() & {
		if (!hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return exception_;
	}

	std::exception_ptr&& exception() && {
		if (!hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return exception_;
	}

	bool hasValue() const {
		return state_ == State::Value;
	}

	bool hasException() const {
		return state_ == State::Exception;
	}

	const T& operator*() const {
		return value();
	}

	T& operator*() {
		return value();
	}

	struct UninitializedTry {};

	void check() const {
		if (state_ == State::Exception) {
			std::rethrow_exception(exception_);
		}
		else if (state_ == State::None) {
			throw UninitializedTry();
		}
	}

	template<typename R>
	R get() {
		return std::forward<R>(value());
	}

private:

	State state_;

	// A union is a special class type that can hold only one of its non-static
	// data members at a time
	union {
		T value_;
		// std::exception_ptr is a nullable pointer-like type that manages an
		// exception object which has been thrown and captured with std::current_exception.
		std::exception_ptr exception_;
	};
};

template<>
class Try<void> {

	enum class State {
		Exception,
		Value
	};

public:

	Try() :
		state_(State::Value) {
	}

	explicit Try(std::exception_ptr e) :
		state_(State::Exception),
		exception_(std::move(e)) {
	}

	// Move constructor
	Try(Try<void>&& t) noexcept :
		state_(t.state_) {
		if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(std::move(t.exception_));
		}
	}

	// Move assignment operator
	Try<void>& operator=(Try<void>& t) {
		if (this == &t) {
			return *this;
		}

		this->~Try();

		state_ = t.state_;
		if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(std::move(t.exception_));
		}

		return *this;
	}

	// Copy constructor
	Try(const Try<void>& t) :
		state_(t.state_) {
		if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(t.exception_);
		}
	}

	// Copy assignment operator
	Try<void>& operator=(const Try<void>& t) {
		if (this == &t) {
			return *this;
		}

		this->~Try();

		state_ = t.state_;
		if (state_ == State::Exception) {
			new (&exception_) std::exception_ptr(t.exception_);
		}

		return *this;
	}

	~Try() {
		if (state_ == State::Exception) {
			exception_.~exception_ptr();
		}
	}

	const std::exception_ptr& exception() const & {
		if (!hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return exception_;
	}

	std::exception_ptr& exception() & {
		if (!hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return exception_;
	}

	std::exception_ptr&& exception() && {
		if (hasException()) {
			throw std::runtime_error("Not exception state");
		}

		return std::move(exception_);
	}

	bool hasValue() const {
		return state_ == State::Value;
	}

	bool hasException() const {
		return state_ == State::Exception;
	}

	void check() const {
		if (state_ == State::Exception) {
			std::rethrow_exception(exception_);
		}
	}

	template<typename R>
	R get() {
		return std::forward<R>(*this);
	}

	void print() {
		std::cout << "Hello World" << std::endl;
	}

private:

	State state_;
	std::exception_ptr exception_;

};

// TryWrapper<T>: If T is Try type, then Type is T otherwise Try<T>
/*
相对于下面模板比较general，当T不是一个被Try包裹的类型的时候会匹配这个模板
在里面定义了Type = Try<T>，所以Type是一个被Try包裹的类型
总的来说，被TryWrapper包裹的类型里面都会有一个叫做Type的type alias，而这个
Type代表被Try struct包裹的类型T
*/
template<typename T>
struct TryWrapper {
	using Type = Try<T>;
};

/*
相对于上面的模板更加特化，所以如果T是一个被Try包裹的type的话，那么会优先匹配这个类型
被匹配之后，T是被Try包裹的真正的类型
在里面又有一个type alias定义了Type，是一个Try struct包裹真正的类型
*/
template<typename T>
struct TryWrapper<Try<T>> {
	using Type = Try<T>;
};

/*
Wrap function f(...) return by Try<T>

先从里面的typename定义看起
typename std::result_of<F(Args...)>::type是F(Args...)的类型
之后又在外面加了一层TryWrapper<>::Type，TryWrapper提供了一个统一的interface使得::Type
是被Try包裹一层的真正type

template<class T, class U>
If T and U name the same type (including const/volatile qualifications), 
provides the member constant value equal to true. Otherwise value is false

接下来再看std::enable_if的条件，是判断两个type是否相等
第一个为typename std::result_of<F(Args...)>::type, 是F(Args...)的结果类型
第二个为void
也就是说这个std::is_same是为了判断F(Args...)的结果类型是不是void

最后再来看外层的std::enable_if，如果第一个参数为true的话，它有一个public typedef type
也就是说这个function返回值的意思是如果F(Args...)的结果不是void的话，就用一个Try把它
包裹起来，如果是void的话，这个函数模板无法匹配
*/
template<typename F, typename... Args>
typename std::enable_if<
	!std::is_same<typename std::result_of<F(Args...)>::type, void>::value,
	typename TryWrapper<typename std::result_of<F(Args...)>::type>::Type>::type
WrapWithTry(F&& f, Args&& ... args) {

	// 首先使用一个type alias定义F(Args...)返回值的类型
	using Type = typename std::result_of<F(Args...)>::type;

	try {
		return typename TryWrapper<Type>::Type(std::forward<F>(f)(std::forward<Args>(args)...));
	}
	catch (std::exception& e) {
		return typename TryWrapper<Type>::Type(std::current_exception());
	}
}

/*
这个函数模板与上面的比较类似
只不过这次匹配的是F(Args...)返回值type为void的情况，使用Try struct将它包裹起来
*/
template<typename F, typename... Args>
typename std::enable_if <
	std::is_same<typename std::result_of<F(Args...)>::type, void>::value,
	Try<void>>::type
WrapWithTry(F&& f, Args&& ... args) {
	try {
		std::forward<F>(f)(std::forward<Args>(args)...);
		return Try<void>();
	}
	catch (std::exception& e) {
		return Try<void>(std::current_exception());
	}
}

/*
与上面的分析相似，这也是一个模板函数，但这次只接受一个参数F，也就是说这个
function不需要参数

如果F()调用结果的类型是void，那么会trigger SFINAE机制，匹配其他的模板
如果F()调用结果的类型不是void，那么这个函数返回的是被Try包裹的F()结果类型
*/
template<typename F>
typename std::enable_if<
	!std::is_same<typename std::result_of<F()>::type, void>::value,
	typename TryWrapper<typename std::result_of<F()>::type>::Type>::type
WrapWithTry(F&& f, Try<void>&& arg) {
	using Type = typename std::result_of<F()>::type;

	try {
		return typename TryWrapper<Type>::Type(std::forward<F>(f)());
	}
	catch (std::exception& e) {
		return typename TryWrapper<Type>::Type(std::current_exception());
	}
}

/*
匹配F()返回值为void的情况
这种情况下返回被Try包裹的Try<void>
*/
template<typename F>
typename std::enable_if<
	std::is_same<typename std::result_of<F()>::type, void>::value,
	Try<typename std::result_of<F()>::type>>::type
WrapWithTry(F&& f, Try<void>&& arg) {
	try {
		std::forward<F>(f)();
		return Try<void>();
	}
	catch (std::exception& e) {
		return Try<void>(std::current_exception());
	}
}


}  // namespace Quokka