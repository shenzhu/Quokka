#include <algorithm>

#include "Timer.h"

namespace Quokka {

unsigned int TimerManager::s_timerIdGen_ = 0;

TimerManager::TimerManager() {
}

TimerManager::~TimerManager() {
}

void TimerManager::update() {
	if (timers_.empty()) return;

	const auto now = std::chrono::steady_clock::now();

	for (auto it = timers_.begin(); it != timers_.end();) {
		if (it->first > now) return;

		it->second.onTimer();

		Timer timer(std::move(it->second));
		it = timers_.erase(it);

		if (timer.count_ != 0) {
			const auto timePoint = timer.id_->first;
			auto itNew = timers_.insert(std::make_pair(timePoint, std::move(timer)));
			if (it == timers_.end() || itNew->first < it->first) {
				it = itNew;
			}
		}
	}
}

template<int RepeatCount, typename Duration, typename F, typename... Args>
TimerId TimerManager::scheduleAtWithRepeat(const TimePoint& triggerTime, const Duration& period, F&& f, Args&& ... args) {
	static_assert(RepeatCount != 0, "Can not add timer with zero count");

	Timer t(triggerTime);

	t.interval_ = std::max(std::chrono::milliseconds(1), std::chrono::duration_cast<std::chrono::milliseconds>(period));
	t.count_ = RepeatCount;

	TimerId id = t.id();

	t.setCallback(std::forward<F>(f), std::forward<Args>(args)...);
	timers_.insert(std::make_pair(triggerTime, std::move(t)));

	return id;
}

template<int RepeatCount, typename Duration, typename F, typename... Args>
TimerId TimerManager::scheduleAfterWithRepeat(const Duration& period, F&& f, Args&& ... args) {
	const auto now = std::chrono::steady_clock::now();
	return scheduleAtWithRepeat<RepeatCount>(now + period,
		period,
		std::forward<F>(f),
		std::forward<Args>(args)...);
}

template<typename F, typename... Args>
TimerId TimerManager::scheduleAt(const TimePoint& triggerTime, F&& f, Args&& ... args) {
	return scheduleAtWithRepeat(triggerTime,
		std::chrono::milliseconds(0),
		std::forward<F>(f),
		std::forward<Args>(args)...);
}

template<typename Duration, typename F, typename... Args>
TimerId TimerManager::scheduleAfter(const Duration& duration, F&& f, Args&& ... args) {
	const auto now = std::chrono::steady_clock::now();
	return scheduleAt(now + duration,
		std::forward<F>(f),
		std::forward<Args>(args)...);
}

bool TimerManager::cancel(TimerId id) {
	// Find in multimap(timers_) all the records whose key equals to
	// id->first(std::chrono::steady_clock::time_point)
	auto begin = timers_.lower_bound(id->first);
	if (begin == timers_.end()) return false;

	auto end = timers_.upper_bound(id->first);

	for (auto it = begin; it != end; ++it) {
		if (it->second.uniqueId() == id->second) {
			it->second.count_ = 0;
			return true;
		}
	}

	return false;
}

std::chrono::milliseconds TimerManager::nearestTimer() const {
	if (timers_.empty()) return std::chrono::milliseconds::max();

	const auto& timer = timers_.begin()->second;
	auto now = std::chrono::steady_clock::now();

	if (now > timer.id()->first) {
		return std::chrono::milliseconds::min();
	}
	else {
		return std::chrono::duration_cast<std::chrono::milliseconds>(timer.id()->first - now);
	}
}

TimerManager::Timer::Timer(const TimePoint& tp) :
	id_(std::make_shared<std::pair<TimePoint, unsigned int>>(tp, ++TimerManager::s_timerIdGen_)),
	interval_(0),
	count_(kForever) {
}

TimerManager::Timer::Timer(Timer&& rhs) noexcept:
	id_(std::move(rhs.id_)),
	func_(std::move(rhs.func_)),
	interval_(std::move(rhs.interval_)),
	count_(rhs.count_) {
}

TimerManager::Timer& TimerManager::Timer::operator=(Timer&& rhs) noexcept {
	if (this != &rhs) {
		id_ = std::move(rhs.id_);
		func_ = std::move(rhs.func_);
		interval_ = std::move(rhs.interval_);
		count_ = rhs.count_;
	}

	return *this;
}

void TimerManager::Timer::onTimer() {
	if (!func_ || count_ == 0) return;

	if (count_ == kForever || count_-- > 0) {
		func_();
		id_->first += interval_;
	}
	else {
		count_ = 0;
	}
}

TimerId TimerManager::Timer::id() const {
	return id_;
}

unsigned int TimerManager::Timer::uniqueId() const {
	return id_->second;
}

template <typename F, typename... Args>
void TimerManager::Timer::setCallback(F&& f, Args&& ... args) {
	func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
}

}  // namespace Quokka
