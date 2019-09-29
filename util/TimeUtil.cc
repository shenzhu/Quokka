#include <sstream>

#include "TimeUtil.h"

namespace Quokka {

Time::Time() : valid_(false) {
	now();
}

void Time::now() {
	now_ = std::chrono::system_clock::now();
	valid_ = false;
}

uint64_t Time::milliSeconds() const {
	return std::chrono::duration_cast<std::chrono::milliseconds>(now_.time_since_epoch()).count();
}

uint64_t Time::microSeconds() const {
	return std::chrono::duration_cast<std::chrono::microseconds>(now_.time_since_epoch()).count();
}

std::string Time::formatTime() const {
	std::stringstream ss;
	ss << getYear() << "-" << getMonth() << "-" << getDay()
		<< "[" << getHour() << ":" << getMinute() << ":" << getSecond()
		<< "." << (milliSeconds() % 1000000) << "]";

	return ss.str();
}

inline int Time::getYear() const {
	updateTm();
	return tm_.tm_year + 1900;
}

inline int Time::getMonth() const {
	updateTm();
	return tm_.tm_mon + 1;
}

inline int Time::getDay() const {
	updateTm();
	return tm_.tm_mday;
}

inline int Time::getHour() const {
	updateTm();
	return tm_.tm_hour;
}

inline int Time::getMinute() const {
	updateTm();
	return tm_.tm_min;
}

inline int Time::getSecond() const {
	updateTm();
	return tm_.tm_sec;
}

void Time::updateTm() const {
	if (valid_) return;

	valid_ = true;

    const time_t now(milliSeconds() / 1000UL);
	// Using local time instead of UTC time
	// localtime_r(&now, &tm_);
}

inline Time::operator uint64_t () const {
	return milliSeconds();
}

}  // namespace Quokka
