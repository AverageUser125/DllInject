#include "coreDllinj.hpp"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cctype>
#include <WinUser.h>
#include <shellapi.h>
#include <algorithm>
#include "gui.hpp"
#include <Psapi.h>
#include <map>
#include <codecvt>
#include "extractIcon.hpp"

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

// Function to resolve a DLL path on Windows
std::wstring resolveAbsolutePath(const std::string& relativePath) {
	WCHAR absoluteDllPath[MAX_PATH]{};

	// Convert std::string to std::wstring
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::wstring wRelativePath = converter.from_bytes(relativePath);

	if (GetFullPathNameW(wRelativePath.c_str(), MAX_PATH, absoluteDllPath, nullptr) == 0) {
		std::wcerr << L"Error resolving absolute path: " << wRelativePath << std::endl;
		return L""; // Return empty string on failure
	}

	return absoluteDllPath; // Return the absolute path
}

bool EnableDebugPrivilege() {
	HANDLE hToken;
	LUID luid;
	TOKEN_PRIVILEGES tkp{};

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		std::cout << GetLastErrorAsString();
		return 0;
	}

	if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
		std::cout << GetLastErrorAsString();
		return 0;
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
			return 0;
		}
		std::cout << ErrorToString(errId);
		return 0;
	}

	if (!CloseHandle(hToken)) {
		std::cout << GetLastErrorAsString();
		return 0;
	}

	return 1;
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

int injectDll(const ProcessInfo& info, const std::wstring& dllPath) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, info.processId);
	TRY_PRINT(hProcess, "Can't open process");

	LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	TRY_PRINT(dllPathAddress, "Failed to allocate memory in target process");

	const auto freeMem = [hProcess, dllPathAddress]() { VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE); };

	TRY_PRINT_DO(
		WriteProcessMemory(hProcess, dllPathAddress, dllPath.data(), (dllPath.size() + 1) * sizeof(wchar_t), NULL),
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

void EnumerateRunningApplications(std::vector<ProcessInfo>& cachedProcesses) {
	std::vector<HWND> windowHandles = getWindows();
	std::vector<ProcessInfo> newProcesses;
	newProcesses.reserve(windowHandles.size() + 1);

	constexpr DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];


	// Map to store loaded icons by process path (ensures icons are loaded once per program)

	for (const auto& windowHandle : windowHandles) {
		ProcessInfo info{};

		// Get window title
		GetWindowTextW(windowHandle, windowTitle, TITLE_SIZE);
		info.processName = &windowTitle[0];

		// Skip irrelevant windows
		if (info.processName.empty() || info.processName == L"Program Manager" ||
			info.processName == L"Windows Input Experience") {
			continue;
		}

		// Get process ID
		if (GetWindowThreadProcessId(windowHandle, &info.processId) == NULL) {
			info.processId = 0;
		}

		// Check if this process is already cached
		auto it = std::find_if(cachedProcesses.begin(), cachedProcesses.end(),
							   [&](const ProcessInfo& p) { return p.processId == info.processId; });

		if (it != cachedProcesses.end()) {
			newProcesses.push_back(*it); // Reuse the existing process info
			continue;
		}

		// Process is new, gather additional information

		// Open process handle
		HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, info.processId);
		if (processHandle != NULL) {
			WCHAR filename[MAX_PATH]{};
			if (GetModuleFileNameExW(processHandle, NULL, filename, MAX_PATH) == 0) {
				info.processPath = L"null";
			} else {
				info.processPath = filename;
			}
			CloseHandle(processHandle);
		}

		// Check if this process path has already loaded an icon
		auto iconIt = loadedIcons.find(info.processPath);
		if (iconIt != loadedIcons.end()) {
			// Icon already loaded, reuse it
			// info.textureId = iconIt->second;
		} else {
			getIcon(info, windowHandle);
		}

		// Add the new process to the list (one entry per process)
		newProcesses.push_back(info);
	}

	// Replace old cached processes with the updated list
	cachedProcesses = std::move(newProcesses);
}

void TerminateProcessEx(const ProcessInfo& info) {
	DWORD dwDesiredAccess = PROCESS_TERMINATE;
	BOOL bInheritHandle = FALSE;
	DWORD dwProcessId = info.processId;
	HANDLE hProcess = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (hProcess == NULL)
		return;

	BOOL result = TerminateProcess(hProcess, 1);

	CloseHandle(hProcess);
}

#endif