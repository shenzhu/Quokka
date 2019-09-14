#include <iostream>

#include "util/StringView.h"
#include "util/TimeUtil.h"

int main() {
	Quokka::StringView sv("Hello", 5);
	std::cout << sv << std::endl;

	Quokka::Time time;
	std::cout << time.formatTime() << std::endl;
}