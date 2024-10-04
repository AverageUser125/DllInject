#include "font.h"

#include <string>
#include <vector>
#include <codecvt>
#include <locale>
#include <algorithm>

std::string wstringToString(std::wstring wstr) {
	// Use codecvt to convert wstring to string (UTF-8)
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);
}

bool isHebrew(wchar_t ch) {
	return (ch >= 0x0590 && ch <= 0x05FF) || (ch >= 0xFB1D && ch <= 0xFB4F);
}

// Function to process the input string
std::string processHebrewText(const std::wstring& input) {
	std::wstring result;
	std::vector<std::wstring> hebrewGroups;
	std::wstring currentHebrewGroup;

	for (size_t i = 0; i < input.length(); ++i) {
		wchar_t ch = input[i];
		if (isHebrew(ch)) {
			currentHebrewGroup += ch; // Add to the current Hebrew group
		} else {
			// If there's a current Hebrew group, reverse it, append to result, and reset
			if (!currentHebrewGroup.empty()) {
				std::reverse(currentHebrewGroup.begin(), currentHebrewGroup.end());
				result += currentHebrewGroup;
				currentHebrewGroup.clear();
			}
			result += ch; // Add the non-Hebrew character directly
		}
	}

	// If there's a remaining Hebrew group, reverse it and append to the result
	if (!currentHebrewGroup.empty()) {
		std::reverse(currentHebrewGroup.begin(), currentHebrewGroup.end());
		result += currentHebrewGroup;
	}

	return wstringToString(result); // Convert back to UTF-8 string
}