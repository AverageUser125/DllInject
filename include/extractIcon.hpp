#pragma once
#include "coreDllinj.hpp"

extern std::map<std::wstring, GLuint> loadedIcons;


void getIcon(const ProcessInfo& info, void* handle);


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)


#elif defined(__linux__)


#endif