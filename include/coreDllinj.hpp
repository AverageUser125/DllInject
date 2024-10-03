#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <glad/glad.h>
// bassically an assert
#define TRY(ptr) do { if (ptr == NULL) {  return EXIT_FAILURE; }} while (0)
#define TRY_PRINT(ptr, msg) do { if (ptr == NULL) { std::cerr << msg; return EXIT_FAILURE; }} while (0)
#define TRY_PRINT_DO(ptr, msg, func) do { if (ptr == NULL) { std::cerr << msg; func; return EXIT_FAILURE; }} while (0)


#include <unistd.h>

#ifdef __linux__
struct ProcessInfo {
	pid_t processId = 0;
	std::wstring processName = L"";
	std::wstring processPath = L"";
	std::wstring iconPath = L"";
};
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
struct ProcessInfo {
	pid_t processId = 0;
	std::wstring processName = L"";
	std::wstring processPath = L"";
};
#endif
extern std::map<std::wstring, GLuint> loadedIcons;

// proccess Id is not a valid key, since there can be the same program launched multiple times
// the only way without the path is to make an int64 identifier which just is a counter
bool EnableDebugPrivilege();
void TerminateProcessEx(const ProcessInfo& info);
void EnumerateRunningApplications(std::vector<ProcessInfo>& cachedProcesses);
std::wstring resolveAbsolutePath(const std::string& relativePath);
int injectDll(const ProcessInfo& info, const std::wstring& dllPath);
