#pragma once

#include <deque>
#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>
#include "..//future/Future.h"

/*
* A ThreadPool implementation with Future interface
*
* Usage:
*
* pool.Execute(your_heavy_work, some_args)
*     .Then(process_heavy_work_result)
*
* Here, your_heavy_work will be executed in a thread, and return Future
* immediately. When it done, function process_heavy_work_result will be called.
* The type of argument of process_heavy_work_result is the same as the return
* type of your_heavy_work.
*/

namespace Quokka {

class ThreadPool final {
public:
	ThreadPool();
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	/*
	Execute work in this pool

	Returns a future, you can register callback on it

	If the threads size not reach limit, or there are some
	idle threads, f will be executed at once.
	But if all threads are busy and threads size reach limit,
	f will be queueing and executed later.

	F returns non-void
	*/
	template<typename F, typename... Args,
		typename = typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value, void>::type,
		typename Dummy = void>
	auto execute(F && f, Args && ... args)->Future<typename std::result_of<F(Args...)>::type>;

	/*
	Same as above
	
	F returns void
	*/
	template<typename F, typename... Args,
		typename = typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value, void>::type>
	auto execute(F&& f, Args&& ... args)->Future<void>;

	// Stop thread pool and wait all threads terminate
	void joinAll();

	/*
	Set max size of idle threads

	Details about threads in pool:
	Busy threads, they are doing work on behalf of us
	Idle threads, they are waiting on a queue for new work
	Monitor thread, the internal thread, only one, it'll check
	the size of idle threads periodly, if there are too many idle
	threads, they will be recycled by monitor thread
	*/
	void setMaxIdleThreads(unsigned int);

	/*
	Set max size of idle threads

	Max threads size is the total of idle threads and busy threads,
	not include monitor thread.
	Default value is 1024
	Example: if you setMaxThreads(8), setMaxIdleThreads(2)
	and now execute 8 long working, there will be 8 busy threads,
	0 idle thread, when all work done, will be 0 busy thread, 2 idle
	threads, other 6 threads are recycled by monitor thread
	*/
	void setMaxThreads(unsigned int);

private:
	void _spawnWorker();
	void _workerRoutine();
	void _monitorRoutine();

	// Recycle redundant threads
	std::thread monitor_;

	std::atomic<unsigned> maxThreads_;
	std::atomic<unsigned> currentThreads_;
	std::atomic<unsigned> maxIdleThreads_;
	std::atomic<unsigned> pendingStopSignal_;

	/*
	变量存储

	auto
	该关键字用于两种情况:
	1. 声明变量时： 根据初始化表达式自动推断变量类型
	2. 声明函数作为函数返回值的占位符

	static
	static变量只初始化一次，除此之外它还有可见性的属性
	1. static修饰函数内的"局部"变量时，表明它不需要在进入或离开函数时创建或销毁
	且仅在函数内可见
	2. static修饰全局变量时，表明该变量仅在当前(声明它的)文件内可见
	3. static修饰类的成员变量时，则该变量被该类的所有实例共享

	register
	寄存器变量，该变量存储在CPU寄存器中，而不是RAM(栈或堆)中
	该变量的最大尺寸等于寄存器的大小
	由于是存储于寄存器中，因此不能对该变量进行取地址操作

	extern
	引用一个全局变量。当在一个文件中定义了一个全局变量时
	就可以在其它文件中使用extern来声明并引用该变

	mutable
	仅适用于类成员变量
	以mutable修饰的成员变量可以在const成员函数中修改

	thread_local
	线程周期
	thread_local修饰的变量具有如下特性
	1. 变量在线程创建时生成
	2. 线程结束时被销毁
	3. 每个线程都拥有其自己的变量副本
	4. thread_local可以和static或extern联合使用
	*/
	static thread_local bool working_;

	std::deque<std::thread> workers_;

	std::mutex mutex_;
	std::condition_variable cond_;
	unsigned waiters_;
	bool shutdown_;
	std::deque<std::function<void()>> tasks_;

	static const int kMaxThreads = 1024;
	static std::thread::id s_mainThread;
};

// If F returns something
template<typename F, typename... Args,
	typename = typename std::enable_if<!std::is_void<typename std::result_of<F(Args...)>::type>::value, void>::type,
	typename Dummy = void>
auto ThreadPool::execute(F && f, Args && ... args) -> Future<typename std::result_of<F(Args...)>::type> {
	using resultType = typename std::result_of<F(Args...)>::type;

	std::unique_lock<std::mutex> guard(mutex_);
	if (shutdown_) {
		return makeReadyFuture<resultType>(resultType());
	}

	Promise<resultType> promise;
	auto future = promise.getFuture();

	auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	auto task = [t = std::move(func), pm = std::move(promise)]() mutable {
		try {
			pm.setValue(Try<resultType>(t()));
		}
		catch (...) {
			pm.setException(std::current_exception());
		}
	};

	tasks_.emplace_back(std::move(task));
	if (waiters_ == 0 && currentThreads_ < maxThreads_) {
		_spawnWorker();
	}

	cond_.notify_one();

	return future;
}

// F returns void
template<typename F, typename... Args,
	typename = typename std::enable_if<std::is_void<typename std::result_of<F(Args...)>::type>::value, void>::type>
auto ThreadPool::execute(F&& f, Args&& ... args) -> Future<void> {
	using resultType = typename std::result_of<F(Args...)>::type;
	static_assert(std::is_void<resultType>::value, "must be void");

	std::unique_lock<std::mutex> guard(mutex_);
	if (shutdown_) {
		return makeReadyFuture();
	}

	Promise<resultType> promise;
	auto future = promise.getFuture();

	auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
	auto task = [t = std::move(func), pm = std::move(promise)]() mutable {
		try {
			t();
			pm.setValue();
		}
		catch (...) {
			pm.setException(std::current_exception());
		}
	};

	tasks_.emplace_back(std::move(task));
	if (waiters_ == 0 && currentThreads_ < maxThreads_) {
		_spawnWorker();
	}

	cond_.notify_one();

	return future;
}

} // namespace Quokka