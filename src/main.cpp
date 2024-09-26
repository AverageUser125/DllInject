#include "coreDllinj.hpp"
#include "gui.hpp"


int main() {
	EnableDebugPrivilege();
	#ifndef NO_CONSOLE
	// Enable output of Unicode in the console
	SetConsoleOutputCP(CP_UTF8);
	std::wcout.imbue(std::locale("en_US.UTF-8"));
    #endif 

	// convert to absolute path
	WCHAR absoluteDllPath[MAX_PATH]{};
	GetFullPathNameW(L"./" DLL_NAME ".dll", MAX_PATH, absoluteDllPath, nullptr);
	std::wcout << L"DLL PATH: " << absoluteDllPath << std::endl;

	guiInit();
	guiLoop(absoluteDllPath);
	guiCleanup();
	// DWORD processId = GetProcessIDByWindow("Untitled - Notepad");
	// TRY(processId);

	return EXIT_SUCCESS;
}