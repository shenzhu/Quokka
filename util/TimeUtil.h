#pragma once

#include <chrono>
#include <ctime>
#include <mutex>

namespace Quokka {

class Time {
public:

	Time();

	void now();
	
	// Milli seconds since 1970
	uint64_t milliSeconds() const;
	// Micro seconds since 1970
	uint64_t microSeconds() const;

	inline int getYear() const;
	inline int getMonth() const;
	inline int getDay() const;
	inline int getHour() const;
	inline int getMinute() const;
	inline int getSecond() const;

	// Format time with format 2019-09-14[18:29:03.123]
	std::string formatTime() const;

	inline operator uint64_t () const;

private:

	std::chrono::system_clock::time_point now_;
	mutable tm tm_;
	mutable bool valid_;

	void updateTm() const;
};

}  // namespace Quokka