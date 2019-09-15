#pragma once

#include <chrono>
#include <functional>
#include <map>
#include <memory>

namespace Quokka {

namespace {

using TimePoint = std::chrono::steady_clock::time_point;
using TimerId = std::shared_ptr<std::pair<TimePoint, unsigned int>>;

constexpr int kForever = -1;

}  // end namespace

class TimerManager final {
public:

	TimerManager();
	~TimerManager();

	// Tick
	void update();

	/*
	* Schedule timer at absolute timepoint then repeat with period
	* @param triggerTime: The absolute time when timer fist triggered
	* @param period: After first trigger, will be triggered by this period repeated until RepeatCount
	* @param f: The function to execute
	* @param args: Args for f
	*/
	template<int RepeatCount, typename Duration, typename F, typename... Args>
	TimerId scheduleAtWithRepeat(const TimePoint& triggerTime, const Duration& period, F&& f, Args&&... args);

	/*
	* Schedule timer with period
	* @param period: Timer will be triggered every period
	*/
	template<int RepeatCount, typename Duration, typename F, typename... Args>
	TimerId scheduleAfterWithRepeat(const Duration& period, F&& f, Args&&... args);

	/*
	* Schedule timer at timepoint
	* @param triggerTime: The absolute time when timer trigger
	*/
	template<typename F, typename... Args>
	TimerId scheduleAt(const TimePoint& triggerTime, F&& f, Args&&... args);

	/*
	* Schedule timer after duration
	* @param duration: After duration, timer will be triggered
	*/
	template<typename Duration, typename F, typename... Args>
	TimerId scheduleAfter(const Duration& duration, F&& f, Args&&... args);

	/*
	* How far the nearest timer will be trigger.
	*/
	std::chrono::milliseconds nearestTimer() const;

	/*
	* Cancel timer
	* @param @id : Id of timer
	*/
	bool cancel(TimerId id);

private:

	class Timer {
		friend class TimerManager;

	public:

		explicit Timer(const TimePoint& tp);

		// Move only
		Timer(Timer&& rhs) noexcept;
		Timer& operator=(Timer&& rhs) noexcept;

		// Delete copy operations
		Timer(const Timer& rhs) = delete;
		Timer& operator=(const Timer& rhs) = delete;

		template<typename F, typename... Args>
		void setCallback(F&& f, Args&& ... args);

		void onTimer();

		// Returns id for Timer
		TimerId id() const;
		unsigned int uniqueId() const;

	private:

		TimerId id_;

		std::function<void()> func_;
		std::chrono::milliseconds interval_;
		int count_;
	};

	std::multimap<TimePoint, Timer> timers_;

	static unsigned int s_timerIdGen_;
};

}  // namespace Quokka