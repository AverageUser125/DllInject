#pragma once

#include <imgui.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "coreDllInj.hpp"

constexpr int screenHeight = 720;
constexpr int screenWidth = 1280;
std::vector<ProcessInfo> EnumerateRunningApplications();
int RenderProcessSelector();
void guiInit();
ProcessInfo guiLoop();
void guiCleanup();
GLuint LoadIconAsTexture(HICON hIcon);