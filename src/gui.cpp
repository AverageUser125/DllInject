
#include "gui.hpp"
#include "windIcon.h"
#include "NotoSansHebrew.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/errorReporting.hpp>
#include <imguiThemes.h>
#include "font.h"

#include <chrono>

static GLFWwindow* window = nullptr;
static std::vector<ProcessInfo> sharedProcesses;

void refreshOptions() {
	EnumerateRunningApplications(sharedProcesses);
}

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

void RenderProcessSelector(std::vector<ProcessInfo> processes, const std::wstring& absoluteDllPath) {
	static int selectedProcess = -1;
	assert(processes.size() != 0);

	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// Set the next ImGui window position and size to take the entire screen
	ImGui::SetNextWindowPos(ImVec2(0, 0));						   // Top-left corner of the window
	ImGui::SetNextWindowSize(ImVec2((float)width, (float)height)); // Set size to fill the GLFW window

	// Begin the ImGui window with these dimensions
	ImGui::Begin("Running Applications", nullptr,
				 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	// Adjust the size of icons and text dynamically based on window size
	float iconSize = std::max(width * 0.03f, 0.01f); // Set icon size relative to window height

	// Set text size for the ImGui elements
	ImGui::SetWindowFontScale(1.25); // Adjust font scale relative to the default size

	// Define the height of the region for the table, leaving space for buttons at the bottom
	float tableRegionHeight = height - 75.0f; // Adjust this based on your button height and spacing

	// Start a child region for the table, which can be scrollable independently
	ImGui::BeginChild("TableRegion", ImVec2(width, tableRegionHeight), true);

	// Use the new ImGui Tables API with 3 columns
	if (ImGui::BeginTable("ProcessTable", 3,
						  ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY)) {
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

			GLuint loadedIcon = loadedIcons.at(info.processPath); 
			// Show the process icon if available
			if (loadedIcon) {
				ImGui::Image((void*)(intptr_t)loadedIcon, ImVec2(iconSize, iconSize));
			}
			// Move to the second column (process name)
			ImGui::TableSetColumnIndex(1);
			// Instead of wrapping the selectable itself, wrap the text inside the selectable
			if (ImGui::Selectable("##hidden_selectable", selectedProcess == i, ImGuiSelectableFlags_SpanAllColumns)) {
				selectedProcess = i;
			}

			ImGui::SameLine(); // Continue in the same line for text display

			// Display the wrapped process name
			std::string processDisplayName = processHebrewText(info.processName);
			ImGui::PushTextWrapPos(ImGui::GetColumnWidth()); // Manually set the wrap width
			ImGui::TextWrapped("%s", processDisplayName.c_str());
			ImGui::PopTextWrapPos();

			// Move to the third column (process path)
			ImGui::TableSetColumnIndex(2);

			// Display the wrapped process path
			std::string processDisplayPath = processHebrewText(info.processPath);
			ImGui::TextWrapped("%s", processDisplayPath.c_str());

			ImGui::PopID(); // Pop the ID after the row
		}

		ImGui::EndTable(); // End the table
	}

	ImGui::EndChild(); // End the scrollable table region

	// Position the buttons at the bottom of the window, outside of the scrollable table region
	ImGui::SetCursorPosY(height - 50.0f); // Set position at the bottom of the window, above some padding

	ImVec2 buttonSize(80.0f, 40.0f);

	// Position the refresh button at the right side
	ImGui::SetCursorPosX(width - buttonSize.x - 5.0f); // Set X position near the right edge
	if (ImGui::Button("Refresh", buttonSize)) {
		refreshOptions();
	}
	ImGui::SameLine();

	// Position the Inject DLL button to the left of the refresh button
	ImGui::SetCursorPosX(25.0f); // Position the Inject DLL button near the left side of the window
	if (selectedProcess >= 0 && selectedProcess < processes.size()) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));		   // Red background
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Hovered red
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));  // Active red
		if (ImGui::Button("Inject DLL", ImVec2((float)width - buttonSize.x - 145.0f, 40.0f))) {
			injectDll(processes[selectedProcess], absoluteDllPath);
		}
		ImGui::PopStyleColor(3);
	}

	// Add the Terminate button next to the Inject DLL button
	ImGui::SameLine();
	if (selectedProcess >= 0 && selectedProcess < processes.size()) {
		if (ImGui::Button("Terminate", ImVec2(buttonSize.x + 25.0f, buttonSize.y))) {
			// Call your process termination logic here
			TerminateProcessEx(processes[selectedProcess]); // Example function call
			refreshOptions();
		}
	}

	ImGui::End();
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
	(void)io;

	// font
	ImVector<ImWchar> ranges;
	ImFontGlyphRangesBuilder builder;
	builder.AddRanges(io.Fonts->GetGlyphRangesHebrew()); // Add one of the default ranges
	builder.BuildRanges(&ranges); // Build the final result (ordered ranges with all the unique characters submitted)
	
	// io.Fonts->AddFontDefault();
	ImFontConfig font_cfg;
	font_cfg.MergeMode = false;
	font_cfg.FontDataOwnedByAtlas = false;
	// the 22 is the font size that I choose, 22 seems nice
	ImFont* font = io.Fonts->AddFontFromMemoryTTF((void*)(NotoSansHebew_data), NotoSansHebew_size,
												  22, &font_cfg, ranges.Data);
	io.Fonts->Build();
	

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	
	imguiThemes::embraceTheDarkness();
	// Set the window icon

	const GLFWimage icon{windowIcon_width, windowIcon_height, (unsigned char*)(windowIcon)};
	glfwSetWindowIcon(window, 1, &icon);
}

void guiLoop(const std::wstring& absoluteDllPath) {
	guiInit();

	refreshOptions();

	auto lastRefreshTime = std::chrono::steady_clock::now();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		// Check if REFRESH_TIME seconds have passed
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsedTime = currentTime - lastRefreshTime;
		if (elapsedTime.count() >= REFRESH_TIME) {
			refreshOptions();
			lastRefreshTime = currentTime; // Reset the timer
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		RenderProcessSelector(sharedProcesses, absoluteDllPath);

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}


	guiCleanup();
}

void guiCleanup() {
	for (const auto& procInfo : loadedIcons) {
		glDeleteTextures(1, &procInfo.second);
	}

	if (window) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(window);
		glfwTerminate();
	}
}