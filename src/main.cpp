#include "coreDllinj.hpp"
#include <cstdio>
#include <limits.h>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>
#include "font.h"
#include "gui.hpp"

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

	static const char* relativePath = "./" DLL_NAME "." DLL_EXTENSION;

	std::wstring absolutePath = resolveAbsolutePath(relativePath);
	std::wcout << absolutePath;

	guiLoop(absolutePath);

	return EXIT_SUCCESS;
}