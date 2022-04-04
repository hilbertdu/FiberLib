// Fiber.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Types.h"
#include "Job.h"
#include "Worker.h"
#include <memory_resource>
#include <chrono>


class FiberWorker;
class JobSignal;
class FiberJob;
class FiberScheduler;

using JobSignalPtr = std::shared_ptr<JobSignal>;
using FiberJobPtr = std::shared_ptr<FiberJob>;
using TimerMS = std::chrono::milliseconds;

// class FiberJob
//------------------------------------------------------------------------------
class FiberJob
{
public:
	FiberJob();
	FiberJob(FiberScheduler* sche);
	FiberJob(FiberScheduler* sche, std::shared_ptr<Job> job);
	~FiberJob();

	FORCE_INLINE JobSignalPtr         GetSignal() const { return m_JobSignal; }
	FORCE_INLINE FiberScheduler*      GetScheduler() const { return m_Scheduler; }
	FORCE_INLINE uint32               GetWorkerID() const { return m_WorkerID; }
	FORCE_INLINE uint64               GetWorkerFilter() const { return m_WorkerFilter; }
	FORCE_INLINE std::shared_ptr<Job> GetJob() const { return m_Job; }

	FORCE_INLINE void StartCounter() { m_TimeStamp = std::chrono::duration_cast<TimerMS>(std::chrono::steady_clock::now().time_since_epoch()); }
	FORCE_INLINE bool IsTimeout() const { return m_HoldTime == 0 ? false : m_TimeStamp.count() > m_HoldTime; }
	FORCE_INLINE void SetMaxPeriod(uint32 period) { m_HoldTime = period; }

	FORCE_INLINE void SetStatus(Job::Status status) { m_Job->SetStatus(status); }
	FORCE_INLINE void SetPreResult(int32 result) { m_Job->SetPreResult(result); }

	int32 Execute();

	FiberJobPtr PostSuccessor(std::shared_ptr<Job> job, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	FiberJobPtr PostCompletor(std::shared_ptr<Job> job);
	template<class Functor>
	FiberJobPtr PostSuccessor(Functor&& job, uint64 worker = ThreadWorkerFilter::E_WORKER_ON_ANY_EXCEPT_MAIN);
	template<class Functor>
	FiberJobPtr PostCompletor(Functor&& job);

private:
	std::shared_ptr<Job> m_Job;
	JobSignalPtr         m_JobSignal;
	uint32               m_WorkerID;
	uint64               m_WorkerFilter;
	FiberScheduler*      m_Scheduler;
	TimerMS              m_TimeStamp;
	uint32               m_HoldTime;

	friend class FiberScheduler;
	friend class JobSignal;
};

template<class Functor>
FiberJobPtr FiberJob::PostSuccessor(Functor&& func, uint64 worker)
{
	std::shared_ptr<Job> job = std::allocate_shared<FuncJob>(m_Scheduler->GetJobAllocator(), std::forward<Functor>(func));
	return PostSuccessor(job, worker);
}

template<class Functor>
FiberJobPtr FiberJob::PostCompletor(Functor&& func)
{
	std::shared_ptr<Job> job = std::allocate_shared<FuncCompletorJob>(m_Scheduler->GetJobAllocator(), std::forward<Functor>(func));
	return PostCompletor(job);
}

using FiberJobAllocator = std::pmr::polymorphic_allocator<FiberJob>;


// class JobSignal
//------------------------------------------------------------------------------
class JobSignal
{
public:
	JobSignal();
	JobSignal(FiberScheduler* scheduler);

	void Trigger(int32 result);	
	void AddTrigger(JobSignalPtr signal, bool inc = true);

private:
	bool IsValid();
	void PushJob(FiberJobPtr job, bool lock);

	FiberScheduler*           m_Scheduler;
	std::vector<FiberJobPtr>  m_NextJobs;
	std::atomic<int32>        m_RefCount;
	std::vector<JobSignalPtr> m_Triggers;
	std::mutex                m_Mutex;

	friend class FiberJob;
	friend class FiberScheduler;
};

using JobSignalAllocator = std::pmr::polymorphic_allocator<JobSignal>;



// class FiberDesc
//------------------------------------------------------------------------------
class FiberDesc
{
public:
	FORCE_INLINE int32 GetRemainTime() const { return (int32)(m_LoopMS - m_StartMS.count()); }
	FORCE_INLINE void  StartCounter() { m_StartMS = std::chrono::duration_cast<TimerMS>(std::chrono::steady_clock::now().time_since_epoch()); }

	void*           m_Fiber{ nullptr };
	FiberScheduler* m_Scheduler{ nullptr };
	FiberJobPtr     m_CurrentJob;
	TimerMS         m_StartMS;
	uint32          m_LoopMS{ 0 };
};

using FiberDescAllocator = std::pmr::polymorphic_allocator<FiberDesc>;


//------------------------------------------------------------------------------
