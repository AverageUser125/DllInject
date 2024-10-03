#include "dll.h" 

#ifdef __linux__

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <limits.h>
#include <dlfcn.h>
#include <errno.h>
#include <cstdlib>

#define TRY_AND_DO(ptr, func) do { if (ptr == NULL) {func;  return; }} while (0)
#define TRY(ptr) do { if (ptr == NULL) { return; }} while (0)

// Function to get the absolute path of the shared library
std::string getLibraryPath() {
	Dl_info dl_info;
	if (dladdr((void*)getLibraryPath, &dl_info)) {
		return dl_info.dli_fname; // dli_fname contains the path to the .so file
	}
	return std::string(); // Return an empty string on failure
}

std::vector<std::string> getExecutableAndArgs() {
	std::ifstream cmdline("/proc/self/cmdline");
	std::vector<std::string> args;
	std::string arg;

	if (cmdline.is_open()) {
		while (std::getline(cmdline, arg, '\0')) {
			args.push_back(arg);
		}
		cmdline.close();
	} else {
		std::cerr << "Could not open /proc/self/cmdline" << std::endl;
	}

	return args;
}

void preventDetach() {
	// Get the original application path and arguments
	std::vector<std::string> args = getExecutableAndArgs();
	if (args.size() < 2) { // Ensure there are at least two arguments
		std::cerr << "Failed to retrieve the application path and arguments." << std::endl;
		return;
	}

	std::string str;
	for (const auto& s : args) {
		str += s + " ";
	}

	// Check the conditions for aborting
	if (args[0] != "/bin/bash" || (args[1] != "./test.sh" && args[1] != "/home/user/.vs/test.sh")) {
		std::cerr << str << " ABORTING\n";
		return;
	}

	const std::string preloadPath = getLibraryPath();

	std::cout << "Detaching and relaunching " << str << std::endl;

	// Construct the command without additional quotes
	std::string command = "LD_PRELOAD=" + preloadPath + " " + args[0] + " " + args[1];

	// Create a char* array for execvp
	std::vector<char*> cExecArgs;
	cExecArgs.push_back(const_cast<char*>("bash"));
	cExecArgs.push_back(const_cast<char*>("-c"));
	cExecArgs.push_back(const_cast<char*>(command.c_str()));
	cExecArgs.push_back(nullptr); // Null-terminate the array

	// Print the contents of cExecArgs for verification
	std::cout << "cExecArgs:" << std::endl;
	for (const char* arg : cExecArgs) {
		if (arg != nullptr) {
			std::cout << arg << std::endl;
		} else {
			std::cout << "nullptr" << std::endl; // Indicate the null terminator
		}
	}

	execvp(cExecArgs[0], cExecArgs.data());

	// If execvp fails
	std::cerr << "Failed to exec " << str << std::endl;
	exit(EXIT_FAILURE); // Exit if exec fails
}

// This function will run when the shared object is loaded
//__attribute__((constructor)) void onLoad() {
//	std::cout << "SHARED LIBRARY LOADED\n";
//}

// This function will run when the shared object is unloaded
__attribute__((destructor)) void onUnload() {
	preventDetach();
}
#endif
