
#pragma once

#include <imgui.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "coreDllInj.hpp"

constexpr auto REFRESH_TIME = 2.0;

constexpr int screenHeight = 720;
constexpr int screenWidth = 1280;
void guiInit();
void RenderProcessSelector(std::vector<ProcessInfo> processes, const std::wstring& absoluteDllPath);
void guiLoop(const std::wstring& absoluteDllPath);
void guiCleanup();
GLuint LoadIconAsTexture(HICON hIcon);
