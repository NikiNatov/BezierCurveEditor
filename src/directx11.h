#pragma once

#include <vector>
#include <iostream>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <string>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <dxgi.h>
#include <d3d11.h>
#include <wrl.h>
#include <dxgidebug.h>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#if defined(_DEBUG)

#define DXCall(hr)																																	   \
		if(FAILED(hr))																																   \
		{																																			   \
			LPSTR error;																													           \
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,\
				nullptr,																															   \
				hr,																																	   \
				0,																							                                           \
				(LPSTR)&error,																														   \
				512,																																   \
				nullptr																																   \
			);																																		   \
			printf("DirectX Error:\n");																									               \
			printf("[Description]: %s", error);																								           \
			__debugbreak();																															   \
		}
#else
#define DXCall(hr) hr
#endif

#define Align(size, alignment) ((size + (alignment - 1)) & ~(alignment - 1))
