#pragma once

#include <iostream>
#include <string>
#include <vector>
#define NOMINMAX
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <wtypes.h>

#include <shellapi.h>
#include <Psapi.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// bassically an assert
#define TRY(ptr) do { if (ptr == NULL) {  return EXIT_FAILURE; }} while (0)
#define TRY_PRINT(ptr, msg) do { if (ptr == NULL) { std::cerr << msg; return EXIT_FAILURE; }} while (0)
#define TRY_PRINT_DO(ptr, msg, func) do { if (ptr == NULL) { std::cerr << msg; func; return EXIT_FAILURE; }} while (0)

struct ProcessInfo {
	DWORD processId = 0;
	std::wstring processName = L"";
	std::wstring processPath = L"";
	GLuint textureId = 0;
};

std::string wstringToString(std::wstring wide);
std::string ErrorToString(DWORD errorMessageID);
std::string GetLastErrorAsString();
BOOL EnableDebugPrivilege();
DWORD GetProcessIDByWindow(const std::wstring& name);
int injectDll(HANDLE hProcess, const std::wstring& dllPath);
void CopyToClipboard(const std::string& text);
std::vector<ProcessInfo> EnumerateRunningApplications();
BOOL TerminateProcessEx(DWORD dwProcessId, UINT uExitCode);