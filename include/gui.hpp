#pragma once

#include <imgui.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "coreDllInj.hpp"

constexpr int screenHeight = 720;
constexpr int screenWidth = 1280;
std::vector<ProcessInfo> EnumerateRunningApplications();
void guiInit();
void RenderProcessSelector(const std::wstring& absoluteDllPath);
void guiLoop(const std::wstring& absoluteDllPath);
void guiCleanup();
GLuint LoadIconAsTexture(HICON hIcon);