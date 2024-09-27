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
#include <map>
// bassically an assert
#define TRY(ptr) do { if (ptr == NULL) {  return EXIT_FAILURE; }} while (0)
#define TRY_PRINT(ptr, msg) do { if (ptr == NULL) { std::cerr << msg; return EXIT_FAILURE; }} while (0)
#define TRY_PRINT_DO(ptr, msg, func) do { if (ptr == NULL) { std::cerr << msg; func; return EXIT_FAILURE; }} while (0)

struct ProcessInfo {
	DWORD processId = 0;
	std::wstring processName = L"";
	std::wstring processPath = L"";
};

// proccess Id is not a valid key, since there can be the same program launched multiple times
// the only way without the path is to make an int64 identifier which just is a counter
extern std::map<std::wstring, GLuint> loadedIcons;

std::string wstringToString(std::wstring wide);
bool EnableDebugPrivilege();
void TerminateProcessEx(const ProcessInfo& info);
void EnumerateRunningApplications(std::vector<ProcessInfo>& cachedProcesses);

int injectDll(const ProcessInfo& info, const std::wstring& dllPath);