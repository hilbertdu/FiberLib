// Worker.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include <assert.h>
#include <chrono>
#include "Worker.h"

#if defined(__WINDOWS__)
	#include <windows.h>
#endif

// Static
//------------------------------------------------------------------------------
static THREAD_LOCAL uint32        s_WorkerThreadID = 0;
static THREAD_LOCAL ThreadWorker* s_Worker;


/*explicit*/ ThreadWorker::ThreadWorker(const char* name, uint32 threadID)
	: m_ThreadID(threadID)
	, m_ThreadFilterID((uint64)1 << threadID)
	, m_ThreadName(name)
	, m_Stopped(false)
	, m_Exited(false)
{	
	ASSERT(m_ThreadID < 64);
	SetAffinityMask(m_ThreadFilterID);
}
/*virtual*/ ThreadWorker::~ThreadWorker()
{
	while (!m_Exited) 
		std::this_thread::sleep_for(std::chrono::seconds(1));
}

void ThreadWorker::Init()
{
	m_Thread = std::thread([this]() { ThreadWrapperFunc(this); });
}

void ThreadWorker::SetAffinityMask(uint64 mask)
{
#if defined(__WINDOWS__)
	::SetThreadAffinityMask((HANDLE)m_Thread.native_handle(), mask);
#endif
}

void ThreadWorker::Sleep(std::unique_lock<std::mutex>& lock, uint32 timeMS)
{
	m_CV.wait_for(lock, std::chrono::milliseconds(timeMS));
}

void ThreadWorker::WakeUp()
{
	m_CV.notify_all();
}

/*static*/ uint32 ThreadWorker::GetCurrentThreadID()
{
	return s_WorkerThreadID;
}

/*static*/ uint64 ThreadWorker::GetCurrentThreadFilter()
{
	return (uint64)1 << s_WorkerThreadID;
}

/*static*/ ThreadWorker* ThreadWorker::GetCurrentThreadWorker()
{
	return s_Worker;
}

/*static*/ uint32 ThreadWorker::ThreadWrapperFunc(void* param)
{
	ThreadWorker* wt = reinterpret_cast<ThreadWorker*>(param);
	s_WorkerThreadID = wt->m_ThreadID;
	s_Worker = wt;
	wt->Main();
	wt->m_Exited = true;
	return 0;
}

//------------------------------------------------------------------------------