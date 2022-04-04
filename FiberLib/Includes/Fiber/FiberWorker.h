// Worker.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Types.h"
#include "Misc.h"
#include "Worker.h"
#include <thread>
#include <atomic>

class FiberDesc;
class FiberScheduler;


// WorkerThread
//------------------------------------------------------------------------------
class FiberWorker: public ThreadWorker
{
public:
	explicit FiberWorker(const char* name, uint32 threadID);
	virtual ~FiberWorker();

	FORCE_INLINE void SetScheduler(FiberScheduler* scheduler) { m_Scheduler = scheduler; }

	static FiberWorker* GetCurrentThreadWorker();

protected:
	virtual void Main();

public:
	void*           m_MainFiber{ nullptr };
	FiberDesc*      m_CurrentFiber{ nullptr };
	FiberScheduler* m_Scheduler{ nullptr };
};

//------------------------------------------------------------------------------
