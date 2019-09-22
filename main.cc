#include <iostream>

#include "util/StringView.h"
#include "util/TimeUtil.h"
#include "util/Timer.h"

#include "future/Helper.h"
#include "future/Try.h"
#include "future/Future.h"

int main() {
	Quokka::StringView sv("Hello", 5);
	std::cout << sv << std::endl;

	Quokka::Time time;
	std::cout << time.formatTime() << std::endl;

	Quokka::Promise<int> promise;

	Quokka::Try<void> tryInstance;
	Quokka::Try<void> copyInstance = tryInstance;
	Quokka::Try<void> moveInstance(std::move(tryInstance));
	copyInstance.print();
	moveInstance.print();
}