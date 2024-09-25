
#include "gui.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <iostream>
#include <glad/errorReporting.hpp>
#include <algorithm>
#include <map>
#include <atomic>
#include <thread>

static GLFWwindow* window = nullptr;
static std::vector<ProcessInfo> processes;
// static std::atomic<bool> updateInProgress(false); // Atomic flag to indicate if an update is in progress

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


int RenderProcessSelector() {
	static int selectedProcess = -1;
	assert(processes.size() != 0);

	int width;
	int height;
	glfwGetWindowSize(window, &width, &height);

	// Set the next ImGui window position and size to take the entire screen
	ImGui::SetNextWindowPos(ImVec2(0, 0));						   // Top-left corner of the window
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height)); // Set size to fill the GLFW window

	// Begin the ImGui window with these dimensions
	ImGui::Begin("Running Applications", nullptr,
				 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// Adjust the size of icons and text dynamically based on window size
	float iconSize = std::max(width * 0.03f, 0.01f);	// Set icon size relative to window height

	// Set text size for the ImGui elements
	ImGui::SetWindowFontScale(1.4); // Adjust font scale relative to the default size
	
	// Use the new ImGui Tables API with 3 columns
	if (ImGui::BeginTable("ProcessTable", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
		// Set column widths
		ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, iconSize + 10); // First column for the icon
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch,
								(width / 2.0f) - (iconSize + 10)); // Process name column
		ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch,
								(width / 2.0f) - (iconSize + 10)); // Process path column

		// Create the header row
		ImGui::TableHeadersRow();

		for (int i = 0; i < processes.size(); i++) {
			const auto& info = processes[i];
			ImGui::TableNextRow();

			// Push a unique ID for this selectable item
			ImGui::PushID(i); // Use the index as a unique ID

			// Move to the first column (icon)
			ImGui::TableSetColumnIndex(0);

			// Show the process icon if available
			if (info.textureId != 0) {
				ImGui::Image((void*)(intptr_t)info.textureId, ImVec2(iconSize, iconSize));
			}
			// Move to the second column (process name)
			ImGui::TableSetColumnIndex(1);
			// Instead of wrapping the selectable itself, wrap the text inside the selectable
			if (ImGui::Selectable("##hidden_selectable", selectedProcess == i, ImGuiSelectableFlags_SpanAllColumns)) {
				selectedProcess = i;
			}

			ImGui::SameLine(); // Continue in the same line for text display

			// Display the wrapped process name
			std::string processDisplayName = wstringToString(info.processName);
			ImGui::PushTextWrapPos(ImGui::GetColumnWidth()); // Manually set the wrap width
			ImGui::TextWrapped("%s", processDisplayName.c_str());
			ImGui::PopTextWrapPos();

			// Move to the third column (process path)
			ImGui::TableSetColumnIndex(2);

			// Display the wrapped process path
			std::string processDisplayPath = wstringToString(info.processPath);
			ImGui::TextWrapped("%s", processDisplayPath.c_str());

			ImGui::PopID(); // Pop the ID after the row
		}

		ImGui::EndTable(); // End the table
	}

	// Set the size of the refresh button (e.g., 40x40 pixels)
	ImVec2 refreshButtonSize(75.0f, 40.0f);

	// Position the refresh button at the bottom right corner
	ImGui::SetCursorPos(ImVec2(width - refreshButtonSize.x - 10.0f, height - refreshButtonSize.y - 10.0f));

	// Create the refresh button
	// TODO: make it use threads
	if (ImGui::Button("Refresh", refreshButtonSize)) {
		processes = EnumerateRunningApplications();
	}

	// If a process is selected, show the "Inject DLL" button
	if (selectedProcess >= 0 && selectedProcess < processes.size()) {
		if (ImGui::Button("Inject DLL", ImVec2((float)width, 40))) {
			ImGui::End();
			return selectedProcess;
		}
	}

	ImGui::End();
	return -1;
}

void guiInit() {

	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return;
	}

	window = glfwCreateWindow(screenWidth, screenHeight, "Dll inject demo", NULL, NULL);
	if (!window) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return;
	}
	// enable error reporting
#ifndef NDEBUG
	enableReportGlErrors();
#endif
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiWindowFlags_NoResize;
	io.IniFilename = NULL;
	// io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;		  // IF using Docking Branch
	(void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	processes = EnumerateRunningApplications();
}


ProcessInfo guiLoop() {
	ProcessInfo info{};
	processes = _processes;
	while (!glfwWindowShouldClose(window) && info.processId == 0) {
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();



		int selectedIndex =  RenderProcessSelector();
		if (selectedIndex != -1) {
			info = processes[selectedIndex];
		}



		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}
	return info;
}

void guiCleanup() {
	if (window) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	// TODO: have a seperate array just for texture so this doesn't need a loop and can just do
	// glDeleteTexture(texArr.size(), texArr.data())
	for (const auto& procInfo : processes) {
		glDeleteTextures(1, &procInfo.textureId);
	}

}