// Copyright Millicast 2022. All Rights Reserved.
#pragma once

#include <string>

namespace Millicast::Player
{

	inline std::string to_string(const FString& Str)
	{
		auto Ansi = StringCast<ANSICHAR>(*Str, Str.Len());
		std::string Res{ Ansi.Get(), static_cast<SIZE_T>(Ansi.Length()) };
		return Res;
	}

	inline FString ToString(const std::string& Str)
	{
		auto Conv = StringCast<TCHAR>(Str.c_str(), Str.size());
		FString Res{ Conv.Length(), Conv.Get() };
		return Res;
	}

}

#ifdef _MSC_VER
#if _MSVC_LANG >= 202002L
#define MILLICAST_HAS_CXX20 1
#endif
#else
#if __cplusplus >= 202002L
#define MILLICAST_HAS_CXX20 1
#endif
#endif

#ifndef MILLICAST_HAS_CXX20
#define MILLICAST_HAS_CXX20 0
#endif
