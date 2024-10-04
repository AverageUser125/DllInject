#include "coreDllinj.hpp"
#ifdef __linux__
#include <unistd.h>
#include <dlfcn.h>
#include <limits.h>
#include <csignal>
#include <sys/types.h>
#include "injector.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <locale>
#include <string>
#include <codecvt>
#include <map>
#include "extractIcon.hpp"

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

// Function to resolve a relative path on POSIX systems
std::wstring resolveAbsolutePath(const std::string& relativePath) {
	char absolutePath[PATH_MAX];

	if (realpath(relativePath.c_str(), absolutePath) == nullptr) {
		std::cerr << "Error resolving absolute path: " << relativePath << std::endl;
		return L""; // Return empty string on failure
	}

	// Convert std::string to std::wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(absolutePath); // Return the absolute path
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

std::wstring GetIconPathForProcess(const std::wstring& processName) {
	std::string desktopFilePath;
	for (const auto& entry : std::filesystem::directory_iterator("/usr/share/applications")) {
		if (entry.path().filename() == (processName + L".desktop")) {
			desktopFilePath = entry.path().string();
			break;
		}
	}

	std::ifstream desktopFile(desktopFilePath);
	std::string line;
	while (std::getline(desktopFile, line)) {
		if (line.find("Icon=") != std::string::npos) {
			return std::wstring(line.substr(5).begin(), line.substr(5).end()); // Extract icon path
		}
	}

	return L""; // Return empty if no icon is found
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
		ProcessInfo info;
		info.processId = pid;
		// Read the command line to get process name and path
		std::ifstream cmdlineFile(entry.path() / "cmdline");
		if (!cmdlineFile) {
			continue;
		}
		std::string cmdline;
		std::getline(cmdlineFile, cmdline);
		if (cmdline.empty()) {
			continue;
		}
		size_t pos = cmdline.find('\0');
		info.processName =
			std::wstring(cmdline.begin(), cmdline.begin() + (pos != std::string::npos ? pos : cmdline.size()));

		if (info.processName == L"-bash") {
			continue;
		}
		info.processPath = std::wstring(cmdline.begin(), cmdline.end());

		// Check if the icon is already loaded
		auto it = loadedIcons.find(info.processName);
		
		if (it != loadedIcons.end()) {
			// Icon is already loaded, reuse existing texture
			// textureID = it->second;
		} else {
			getIcon(info, nullptr);
		}

		// Add the process information to the cachedProcesses vector
		cachedProcesses.push_back(info);
	}
}



int injectDll(const ProcessInfo& info, const std::wstring& dllPath) {

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string dllPathStr = converter.to_bytes(dllPath);

    injector_t* injector = nullptr;

	if (injector_attach(&injector, info.processId) != 0) {
		printf("ATTACH ERROR: %s\n", injector_error());
		return -1;
	}

	if (injector_inject(injector, dllPathStr.c_str(), NULL) != 0) {
		printf("INJECT ERROR: %s\n", injector_error());
	}

	injector_detach(injector);
	return 0;
}
#endif