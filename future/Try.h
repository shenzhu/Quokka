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
	union {
		T value_;
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
template<typename T>
struct TryWrapper {
	using Type = Try<T>;
};

template<typename T>
struct TryWrapper<Try<T>> {
	using Type = Try<T>;
};

template<typename F, typename... Args>
typename std::enable_if<
	!std::is_same<typename std::result_of<F(Args...)>::type, void>::value,
	typename TryWrapper<typename std::result_of<F(Args...)>::type>::Type>::type
WrapWithTry(F&& f, Args&& ... args) {
	using Type = typename std::result_of<F(Args...)>::type;

	try {
		return typename TryWrapper<Type>::Type(std::forward<F>(f)(std::forward<Args>(args)...));
	}
	catch (std::exception& e) {
		return typename TryWrapper<Type>::Type(std::current_exception());
	}
}

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