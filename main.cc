#include <iostream>

#include "util/StringView.h"

int main() {
	Quokka::StringView sv("Hello", 5);
	std::cout << sv << std::endl;
}