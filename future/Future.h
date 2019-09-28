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
	/*
	template<class T, class U>
	struct is_same;
	If T and U name the same type(including const/voltile qualifications, for example, 
	char and const char are not the same type), provide the member constant
	value equal to true. Otherwise value is false.

	这个判断表示T是void或者T可以被copy construct或者可以被move construct
	*/
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
	从Try.h文件中的分析可以发现TryWrapper<T>::Type指的是被Try包裹的T，即使T是Try<T>
	所以ValueType就是被Try包裹的value type
	*/
	using ValueType = typename TryWrapper<T>::Type;
	ValueType value_;

	/*
	ValueType是被Try包裹的type
	这里的std::function包裹了一个接受ValueType的右值引用并且返回void的函数
	*/
	std::function<void (ValueType&& )> then_;
	Progress progress_;

	/*
	TimeoutCallback是std::function<void()>，包裹一个不接受参数返回void的函数
	这里的std::function包裹了一个接受 不接受参数返回void的函数 为argument并且返回void的函数
	*/
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
		if (state_->progress_ != Progress::None) {
			return;
		}

		state_->progress_ = Progress::Done;
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

template<typename T2>
Future<T2> makeExceptionFuture(std::exception_ptr&&);

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

	typename State<T>::ValueType
	wait(const std::chrono::milliseconds& timeout = std::chrono::milliseconds(24 * 3600 * 1000)) {
		std::unique_lock<std::mutex> guard(state_->thenLock_);
		switch (state_->progress_) {
		case Progress::None:
			break;

		case Progress::Timeout:
			throw std::runtime_error("Future timeout");

		case Progress::Done:
			state_->progress_ = Progress::Retrieved;
			return std::move(state_->value_);

		default:
			throw std::runtime_error("Future already retrieved");
		}
		guard.unlock();

		auto cond(std::make_shared<std::condition_variable>());
		auto mutex(std::make_shared<std::mutex>());
		bool ready = false;
		typename State<T>::ValueType value;

		this->then([
			&value,
			&ready,
			wcond = std::weak_ptr<std::condition_variable>(cond),
			wmutex = std::weak_ptr<std::mutex>(mutex)
		](typename State<T>::ValueType&& v) {
			auto cond = wcond.lock();
			auto mutex = wmutex.lock();
			if (!cond || !mutex) return;

			std::unique_lock<std::mutex> guard(*mutex);
			value = std::move(v);
			ready = true;
			cond->notify_one();
		});

		std::unique_lock<std::mutex> waiter(*mutex);
		bool success = cond->wait_for(waiter, timeout, [&ready]() { return ready; });
		if (success) {
			return std::move(value);
		}
		else {
			throw std::runtime_error("Future wait_for timeout");
		}
	}

	// T is of type Future<InnerType>
	template<typename SHIT = T>
	typename std::enable_if<IsFuture<SHIT>::value, SHIT>::type
	unwrap() {
		using InnerType = typename IsFuture<SHIT>::Inner;

		static_assert(std::is_same<SHIT, Future<InnerType>>::value, "Kidding me?");

		Promise<InnerType> prom;
		Future<InnerType> fut = prom.getFuture();

		std::unique_lock<std::mutex> guard(state_->thenLock_);
		if (state_->progress_ == Progress::Timeout) {
			throw std::runtime_error("Wrong state: Timeout");
		}
		else if (state_->progress_ == Progress::Done) {
			try {
				auto innerFuture = std::move(state_->value_);
				return std::move(innerFuture.value());
			}
			catch (const std::exception& e) {
				return makeExceptionFuture<InnerType>(std::current_exception());
			}
		}
		else {
			_setCallback([pm = std::move(prom)](typename TryWrapper<SHIT>::Type&& innerFuture) mutable {
				try {
					SHIT future = std::move(innerFuture);
					future._setCallback([pm = std::move(pm)](typename TryWrapper<InnerType>::Type&& t) mutable {
						// No need scheduler here, think about this code:
						// `outer.Unwrap().Then(sched, func);`
						// outer.Unwrap() is the inner future, the below line
						// will trigger func in sched thread.
						pm.setValue(std::move(t));
					});
				}
				catch (...) {
					pm.setException(std::current_exception());
				}
			});
		}

		return fut;
	}

	template<typename F, typename R = CallableResult<F, T>>
	auto then(F&& f) -> typename R::ReturnFutureType {
		typedef typename R::Arg Arguments;
		return _thenImpl<F, R>(nullptr, std::forward<F>(f), Arguments());
	}

	template<typename F, typename R = CallableResult<F, T>>
	auto then(Scheduler* sched, F&& f) -> typename R::ReturnFutureType {
		typedef typename R::Arg Arguments;
		return _thenImpl<F, R>(sched, std::forward<F>(f), Arguments());
	}

	// 1. F does not return future type
	/*
	template<bool B, class T=void>
	struct enable_if;

	If B is true, std::enable_if has a public member typedef type, equal to T;
	otherwise there is not member typedef
	如果B为true的话，std::enable_if就有一个public的typedef叫做type，它等于T(默认为void)

	R::IsReturnsFuture::value，根据这个来看，R的type应该是个CallableResult，而如果这个CallableResult
	没有被Future包裹的话，那么特化的IsFuture是从std::false_type继承而来，也就是有一个false的typedef value
	R::ReturnFutureType就是函数调用的结果在外面套了一个Future struct

	总的来说，这个std::enable_if的意思就是如果R这个CallableResult的result type没有被Future包裹的话，就在
	外面包裹一个Future struct；如果R这个CallableResult的result type被Future包裹了的话，std::enable_if
	没有value这个typedef，匹配失败，进而trigger SFINAE机制
	*/
	template<typename F, typename R, typename... Args>
	typename std::enable_if<!R::IsReturnsFuture::value, typename R::ReturnFutureType>::type
	_thenImpl(Scheduler* sched, F&& f, ResultOfWrapper<F, Args...>) {
		static_assert(sizeof...(Args) <= 1, "Then must take zero/one argument");

		// R应该是个CallableResult，那么R::IsReturnsFuture::Inner指的就是函数调用得到结果的type
		using FReturnType = typename R::IsReturnsFuture::Inner;

		Promise<FReturnType> pm;
		auto nextFuture = pm.getFuture();

		/*
		std::decay<T>::type
		把各种引用之类的修饰全部去掉，比如把const int&退化为int
		这样可以通过std::is_same正确识别初加了引用的类型
		*/
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

			guard.unlock();

			if (sched) {
				/*
				首先来看看这个lambda的3个init caputure都是什么
				t是一个被Try struct包裹的T type，它的值就是当前Future state_中的value
				f是要调用的function
				pm是之前刚刚创建的被F(Args...)所特化的Promise

				接下来在这个lambda的过程中
				首先把f调用t的结果使用Try包裹了一下，然后将pm的value设定成这个结果type
				*/
				sched->schedule([t = std::move(t), f = std::forward<FuncType>(f), pm = std::move(pm)]() mutable {
					auto result = WrapWithTry(f, std::move(t));
					pm.setValue(std::move(result));
				});
			}
			else {
				auto result = WrapWithTry(f, std::move(t));
				pm.setValue(std::move(result));
			}
		}
		else {
			// Set this future's then callback
			/*
			_setCallback接受的参数是一个lambda
			在这个lambda的init capture中，有三个参数
			sched，一个Scheduler的指针
			func，传进来的function
			prom，刚刚创建的promise
			这个lambda接受一个参数，一个被Try struct包裹的type
			*/
			_setCallback(
				[sched, func = std::forward<FuncType>(f), prom = std::move(pm)]
				(typename TryWrapper<T>::Type&& t) mutable {
					if (sched) {
						sched->schedule([func = std::move(func), t = std::move(t), prom = std::move(prom)]() mutable {
							auto result = WrapWithTry(func, std::move(t));
							prom.setValue(std::move(result));
						});
					}
					else {
						auto result = WrapWithTry(func, std::move(t));
						prom.setValue(std::move(result));
					}
				}
			);
		}

		return std::move(nextFuture);
	}

	// 2. F return another future type
	template<typename F, typename R, typename... Args>
	typename std::enable_if<R::IsReturnsFuture::value, typename R::ReturnFutureType>::type
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
				t = decltype(t)(std::current_exception());
			}

			guard.unlock();

			auto cb = [res = std::move(t), f = std::forward<FuncType>(f), prom = std::move(pm)]() mutable {
				// func return another future: innerFuture, when innerFuture is done, nextFuture can be done
				/*
				这里的语法有点奇怪，其实并不是res.template，而是res.(template get<Args>()...)
				指的是使用Args去特化get这个函数然后在res上进行调用
				*/
				decltype(f(res.template get<Args>()...)) innerFuture;
				if (res.hasException()) {
					// Failed if Args... is void
					innerFuture = f(typename TryWrapper<typename std::decay<Args...>::type>::Type(res.exception()));
				}
				else {
					innerFuture = f(res.template get<Args>()...);
				}

				if (!innerFuture.valid()) {
					return;
				}

				std::unique_lock<std::mutex> guard(innerFuture.state_->thenLock_);
				if (innerFuture.state_->progress_ == Progress::Timeout) {
					throw std::runtime_error("Wrong state: Timeout");
				}
				else if (innerFuture.state_->progress_ == Progress::Done) {
					typename TryWrapper<FReturnType>::Type t;

					try {
						t = std::move(innerFuture.state_->value_);
					}
					catch (const std::exception& e) {
						t = decltype(t)(std::current_exception());
					}

					guard.unlock();
					prom.setValue(std::move(t));
				}
				else {
					innerFuture._setCallback([prom = std::move(prom)](typename TryWrapper<FReturnType>::Type&& t) mutable {
						prom.setValue(std::move(t));
					});
				}
			};

			if (sched) {
				sched->schedule(std::move(cb));
			}
			else {
				cb();
			}
		}
		else {
			// Set this future's then callback
			_setCallback([sched = sched, func = std::forward<FuncType>(f), prom = std::move(pm)](typename TryWrapper<T>::Type && t) mutable {
				auto cb = [func = std::move(func), t = std::move(t), prom = std::move(prom)]() mutable {
					// because func return another future: innerFuture, when innerFuture is done, nextFuture can be done
					decltype(func(t.template get<Args>()...)) innerFuture;
					if (t.hasException()) {
						// Failed if Args... is void
						innerFuture = func(typename TryWrapper<typename std::decay<Args...>::type>::Type(t.exception()));
					}
					else {
						innerFuture = func(t.template get<Args>()...);
					}

					if (!innerFuture.valid()) {
						return;
					}
					std::unique_lock<std::mutex> guard(innerFuture.state_->thenLock_);
					if (innerFuture.state_->progress_ == Progress::Timeout) {
						throw std::runtime_error("Wrong state : Timeout");
					}
					else if (innerFuture.state_->progress_ == Progress::Done) {
						typename TryWrapper<FReturnType>::Type t;
						try {
							t = std::move(innerFuture.state_->value_);
						}
						catch (const std::exception& e) {
							t = decltype(t)(std::current_exception());
						}

						guard.unlock();
						prom.setValue(std::move(t));
					}
					else {
						innerFuture._setCallback([prom = std::move(prom)](typename TryWrapper<FReturnType>::Type&& t) mutable {
							prom.setValue(std::move(t));
						});
					}
				};

				if (sched) {
					sched->schedule(std::move(cb));
				}
				else {
					cb();
				}
			});
		}

		return std::move(nextFuture);
	}

	/*
	* When register callbacks and timeout for a future like this:
	*		Future<int> f;
	*		f.then(xx).onTimeout(yy);
	*
	* There will be one future object created except f, we call f as root future.
	* The yy callback is registered on the last future, here are the possibilities:
	* 1. xx is called, and yy is not called
	* 2. xx is not called, and yy is called
	*
	* But be careful below:
	* 
	*		Future<int> f;
	*		f.then(xx).then(yy).onTimeout(zz);
	*
	* There will be 3 future objects created except f, we call f as root future.
	* The zz callback is registed on the last future, here are the possiblities:
	* 1. xx is called, and zz is called, yy is not called.
	* 2. xx and yy are called, and zz is called, aha, it's rarely happend but...
	* 3. xx and yy are called, it's the normal case.
	* So, you may shouldn't use OnTimeout with chained futures!!!
	*/
	void onTimeout(std::chrono::milliseconds duration, std::function<void()> f, Scheduler* scheduler) {
		scheduler->schedulerLater(duration, [state = state_, cb = std::move(f)]() mutable {
			std::unique_lock<std::mutex> guard(state->thenLock_);
			if (state->progress_ != Progress::None) {
				return;
			}

			state->progress_ = Progress::Timeout;
			guard.unlock();

			cb();
		});
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

// Make ready future
template<typename T2>
inline Future<typename std::decay<T2>::type> makeReadyFuture(T2&& value) {
	Promise<typename std::decay<T2>::type> pm;
	auto f(pm.getFuture());
	pm.setValue(std::forward<T2>(value));

	return f;
}

inline Future<void> makeReadyFuture() {
	Promise<void> pm;
	auto f(pm.getFuture());
	pm.setValue();

	return f;
}

// Make exception future
template<typename T2, typename E>
inline Future<T2> makeExceptionFuture(E&& exp) {
	Promise<T2> pm;
	pm.setException(std::make_exception_ptr(std::forward<E>(exp)));

	return pm.getFuture();
}

template<typename T2>
inline Future<T2> makeExceptionFuture(std::exception_ptr&& eptr) {
	Promise<T2> pm;
	pm.setException(std::move(eptr));

	return pm.getFuture();
}

}  // namespace Quokka