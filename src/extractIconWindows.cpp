
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "extractIcon.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <WinUser.h>
#include <shellapi.h>

std::map<std::wstring, GLuint> loadedIcons;

GLuint LoadIconAsTexture(HICON hIcon) {
	// Get the icon's dimensions
	ICONINFO iconInfo;
	GetIconInfo(hIcon, &iconInfo);

	// Create a device context and a bitmap
	HDC hdc = GetDC(NULL);
	HBITMAP hBitmap = iconInfo.hbmColor; // Use the color bitmap
	BITMAP bmp{};
	GetObject(hBitmap, sizeof(BITMAP), &bmp);

	// Create an OpenGL texture
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Convert the bitmap to a texture
	std::vector<BYTE> data(bmp.bmWidthBytes * bmp.bmHeight);
	GetBitmapBits(hBitmap, data.size(), data.data());

	// Upload the texture to OpenGL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp.bmWidth, bmp.bmHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE, data.data());

	// Clean up
	DeleteObject(iconInfo.hbmColor);
	DeleteObject(iconInfo.hbmMask);
	ReleaseDC(NULL, hdc);

	return textureID;
}

void getIcon(const ProcessInfo& info, void* handle) {
	// Get window icon
	HICON hIcon = (HICON)GetClassLongPtrW(reinterpret_cast<HWND>(handle), GCLP_HICON);
	if (!hIcon || hIcon == LoadIcon(NULL, IDI_APPLICATION)) {
		// get from .exe instead of windows
		hIcon = ExtractIconW(0, info.processPath.c_str(), 0);
	}
	if (!hIcon) {
		hIcon = LoadIconA(NULL, IDI_APPLICATION); // Fallback icon
	}
	auto tempTexture = LoadIconAsTexture(hIcon);
	// info.textureId = tempTexture;
	DestroyIcon(hIcon);

	// Store the loaded icon for future reuse
	loadedIcons[info.processPath] = tempTexture;
}
#endif