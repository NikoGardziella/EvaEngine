#include "pch.h"
#include "Engine/Utils/PlatformUtils.h"

#include <commdlg.h> // defines the 32-Bit Common Dialog APIs  
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include "Engine/Core/Application.h"

namespace Engine {


	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn; // Struct for open file dialog
		char filePath[MAX_PATH] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		//ofn.nFilterIndex = 1;
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
		ofn.lpstrDefExt = strchr(filter, '\0') + 1; // Default extension from filter

		if (GetOpenFileNameA(&ofn))
		{
			return std::string(filePath);
		}

		return std::string(); // Return empty string if dialog was canceled
	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		char filePath[MAX_PATH] = { 0 };

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(OPENFILENAMEA);
		ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)Application::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFilter = filter;
		ofn.lpstrFile = filePath;
		ofn.nMaxFile = MAX_PATH;
		//ofn.nFilterIndex = 1;
		ofn.Flags = OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = strchr(filter, '\0') + 1; // Default extension from filter

		if (GetSaveFileNameA(&ofn))
		{
			return std::string(filePath);
		}

		return std::string(); // Return empty string if dialog was canceled
	}
}




