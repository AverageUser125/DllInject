#include "dll.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#define TRY_AND_DO(ptr, func) do { if (ptr == NULL) {func;  return; }} while (0)
#define TRY(ptr) do { if (ptr == NULL) { return; }} while (0)


#define TO_WIDE_STRING(s) L##s
#define WIDE_STRINGIZE(x) TO_WIDE_STRING(#x)

std::wstring GetDllPath() {
	HMODULE hModule = GetModuleHandleW(TO_WIDE_STRING(DLL_NAME ".dll")); // Get the handle of the current module
	if (hModule) {
		WCHAR path[MAX_PATH];
		if (GetModuleFileNameW(hModule, path, MAX_PATH)) {
			std::wstring s = std::wstring(path); // Return the path as a std::wstring
			// showMessage(s.c_str());
			return s;
		}
	}
	return L""; // Return an empty string if failed
}

void injectDll(HANDLE hProcess, const std::wstring& dllPath) {
	LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	TRY(dllPathAddress);

	const auto freeMem = [hProcess, dllPathAddress]() { VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE); };

	TRY_AND_DO(
		WriteProcessMemory(hProcess, dllPathAddress, dllPath.data(), (dllPath.size() + 1) * sizeof(wchar_t), NULL), freeMem());

	// Get the address of the LoadLibraryA function in the kernel32.dll module
	HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
	TRY_AND_DO(kernel32, freeMem());

	LPVOID loadLibraryAddress = (LPVOID)GetProcAddress(kernel32, "LoadLibraryW");
	TRY_AND_DO(loadLibraryAddress, freeMem());

	// Create a remote thread in the target process to load the DLL
	HANDLE remoteThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)loadLibraryAddress,
											 dllPathAddress, NULL, NULL);
	TRY_AND_DO(remoteThread, freeMem);

	// Wait for the remote thread to finish
	WaitForSingleObject(remoteThread, INFINITE);

	// Free the memory allocated in the target process
	freeMem();
}

BOOL PreventDetach() {
	// Relaunch the original process or thread to prevent detaching.
	STARTUPINFOW si = {sizeof(STARTUPINFOW)};
	PROCESS_INFORMATION pi;
	WCHAR path[MAX_PATH];

	// Get the executable path of the original program
	if (GetModuleFileNameW(NULL, path, MAX_PATH)) {
		if (CreateProcessW(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
			std::wstring path = GetDllPath();
			injectDll(pi.hProcess, path.c_str());

			// Close the handles to avoid leaks
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		// MessageBoxW(NULL, L"DLL process injected successfully!", L"DLL Message", MB_OK);
		// disables thread notifications
		// without this we would get every thread attach and detach call in the process
		DisableThreadLibraryCalls(hModule);
		break;

	case DLL_THREAD_ATTACH:
		// this is called when the program closes for some reason?
		// MessageBoxW(NULL, L"DLL thread injected successfully!", L"DLL Message", MB_OK);
		break;

	case DLL_THREAD_DETACH:
		// MessageBoxW(NULL, L"DLL thread detached successfully!", L"DLL Message", MB_OK);
		break;

	case DLL_PROCESS_DETACH:
		// MessageBoxW(NULL, L"DLL process detached successfully!", L"DLL Message", MB_OK);
		return PreventDetach();
		if (lpReserved != nullptr) {
			break; // Normal termination, do nothing.
		}
		break;
	}

	return TRUE;
}
