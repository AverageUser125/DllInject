#include "coreDllinj.hpp"
#include <cstdio>
#include <limits.h>
#include <iostream>

int main() {
	//EnableDebugPrivilege();
	static const char* relativePath = "./relaunchDll.so";

	// Buffer to store the absolute path
	char absolutePath[PATH_MAX];

	// Convert relative path to absolute path
	if (realpath(relativePath, absolutePath) == nullptr) {
		std::cerr << "Error resolving absolute path: " << relativePath << std::endl;
		return 1;
	}


	ProcessInfo info;
	std::cout << "enter ID: ";
	std::cin >> info.processId;

	injectDll(info, absolutePath);


	return EXIT_SUCCESS;
}