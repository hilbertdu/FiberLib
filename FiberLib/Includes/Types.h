// Types.h
//------------------------------------------------------------------------------
#pragma once
#ifndef FOUNDATION_PLATFORM_TYPES_H
#define FOUNDATION_PLATFORM_TYPES_H

// Includes
//------------------------------------------------------------------------------
#include <stdint.h>

// Common types
//------------------------------------------------------------------------------
typedef unsigned char 		uint8;		// 8-bit  unsigned.
typedef unsigned short int	uint16;		// 16-bit unsigned.
typedef unsigned int		uint32;		// 32-bit unsigned.
typedef unsigned long long	uint64;		// 64-bit unsigned.

typedef	signed char			int8;		// 8-bit  signed.
typedef signed short int	int16;		// 16-bit signed.
typedef signed int	 		int32;		// 32-bit signed.
typedef signed long long	int64;		// 64-bit signed.

typedef char				CHAR;		// ANSI character
typedef wchar_t				WCHAR;		// wide character
typedef uint8				CHAR8;		// 8-bit character
typedef uint16				CHAR16;		// 16-bit character
typedef uint32				CHAR32;		// 32-bit character

typedef size_t				SIZET;

#if defined(__WIN64__)
	typedef int64			INTPTR;
	typedef uint64			UINTPTR;
#elif defined(__WIN32__)
	typedef int32			INTPTR;
	typedef uint32			UINTPTR;
#endif


//------------------------------------------------------------------------------
#endif // FOUNDATION_PLATFORM_TYPES_H
