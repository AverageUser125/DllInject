#include "coreDllinj.hpp"
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#include <csignal>
#include <sys/types.h>
#include "injector.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <pwd.h>


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

// Function to check if the process is a user-space process
bool IsUserSpaceProcess(pid_t pid) {
	if (pid <= 1)
		return false; // Skip init/system processes

	// Read the executable path from /proc/[pid]/exe
	char exePath[PATH_MAX];
	std::string linkPath = "/proc/" + std::to_string(pid) + "/exe";

	if (readlink(linkPath.c_str(), exePath, sizeof(exePath)) != -1) {
		std::string executable(exePath);

		// Exclude common system processes by name
		if (executable.find("/init") != std::string::npos || executable.find("systemd") != std::string::npos ||
			executable.find("systemd-timesyncd") != std::string::npos) {
			return false;
		}
		return true;
	}
	return false;
}

void EnumerateRunningApplications(std::vector<ProcessInfo>& cachedProcesses) {
	cachedProcesses.clear();

	for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
		if (!entry.is_directory())
			continue;

		std::string dirName = entry.path().filename().string();
		if (!std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
			continue;
		}
		pid_t pid = std::stoi(dirName);
		if (!IsUserSpaceProcess(pid)) {
			continue; // Only include user-space processes
		}

		// Read the command line to get process name and path
		std::ifstream cmdlineFile(entry.path() / "cmdline");
		if (cmdlineFile) {
			std::string cmdline;
			std::getline(cmdlineFile, cmdline);
			if (!cmdline.empty()) {
				size_t pos = cmdline.find('\0');
				std::wstring name =
					std::wstring(cmdline.begin(), cmdline.begin() + (pos != std::string::npos ? pos : cmdline.size()));

				if (name == L"-bash") {
					continue;
				}
				std::wstring path = std::wstring(cmdline.begin(), cmdline.end());

				// Read UID from /proc/[pid]/status
				std::ifstream statusFile(entry.path() / "status");
				uid_t uid = 0;
				if (statusFile) {
					std::string line;
					while (std::getline(statusFile, line)) {
						if (line.find("Uid:") == 0) {
							std::istringstream iss(line);
							std::string ignore;	  // To skip the first element "Uid:"
							iss >> ignore >> uid; // Read the UID
							break;
						}
					}
				}
				struct passwd* pwd = getpwuid(uid);
				std::wstring userName = pwd ? std::wstring(pwd->pw_name.begin(), pwd->pw_name.end()) : L"unknown";

				// Add the process information to the cachedProcesses vector
				cachedProcesses.push_back({pid, name, path, userName});
			}
		}
	}
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