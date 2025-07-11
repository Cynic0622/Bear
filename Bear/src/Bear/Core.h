#pragma once

#ifdef BEAR_API_EXPORTS
	#define BEAR_API __declspec(dllexport)
#else
	#define BEAR_API __declspec(dllimport)
#endif