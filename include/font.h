#pragma once

#include <string>

std::string processHebrewText(const std::wstring& input);
std::string wstringToString(std::wstring wide);
bool isHebrew(wchar_t ch);