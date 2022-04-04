// Fiber.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Types.h"


// namespace Fiber
//------------------------------------------------------------------------------
namespace Fiber
{
	using FiberProc = void(*)(void*);
	void* InitFromThread();
	void* CreateFiber(int stacksize, FiberProc proc, void* parameter);
	void  DestroyFiber(void* fiber);
	void  SwitchTo(void* fiber);
}

//------------------------------------------------------------------------------
