#include <iostream>

#include "util/StringView.h"
#include "util/TimeUtil.h"
#include "util/Timer.h"
#include "util/ThreadPool.h"

#include "future/Helper.h"
#include "future/Try.h"
#include "future/Future.h"

template<typename T>
T threadFunc() {
	std::cout << "setValue 10\n";
	T v = 10;
	return v;
}

void threadFuncV() {
	std::cout << "setValue void\n";
}

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

	Quokka::ThreadPool threadPool;

	Quokka::Future<int> ft(threadPool.execute(threadFunc<int>));
	ft.then([](int v) {
		std::cout << "1. Then got int value "
			<< v
			<< " and return float 1.0f."
			<< std::endl;

		return 1.0f;
	}).then([](float v) {
		std::cout << "2. Then got flot value " << v
			<< " and return 2."
			<< std::endl;

		return;
	}).then([]() {
		std::cout << "Finished" << std::endl;
	});
}