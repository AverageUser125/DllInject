#include "coreDllinj.hpp"
#include <cstdio>
#include <limits.h>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>
#include "font.h"

int main() {
	EnableDebugPrivilege();

    #if !defined(NO_CONSOLE) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
	AllocConsole();
	(void)freopen("conin$", "r", stdin);
	(void)freopen("conout$", "w", stdout);
	(void)freopen("conout$", "w", stderr);
	std::cout.sync_with_stdio();
	std::wcout.sync_with_stdio();

	// Enable output of Unicode in the console
	SetConsoleOutputCP(CP_UTF8);
	std::wcout.imbue(std::locale("en_US.UTF-8"));
	std::cout.imbue(std::locale("en_US.UTF-8"));
    #endif 

	static const char* relativePath = "./" DLL_NAME ".so";

	std::wstring absolutePath = resolveAbsolutePath(relativePath);
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