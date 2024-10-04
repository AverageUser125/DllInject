#ifdef  __linux__
#include <filesystem>
#include <stb_image.h>
#include <fstream>
#include "extractIcon.hpp"
// Function to load image and create OpenGL texture
GLuint loadTexture(const std::string& imagePath) {
	int width, height, channels;
	unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
	if (!data) {
		std::cerr << "Failed to load image: " << imagePath << std::endl;
		return 0;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload the texture data
	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data); // Free image data after uploading to GPU

	return texture;
}

// Function to get icon from .desktop files (simplified for demonstration)
std::string findDesktopFileIcon(const std::wstring& processName) {
	// Define the paths to search for .desktop files
	const std::vector<std::string> searchPaths = {"/usr/share/applications/",
												  std::string(std::getenv("HOME")) + "/.local/share/applications/"};

	// Convert processName from std::wstring to std::string
	std::string processNameStr(processName.begin(), processName.end());

	for (const auto& path : searchPaths) {
		// Check if the directory exists
		if (!std::filesystem::exists(path)) {
			continue;
		}

		// Iterate through .desktop files in the directory
		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			if (entry.path().extension() == ".desktop") {
				std::ifstream file(entry.path());
				std::string line;
				bool appFound = false;
				std::string iconName;
				while (std::getline(file, line)) {
					if (line.find("Name=" + processNameStr) != std::string::npos) {
						appFound = true;
					}
					if (appFound && line.find("Icon=") == 0) {
						iconName = line.substr(5);
						break;
					}
				}
				if (!iconName.empty()) {
					// Check if iconName is a valid file path or use icon themes
					if (std::filesystem::exists(iconName)) {
						return iconName;
					}
					// Here you can add more logic to handle icon themes (e.g., hicolor)
					return iconName;
				}
			}
		}
	}
	return "";
}

// Main function that modifies global loadedIcons
void getIcon(const ProcessInfo& processInfo, void* handle) {

	// Search for the icon in .desktop files
	std::string iconPath = findDesktopFileIcon(processInfo.processName);

	if (iconPath.empty()) {
		std::cerr << "Icon not found for process: "
				  << std::string(processInfo.processName.begin(), processInfo.processName.end()) << std::endl;
		return;
	}

	// Load the icon as an OpenGL texture
	GLuint texture = loadTexture(iconPath);
	if (texture != 0) {
		loadedIcons[processInfo.processPath] = texture;
		std::wcout << L"Icon loaded for process: " << processInfo.processPath << std::endl;
	} else {
		std::cerr << "Failed to load texture for icon: " << iconPath << std::endl;
	}
}

#endif
