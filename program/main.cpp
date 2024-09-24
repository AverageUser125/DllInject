#include <iostream>
#include <string>
#include <vector>
#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <wtypes.h>


#include <imgui.h>
// #include <Windows.h>

// bassically an assert
#define TRY(ptr) do { if (ptr == NULL) {  return EXIT_FAILURE; }} while (0)
#define TRY_PRINT(ptr, msg) do { if (ptr == NULL) { std::cout << msg; return EXIT_FAILURE; }} while (0)
#define TRY_PRINT_DO(ptr, msg, func) do { if (ptr == NULL) { std::cout << msg; func; return EXIT_FAILURE; }} while (0)

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
DWORD GetProcessIDByWindow(const LPCSTR name) {
	HWND windowHandle = FindWindow(NULL, name);
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
	const DWORD TITLE_SIZE = 1024;
	WCHAR windowTitle[TITLE_SIZE];

	GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);

	int length = ::GetWindowTextLengthW(hwnd);
	std::wstring title(&windowTitle[0]);
	if (!IsWindowVisible(hwnd) || length == 0 || title == L"Program Manager") {
		return TRUE;
	}

	// Retrieve the pointer passed into this callback, and re-'type' it.
	// The only way for a C API to pass arbitrary data is by means of a void*.
	std::vector<std::wstring>& titles = *reinterpret_cast<std::vector<std::wstring>*>(lParam);
	titles.push_back(title);

	return TRUE;
}
std::vector<std::wstring> getWindows() {
	std::vector<std::wstring> titles;
	EnumWindows(speichereFenster, reinterpret_cast<LPARAM>(&titles));
	return titles;
}


int main() {
	EnableDebugPrivilege();	
	
	// Enable output of Unicode in the console
	SetConsoleOutputCP(CP_UTF8);
	std::wcout.imbue(std::locale("en_US.UTF-8"));

	// convert to absolute path
	WCHAR absoluteDllPath[MAX_PATH]{};
	GetFullPathNameW(L"./MyDLL.dll", MAX_PATH, absoluteDllPath, nullptr);
	std::wcout << L"DLL PATH: " << absoluteDllPath << std::endl;

	DWORD processId = GetProcessIDByWindow("Untitled - Notepad");
	TRY(processId);

	// Open a handle to the target process
	HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	TRY_PRINT(processHandle, "Failed to get process handle");

	int result = injectDll(processHandle, absoluteDllPath);

	if (result == EXIT_SUCCESS) {
		std::wcout << L"DONE!" << std::endl;
	}
	return result;
}
