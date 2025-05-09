#pragma once

#ifdef _WIN32
#  define U8LIB_WIN32 1
#else
#  define U8LIB_WIN32 0
#endif

#ifdef U8LIB_DLL
#	ifndef U8LIB_API
#		ifdef _WIN32
#			define U8LIB_API __declspec(dllimport)
#		else
#			define U8LIB_API __attribute__((visibility("default")))
#		endif
#	endif
#else
#	define U8LIB_API
#endif