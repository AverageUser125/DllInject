#include "coreDllinj.hpp"
#include <cstdio>
#include <limits.h>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>

std::string wstringToString(std::wstring wstr) {
	// Use codecvt to convert wstring to string (UTF-8)
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

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
	std::vector<ProcessInfo> start;
	EnumerateRunningApplications(start);

	for (const auto& info : start) {
		std::cout << "PID: " << info.processId 
			<< ", NAME: " << wstringToString(info.processName)
			<< ", PATH: " << wstringToString(info.processPath)
			<< '\n'; 
	
	}

	ProcessInfo info;
	std::cout << "enter ID: ";
	std::cin >> info.processId;

	injectDll(info, absolutePath);


	return EXIT_SUCCESS;
}