// FiberScheduler.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Types.h"
#include "Misc.h"
#include "Fiber/FiberJob.h"
#include "Worker.h"
#include <map>
#include <array>


class FiberWorker;
class FiberJob;
class JobSignal;
class FiberDesc;


// class FiberScheduler
//------------------------------------------------------------------------------
class FiberScheduler
{
public:
	using WorkersArray  = std::vector<FiberWorker*>;
	using PrioJobsArray = std::vector<std::vector<FiberJobPtr>>;
	using JobPool       = std::map<uint64, PrioJobsArray>;
	using ReadyFibers   = std::vector<std::vector<FiberDesc*>>;
	using LoopFibers    = std::vector<FiberDesc*>;
	using PendingJobs   = std::vector<std::vector<FiberJobPtr>>;

	FiberScheduler();
	~FiberScheduler();

	void InitWorker(uint8 count = 1);
	void ShutDown();

	FiberJobPtr  PostJob(std::shared_ptr<Job> job, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	FiberJobPtr  PostJob(std::shared_ptr<Job> job, JobSignalPtr signal, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	template<class Functor>
	FiberJobPtr  PostJob(Functor&& func, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	template<class Functor>
	FiberJobPtr  PostJob(Functor&& func, JobSignalPtr signal, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	void         YieldFor(JobSignalPtr signal);
	void         YieldPoll(uint32 intervalMS);
	void         WakeUpWorkers(uint64 workerFilter);

	FiberDesc*   FetchFiber(bool lock = true);
	void         FreeFiber(FiberDesc* fiber, bool lock = true);
	JobSignalPtr FetchSignal();

	void AddPreCondition(JobSignalPtr signal, JobSignalPtr condition);	

	static void Poll(void* data);

public:
	void         PushJob(FiberJobPtr fiberJob, bool lock = true);
	FiberJobPtr  PopJob(uint64 workerFilter);
	FiberDesc*   PopFiber(uint32 workerID);
	FiberDesc*   PopLoopFiber(uint32 workerID, int32& remainMS);
	FiberWorker* GetWorkerByID(uint32 id);

	bool         HasJobReady(uint64 workerFilter, bool lock = true);
	FiberJobPtr  PopPendingJob(uint32 workerFilter);

	FiberJobAllocator&  GetJobAllocator() { return m_JobAllocator; }
	FiberDescAllocator& GetDescAllocator() { return m_FiberAllocator; }
	JobSignalAllocator& GetSignalAllocator() { return m_SignalAllocator; }

	std::mutex m_Lock;
	std::mutex m_JobLock;

private:
	FiberJobPtr  _PostJob(std::shared_ptr<Job> job, uint64 worker, bool lock);
	FiberJobPtr  _PostJob(std::shared_ptr<Job> job, JobSignalPtr signal, uint64 worker, bool lock);
	void         _PushJobPending(int32 count, uint64 workerFilter);

	WorkersArray        m_Workers;
	ReadyFibers         m_ReadyFibers;
	LoopFibers          m_LoopFibers;
	JobPool             m_Jobs;
	PendingJobs         m_PendingJobs;

	std::vector<FiberDesc*> m_FreeFibers;
	FiberJobAllocator   m_JobAllocator;
	FiberDescAllocator  m_FiberAllocator;
	JobSignalAllocator  m_SignalAllocator;

	friend class FiberWorker;
	friend class FiberJob;
};


template<class Functor>
FiberJobPtr FiberScheduler::PostJob(Functor&& func, uint64 worker)
{
	std::shared_ptr<Job> job = std::allocate_shared<FuncJob>(m_JobAllocator, std::forward<Functor>(func));
	return PostJob(job, worker);
}

template<class Functor>
FiberJobPtr FiberScheduler::PostJob(Functor&& func, JobSignalPtr signal, uint64 worker)
{
	std::shared_ptr<Job> job = std::allocate_shared<FuncJob>(m_JobAllocator, std::forward<Functor>(func));
	return PostJob(job, signal, worker);
}

//------------------------------------------------------------------------------
