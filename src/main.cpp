#include "coreDllinj.hpp"
#include <cstdio>
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>

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

	std::cout << "hello world\n";
	void* handle = dlopen(absolutePath, RTLD_LAZY);
	if (!handle) {
		std::cerr << "Error loading .so file: " << dlerror() << std::endl;
		return 1;
	}


	// Unload the shared library
	if (dlclose(handle) != 0) {
		std::cerr << "Error unloading .so file: " << dlerror() << std::endl;
		return 1;
	}
	// DWORD processId = GetProcessIDByWindow("Untitled - Notepad");
	// TRY(processId);

	return EXIT_SUCCESS;
}