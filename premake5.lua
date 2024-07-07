workspace "BezierCurveEditor"
	architecture "x64"
	startproject "BezierCurveEditor"

	configurations 
	{
		"Debug",
		"Release"
	}

	outputdir = "%{cfg.buildcfg}"

	include "extern/imgui"
project "BezierCurveEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	systemversion "latest"
	staticruntime "on"
	characterset("ASCII")

	targetdir("%{wks.location}/bin/" .. outputdir)
	objdir("%{wks.location}/tmp/" .. outputdir)

	files
	{
		"%{wks.location}/src/**.cpp",
		"%{wks.location}/src/**.h",
	}

	includedirs
	{
		"%{wks.location}/src",
		"%{wks.location}/extern/glm",
		"%{wks.location}/extern/imgui",
	}

	links
	{
		"d3d11",
		"dxgi",
		"ImGui",
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

		defines
		{
			"_DEBUG"
		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

		defines
		{
			"HEXRAY_RELEASE",
			"NDEBUG"
		}