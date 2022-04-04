// Fiber.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Fiber/Fiber.h"

#if defined(__WINDOWS__)
	#include <windows.h>
#endif


//------------------------------------------------------------------------------
namespace Fiber
{
	void* InitFromThread()
	{
		return ::ConvertThreadToFiber(nullptr);
	}

	void* CreateFiber(int stacksize, FiberProc proc, void* parameter)
	{
		return ::CreateFiber(stacksize, proc, parameter);
	}

	void  DestroyFiber(void* fiber)
	{
		::DeleteFiber(fiber);
	}

	void  SwitchTo(void* fiber)
	{
		::SwitchToFiber(fiber);
	}
}

//------------------------------------------------------------------------------