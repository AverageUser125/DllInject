#include "coreDllinj.hpp"
// #include <Windows.h>
#include <cctype> 
#include <WinUser.h>
#include <shellapi.h>
#include <tchar.h>
#include <algorithm>
#include "gui.hpp"
#include <cstring>
#include <fstream>

std::string wstringToString(std::wstring wide) {
	std::string str(wide.length(), 0);
	std::transform(wide.begin(), wide.end(), str.begin(), [](wchar_t c) { return (char)c; });
	return str;
}
std::string ErrorToString(DWORD errorMessageID) {

	LPSTR messageBuffer = NULL;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size =
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					   NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}
std::string GetLastErrorAsString() {
	//Get the error message ID, if any.
	DWORD errorMessageID = GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}
	return ErrorToString(errorMessageID);
}
BOOL EnableDebugPrivilege() {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp{};

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		std::cout << GetLastErrorAsString();
		return FALSE;
	}

	if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
		std::cout << GetLastErrorAsString();
		return FALSE;
	}

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// add check for partial success since it returns 1 even in the case of partial success
	AdjustTokenPrivileges(hToken, false, &tkp, sizeof(tkp), NULL, NULL);
	DWORD errId = GetLastError();
	if (errId != ERROR_SUCCESS) {
		if (errId == ERROR_NOT_ALL_ASSIGNED) {
			std::cout << "NOTICE: Run as admin to enable debug priviliges and allows to inject into more "
						 "applications\n";
			return FALSE;
		}
		std::cout << ErrorToString(errId);
		return FALSE;
	}

	if (!CloseHandle(hToken)) {
		std::cout << GetLastErrorAsString();
		return FALSE;
	}

	return TRUE;
}
DWORD GetProcessIDByWindow(const std::wstring& name) {
	HWND windowHandle = FindWindowW(NULL, name.c_str());
	if (windowHandle == NULL) {
		std::cout << "Window not found\n";
		return NULL;
	}
	DWORD processId = NULL;
    DWORD retValue = GetWindowThreadProcessId(windowHandle, &processId);
	if (retValue == NULL || processId == NULL) {
		std::cout << GetLastErrorAsString();
		return NULL;
	}
	return processId;
}
int injectDll(HANDLE hProcess, const std::wstring& dllPath) {
	std::wcout << dllPath;

	LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	TRY_PRINT(dllPathAddress, "Failed to allocate memory in target process");

	const auto freeMem = [hProcess, dllPathAddress]() { VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE); };	

	TRY_PRINT_DO(WriteProcessMemory(hProcess, dllPathAddress, dllPath.data(), (dllPath.size() + 1) * sizeof(wchar_t), NULL),
		"failed to write to process memory dll path", freeMem());

	// Get the address of the LoadLibraryA function in the kernel32.dll module
	HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
	TRY_PRINT_DO(kernel32, "failed to get kernel32.dll module handle", freeMem());

	LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(kernel32, "LoadLibraryW");
	TRY_PRINT_DO(loadLibraryAddress, "failed to get LoadLibraryW", freeMem());

	// Create a remote thread in the target process to load the DLL
	HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress,
											 dllPathAddress, NULL, NULL);
	TRY_PRINT_DO(remoteThread, "failed to create remote thread on target", freeMem);

	// Wait for the remote thread to finish
	WaitForSingleObject(remoteThread, INFINITE);

	// Free the memory allocated in the target process
	freeMem();

	return EXIT_SUCCESS;
}
BOOL CALLBACK speichereFenster(HWND hwnd, LPARAM lParam) {
	if (!IsWindowVisible(hwnd)) {
		return TRUE;
	}

	// Retrieve the pointer passed into this callback, and re-'type' it.
	// The only way for a C API to pass arbitrary data is by means of a void*.
	std::vector<HWND>& hwndHandles = *reinterpret_cast<std::vector<HWND>*>(lParam);
	hwndHandles.push_back(hwnd);

	return TRUE;
}
std::vector<HWND> getWindows() {
	std::vector<HWND> titles;
	EnumWindows(speichereFenster, reinterpret_cast<LPARAM>(&titles));
	return titles;
}
GLuint LoadIconAsTexture(HICON hIcon);
std::vector<ProcessInfo> EnumerateRunningApplications() {
	std::vector<ProcessInfo> processes;
	std::vector<HWND> windowHandles = getWindows();
	processes.reserve(windowHandles.size());

	constexpr DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];
	for (const auto& windowHandle : windowHandles) {
		ProcessInfo info{};

		GetWindowTextW(windowHandle, windowTitle, TITLE_SIZE);
		info.processName = &windowTitle[0];
		
		// skip alot of explorer.exe shenanigans
		if (info.processName.length() == 0 || info.processName == L"Program Manager" || info.processName == L"Windows Input Experience") {
			continue;
		}

		// get process ID
		if (GetWindowThreadProcessId(windowHandle, &info.processId) == NULL) {
			info.processId = 0;
		}

		HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, info.processId);
		if (processHandle != NULL) {
			WCHAR filename[MAX_PATH]{};

			if (GetModuleFileNameExW(processHandle, NULL, filename, MAX_PATH) == 0)
			{
				info.processPath = L"null";
			} else {
				info.processPath = filename;
			}

			CloseHandle(processHandle);
		}

		// get window icon
		HICON hIcon = (HICON)GetClassLongPtr(windowHandle, GCLP_HICON);
		// If no custom icon or default system icon, load icon from the executable
		if (!hIcon || hIcon == LoadIcon(NULL, IDI_APPLICATION)) {
			hIcon = ExtractIconW(0, info.processPath.c_str(), 0);
		}
		// just use default system icon
		if (!hIcon) {
			hIcon = LoadIconA(NULL, IDI_APPLICATION); // Fallback icon
		}
		info.textureId = LoadIconAsTexture(hIcon);
		DestroyIcon(hIcon);
		processes.push_back(info);
	}
	return processes;
}
void CopyToClipboard(const std::string& text) {
	if (OpenClipboard(NULL)) {
		EmptyClipboard(); // Clear existing clipboard content

		// Allocate a global memory object for the text
		HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, text.size() + 1);
		if (hClipboardData) {
			// Lock the memory and copy the text to it
			char* pchData = (char*)GlobalLock(hClipboardData);
			if (pchData != NULL) {
				strcpy_s(pchData, text.size() + 1, text.c_str());
				GlobalUnlock(hClipboardData);
			}
			// Place the handle on the clipboard
			SetClipboardData(CF_TEXT, hClipboardData);
		}
		CloseClipboard();
	}
}

BOOL TerminateProcessEx(DWORD dwProcessId, UINT uExitCode) {
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL bInheritHandle = FALSE;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (hProcess == NULL)
		return FALSE;

	BOOL result = TerminateProcess(hProcess, uExitCode);

	CloseHandle(hProcess);

	return result;
}