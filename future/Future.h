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

	using ValueType = typename TryWrapper<T>::Type;
	ValueType value_;
	std::function<void(ValueType&&)> then_;

	Progress progress_;

	std::function<void(TimeoutCallback&&)> onTimeout_;
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
	/*
	* template< bool B, class T = void >
	* If B is true, std::enable_if has a public member typedef type, equal to T; otherwise
	* there is no member typedef
	*/
	typename std::enable_if<!std::is_void<SHIT>::value, void>::type

private:

	std::shared_ptr<State<T>> state_;
};

}  // namespace Quokka