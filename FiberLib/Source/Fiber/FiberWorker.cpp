// Worker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Misc.h"
#include "Fiber/FiberWorker.h"
#include "Fiber/Fiber.h"
#include "Fiber/FiberScheduler.h"


static void FiberWorkerProc(void* data)
{
	FiberWorker* worker = FiberWorker::GetCurrentThreadWorker();
	FiberDesc* fiber = worker->m_Scheduler->FetchFiber();

	worker->m_CurrentFiber = fiber;
	Fiber::SwitchTo(fiber->m_Fiber);
}

/*explicit*/ FiberWorker::FiberWorker(const char* name, uint32 threadID)
	: ThreadWorker(name, threadID)
	, m_MainFiber(nullptr)
{
}

/*virtual*/ FiberWorker::~FiberWorker()
{
}

/*virtual*/ void FiberWorker::Main()
{
	m_MainFiber = Fiber::InitFromThread();
	FiberWorkerProc(nullptr);
}

/*static*/ FiberWorker* FiberWorker::GetCurrentThreadWorker()
{
	return (FiberWorker*)ThreadWorker::GetCurrentThreadWorker();
}

//------------------------------------------------------------------------------