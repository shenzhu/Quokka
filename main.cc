#include <iostream>

#include "util/StringView.h"
#include "util/TimeUtil.h"
#include "util/Timer.h"

int main() {
	Quokka::StringView sv("Hello", 5);
	std::cout << sv << std::endl;

	Quokka::Time time;
	std::cout << time.formatTime() << std::endl;
}