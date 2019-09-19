#pragma once

#include <chrono>
#include <functional>

namespace Quokka {

class Scheduler {
public:

	virtual ~Scheduler() {}

	virtual void schedulerLater(std::chrono::milliseconds duration, std::function<void()> f) = 0;
	virtual void schedule(std::function<void()> f) = 0;
};

}  // namespace Quokka