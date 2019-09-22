#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <type_traits>

#include "Helper.h"
#include "Scheduler.h"
#include "Try.h"

namespace Quokka {

enum class Progress {
	None,
	Timeout,
	Done,
	Retrieved
};

using TimeoutCallback = std::function<void()>;

template<typename T>
struct State {
	static_assert(
		std::is_same<T, void>::value ||
		std::is_copy_constructible<T>() ||
		std::is_move_constructible<T>(),
		"Must be copyable or movable or void"
	);

	State() :
		progress_(Progress::None),
		retrieved_(false) {
	}

	std::mutex thenLock_;

	/*
	using ValueType = typename TryWrapper<T>::Type;
	ValueType value_;
	std::function<void(ValueType&&)> then_;
	*/
	typename TryWrapper<T>::Type value_;
	// Wraps a function that takes (typename TryWrapper<T>::Type&&) and return void
	std::function<void (typename TryWrapper<T>::Type&&)> then_;

	Progress progress_;

	// std::function<void(TimeoutCallback&&)> onTimeout_;
	std::function<void(std::function<void()>&&)> onTimeout_;
	std::atomic<bool> retrieved_;
};

template<typename T>
class Promise {
public:

	Promise() :
		state_(std::make_shared<State<T>>()) {
	}

	// The lambda with movable capture can not be stored in
	// std::function, just for compile, do NOT copy promise!
	Promise(const Promise& rhs) = default;
	Promise& operator=(const Promise& rhs) = default;

	Promise(Promise&& promise) = default;
	Promise& operator=(Promise&& promise) = default;

	void setException(std::exception_ptr exp) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = typename State<T>::ValueType(std::move(exp));
		guard.unlock();

		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	// template< bool B, class T = void >
	// If B is true, std::enable_if has a public member typedef type, equal to T; otherwise
	// there is no member typedef
	// 如果SHIT的type是void，那么std::is_void<SHIT>::value的返回值是true，因此enable_if的第一个参数
	// 为false，enable_if就会不存在type
	typename std::enable_if<!std::is_void<SHIT>::value, void>::type
	setValue(SHIT&& t) {
		// If ThenImp is running, here will wait for the lock.
		// After set then_, ThenImp will release lock.
		// And this func got lock, definitely will can then_
		// std::unique_lock与std::lock_guard的区别在于对于std::unique_lock可以进行unlock
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = std::forward<SHIT>(t);

		guard.unlock();

		// When reach here, state_ is determined, so mutex is useless
		// If the ThenImp function run, it'll see the Done state and
		// call user func there, not assign to then_.
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<!std::is_void<SHIT>::value, void>::type
	setValue(const SHIT& t) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = t;

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<!std::is_void<SHIT>::value, void>::type
	setValue(Try<SHIT>&& t) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress != Progress::None) {
			return;
		}

		state_->progress = Progress::Done;
		state_->value_ = std::forward<Try<SHIT>>(t);

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<!std::is_void<SHIT>::value, void>::type
	setValue(const Try<SHIT>& t) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress != Progress::None) {
			return;
		}

		state_->progress = Progress::Done;
		state_->value_ = t;

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<std::is_void<SHIT>::value, void>::type
	setValue(Try<void>&& t) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = Try<void>();

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<std::is_void<SHIT>::value, void>::type
	setValue(const Try<void>& t) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = Try<void>();

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	template<typename SHIT = T>
	typename std::enable_if<std::is_void<SHIT>::value, void>::type
	setValue() {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
		state_->value_ = Try<void>();

		guard.unlock();
		if (state_->then_) {
			state_->then_(std::move(state_->value_));
		}
	}

	Future<T> getFuture() {
		bool expect = false;
		if (!state_->retrieved_.compare_exchange_strong(expect, true)) {
			throw std::runtime_error("Future already retrieved");
		}

		return Future<T>(state_);
	}

	bool isReady() const {
		return state_->progress_ != Progress::None;
	}

private:

	std::shared_ptr<State<T>> state_;
};

template<typename T>
class Future {
public:

	Future() = default;

	Future(const Future& rhs) = delete;
	Future& operator=(const Future& rhs) = delete;

	Future(Future&& future) = default;
	Future& operator=(Future&& future) = default;

	explicit Future(std::shared_ptr<State<T>> state) :
		state_(std::move(state)) {
	}

	bool valid() const {
		return state_ != nullptr;
	}

	// 1. F does not return future type
	template<typename F, typename R, typename... Args>
	typename std::enable_if<!R::IsReturnsFuture::value, typename R::ReturnFutureType>::type
	_thenImpl(Scheduler* sched, F&& f, ResultOfWrapper<F, Args...>) {
		static_assert(sizeof...(Args) <= 1, "Then must take zero/one argument");

		using FReturnType = typename R::IsReturnsFuture::Inner;

		Promise<FReturnType> pm;
		auto nextFuture = pm.getFuture();

		using FuncType = typename std::decay<F>::type;

		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ == Progress::Timeout) {
			throw std::runtime_error("Wrong state: Timeout");
		}
		else if (state_->progress_ == Progress::Done) {
			typename TryWrapper<T>::Type t;
			try {
				t = std::move(state_->value_);
			}
			catch (const std::exception& e) {
				t = (typename TryWrapper<T>::Type)(std::current_exception());
			}

			guard.unlock()
		}
	}

private:

	void _setCallback(std::function<void (typename TryWrapper<T>::Type&&)>&& func) {
		state_->then_ = std::move(func);
	}

	void _setOnTimeout(std::function<void(std::function<void()>&&)>&& func) {
		state_->onTimeout_ = std::move(func);
	}

	std::shared_ptr<State<T>> state_;
};

}  // namespace Quokka