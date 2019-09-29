#include <algorithm>
#include <cassert>

#include "ThreadPool.h"

namespace Quokka {

thread_local bool ThreadPool::working_ = true;
std::thread::id ThreadPool::s_mainThread;

ThreadPool::ThreadPool()
	: currentThreads_(0),
	waiters_(0),
	shutdown_(false) {

	// Returns the number of concurrent threads supported by the implementation
	maxIdleThreads_ = std::max(1U, std::thread::hardware_concurrency());
	maxThreads_ = kMaxThreads;
	pendingStopSignal_ = 0;

	// Init main thread id
	s_mainThread = std::this_thread::get_id();

	monitor_ = std::thread([this]() {
		_monitorRoutine();
	});
}

ThreadPool::~ThreadPool() {
	joinAll();
}

void ThreadPool::joinAll() {
	if (s_mainThread != std::this_thread::get_id()) {
		return;
	}

	decltype(workers_) tmp;

	{
		std::unique_lock<std::mutex> guard(mutex_);
		if (shutdown_) {
			return;
		}

		shutdown_ = true;
		cond_.notify_all();

		tmp.swap(workers_);
	}

	for (auto& t : tmp) {
		if (t.joinable()) {
			t.join();
		}
	}

	if (monitor_.joinable()) {
		monitor_.join();
	}
}

void ThreadPool::setMaxIdleThreads(unsigned int m) {
	if (0 < m && m <= kMaxThreads) {
		maxIdleThreads_ = m;
	}
}

void ThreadPool::setMaxThreads(unsigned int m) {
	if (0 < m && m <= kMaxThreads) {
		maxThreads_ = m;
	}
}

void ThreadPool::_monitorRoutine() {
	while (!shutdown_) {
		// Do NOT use mutex and cond, otherwise `Execute` will awake me
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		std::unique_lock<std::mutex> guard(mutex_);
		if (shutdown_) {
			return;
		}

		// 首先将nw设置为正在等待的线程的数量
		auto nw = waiters_;

		// If there is any pending stop signal to consume waiters
		nw -= pendingStopSignal_;

		while (nw-- > maxIdleThreads_) {
			tasks_.push_back([this]() {
				this->working_ = false;
			});
			cond_.notify_one();
			++pendingStopSignal_;
		}
	}
}

void ThreadPool::_workerRoutine() {
	// working_是一个thread_local的变量，表明每个thread都有一个自己的副本
	working_ = true;

	while (working_) {
		std::function<void()> task;

		{
			std::unique_lock<std::mutex> guard(mutex_);

			++waiters_;
			/*
			wait causes the current thread to block until the condition variable
			is notified or a spurious wakeup occurs,
			optionally looping until some predicate is satisfied
			这里给wait传了第二个参数，是一个谓词，returns false if the waiting should
			be continued

			当shutdown_了或者tasks_不为空的话(表明已经有task可以做了)停止等待
			并且将waiter的数量-1
			*/
			cond_.wait(guard, [this]() {
				return shutdown_ || !tasks_.empty();
			});
			--waiters_;

			assert(shutdown_ || !tasks_.empty());

			// 如果已经shutdown并且任务为空的话，就将当前thread的数量-1并返回
			if (shutdown_ && tasks_.empty()) {
				--currentThreads_;
				return;
			}

			assert(!tasks_.empty());
			task = std::move(tasks_.front());
			tasks_.pop_front();
		}

		task();
	}

	// If reach here, this thread is recycled by monitor thread
	--currentThreads_;
	--pendingStopSignal_;
}

void ThreadPool::_spawnWorker() {
	/*
	Guared by mutex

	将当前的线程数量增加1
	创建一个新的线程capture这个ThreadPool class本身，并且去执行_workRoutine()这个函数
	最后将这个新创建的线程加入workers_中去
	*/
	++currentThreads_;
	std::thread t([this]() {
		this->_workerRoutine();
	});
	workers_.push_back(std::move(t));
}

}