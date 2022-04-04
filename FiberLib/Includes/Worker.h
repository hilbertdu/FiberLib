// Worker
//------------------------------------------------------------------------------
#pragma once
#ifndef ENGINE_SCHEDULER_WORKER_H
#define ENGINE_SCHEDULER_WORKER_H

// Includes
//------------------------------------------------------------------------------
#include "Misc.h"
#include "Types.h"
#include <atomic>
#include <shared_mutex>
#include <condition_variable>
#include <thread>

#define THREAD_COUNT_MIN (sizeof(char) * 8)
#define THREAD_COUNT_MAX (sizeof(uint64) * 8)

enum ThreadWorkerID : uint8
{
	E_WORKER_MAIN,
	E_WORKER_RENDER,
	E_WORKER_COMPUTE,
	E_WORKER_IO_1,
	E_WORKER_IO_2,
	E_WORKER_SHARED_1,
	E_WORKER_SHARED_2,
	E_WORKER_SHARED_3,

	E_WORKER_MIN_COUNT
};


enum ThreadWorkerFilter : uint64
{
	E_WORKER_ON_MAIN			= 1 << E_WORKER_MAIN,
	E_WORKER_ON_RENDER			= 1 << E_WORKER_RENDER,
	E_WORKER_ON_COMPUTE			= 1 << E_WORKER_COMPUTE,
	E_WORKER_ON_IO_1			= 1 << E_WORKER_IO_1,
	E_WORKER_ON_IO_2			= 1 << E_WORKER_IO_2,
	E_WORKER_ON_SHARED_1		= 1 << E_WORKER_SHARED_1,
	E_WORKER_ON_SHARED_2		= 1 << E_WORKER_SHARED_2,
	E_WORKER_ON_SHARED_3		= 1 << E_WORKER_SHARED_3,

	E_WORKER_ON_ANY				= 0xFFFF'FFFF'FFFF'FFFF,
	E_WORKER_ON_ANY_EXCEPT_MAIN = E_WORKER_ON_ANY & ~E_WORKER_ON_MAIN,
	E_WORKER_ON_IO				= E_WORKER_ON_IO_1 | E_WORKER_ON_IO_2,
	E_WORKER_ON_SHARED			= E_WORKER_ON_SHARED_1 | E_WORKER_ON_SHARED_2 | E_WORKER_ON_SHARED_3,
};


// class ThreadWorker
//------------------------------------------------------------------------------
class ThreadWorker
{
public:
	explicit ThreadWorker(const char* name, uint32 threadID);
	virtual ~ThreadWorker();

	void Init();
	void SetAffinityMask(uint64 mask);

	void Sleep(std::unique_lock<std::mutex>& lock, uint32 timeMS = 0);
	void WakeUp();

	FORCE_INLINE void               SetStopped() { m_Stopped = true; };
	FORCE_INLINE bool               IsStopped() const { return m_Stopped; };
	FORCE_INLINE bool               IsFinished() const { return m_Exited; }
	FORCE_INLINE uint32             GetThreadID() const { return m_ThreadID; }
	FORCE_INLINE uint64             GetThreadFilterID() const { return m_ThreadFilterID; }
	FORCE_INLINE const std::string& GetThreadName() const { return m_ThreadName; }

	static uint32        GetCurrentThreadID();
	static uint64        GetCurrentThreadFilter();
	static ThreadWorker* GetCurrentThreadWorker();

protected:
	static uint32 ThreadWrapperFunc(void* param);
	virtual void  Main() = 0;

protected:  
	std::string             m_ThreadName;
	uint32                  m_ThreadID;
	uint64                  m_ThreadFilterID;
	std::thread             m_Thread;
	std::condition_variable m_CV;

	std::atomic<bool> m_Stopped;
	std::atomic<bool> m_Exited;
};


#define SelfThreadID ThreadWorker::GetCurrentThreadID()

//------------------------------------------------------------------------------
#endif // ENGINE_SCHEDULER_WORKER_H 