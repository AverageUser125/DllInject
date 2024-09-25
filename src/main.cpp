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
	GetFullPathNameW(L"./MyDLL.dll", MAX_PATH, absoluteDllPath, nullptr);
	std::wcout << L"DLL PATH: " << absoluteDllPath << std::endl;

	guiInit();
	std::vector<ProcessInfo> processes = EnumerateRunningApplications();
	ProcessInfo chosenProcess = guiLoop(processes);
	guiCleanup();
	DWORD processId = chosenProcess.processId;
	TRY(processId);
	// DWORD processId = GetProcessIDByWindow("Untitled - Notepad");
	// TRY(processId);

	// Open a handle to the target process
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	TRY_PRINT(processHandle, "Failed to get process handle");

	// int result = injectDll(processHandle, absoluteDllPath);

	std::wcout << chosenProcess.processName << std::endl;

	int result = EXIT_SUCCESS;

	if (result == EXIT_SUCCESS) {
		std::wcout << L"DONE!" << std::endl;
	}
	return result;
}