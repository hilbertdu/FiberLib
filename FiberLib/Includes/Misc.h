// Misc.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include <assert.h>

// Warning disabling
//------------------------------------------------------------------------------
#if defined(__WINDOWS__)
	#define PRAGMA_DISABLE_PUSH_MSVC(num)	__pragma(warning(push))			\
											__pragma(warning(disable:num))
	#define PRAGMA_DISABLE_POP_MSVC         __pragma(warning(pop))
#else
	// Do nothing on non-msvc platforms
	#define PRAGMA_DISABLE_PUSH_MSVC(num)
	#define PRAGMA_DISABLE_POP_MSVC
#endif

// Force inline
//------------------------------------------------------------------------------
#if defined(__WINDOWS__)
	#define DEPR			__declspec( deprecated )
	#define NO_INLINE		__declspec( noinline )
	#define FORCE_INLINE	__forceinline
#elif defined(__APPLE__)
	#define DEPR			__attribute__( deprecated )
	#define NO_INLINE		inline __attribute__((noinline))
	#define FORCE_INLINE	inline __attribute__((always_inline))
#endif

// Thread local
//------------------------------------------------------------------------------
#if defined(__WINDOWS__)
	#define THREAD_LOCAL __declspec(thread)
#else
	#define THREAD_LOCAL __thread
#endif

// Memory barrier
//------------------------------------------------------------------------------
#if defined(__LINUX__) || defined(__APPLE__)
	#define MemoryBarrier() __asm__ __volatile__("")
#endif

// Compile print
//------------------------------------------------------------------------------
#if defined __COMPILE_DEBUG__
#define StaticPrint(msg) __pragma(message(msg))
#else
#define StaticPrint(msg)
#endif

// Debug run
//------------------------------------------------------------------------------
#if defined __DEBUG__
	#define DebugRun(...)	__VA_ARGS__
	#define ASSERT(...)		assert(__VA_ARGS__)
#else
	#define DebugRun(...)
	#define ASSERT(...)
#endif

//------------------------------------------------------------------------------
#define KILOBYTE	    (1024)
#define MEGABYTE	    (1024 * 1024)
#define TXT(x)		    x
#define UNUSED(x)	    
#define TMAX(a, b)	    (a) > (b) ? (a) : (b)
#define TMIN(a, b)	    (a) < (b) ? (a) : (b)
#define TCLAMP(x, a, b) TMAX(a, (TMIN(x, b)))

//------------------------------------------------------------------------------
