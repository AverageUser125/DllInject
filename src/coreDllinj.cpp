#include "coreDllinj.hpp"
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#include <csignal>
#include <sys/types.h>
#include "injector.h"

bool EnableDebugPrivilege() {
	// Check if the effective user ID is 0 (root)
	if (geteuid() == 0) {
		std::cout << "Running with root privileges." << std::endl;
		return true;
	}

	std::cout << "No sufficient privileges for debugging." << std::endl;
	return false;
}

void TerminateProcessEx(const ProcessInfo& info) {
	if (info.processId <= 0) {
		std::cerr << "Invalid process ID." << std::endl;
		return;
	}

	// Send SIGTERM signal to the process
	if (kill(info.processId, SIGTERM) == 0) {
		std::cout << "Process " << info.processId << " terminated successfully." << std::endl;
	} else {
		perror("Failed to terminate process");
	}
}

void EnumerateRunningApplications(std::vector<ProcessInfo>& cachedProcesses) {

}


int injectDll(const ProcessInfo& info, const std::string& dllPath) {
	/*
	std::cout << "hello world\n";
	void* handle = dlopen(dllPath.c_str(), RTLD_LAZY);
	if (!handle) {
		std::cerr << "Error loading .so file: " << dlerror() << std::endl;
		return 1;
	}


	// Unload the shared library
	if (dlclose(handle) != 0) {
		std::cerr << "Error unloading .so file: " << dlerror() << std::endl;
		return 1;
	}
	*/
	
    injector_t* injector = nullptr;

	if (injector_attach(&injector, info.processId) != 0) {
		printf("ATTACH ERROR: %s\n", injector_error());
		return -1;
	}

	if (injector_inject(injector, dllPath.c_str(), NULL) != 0) {
		printf("INJECT ERROR: %s\n", injector_error());
	}

	injector_detach(injector);
	return 0;
}