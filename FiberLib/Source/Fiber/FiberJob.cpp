// FiberJob.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Fiber/FiberWorker.h"
#include "Fiber/FiberJob.h"
#include "Fiber/FiberScheduler.h"
#include "Misc.h"
#include <mutex>


FiberJob::FiberJob()
	: m_JobSignal(nullptr)
	, m_Scheduler(nullptr)
	, m_WorkerID(0)
	, m_HoldTime(0)
{
}

FiberJob::FiberJob(FiberScheduler* sche)
	: m_JobSignal(nullptr)
	, m_Scheduler(sche)
	, m_WorkerID(0)
	, m_HoldTime(0)
{
}

FiberJob::FiberJob(FiberScheduler* sche, std::shared_ptr<Job> job)
	: m_Job(job)
	, m_Scheduler(sche)
	, m_JobSignal(sche->FetchSignal())
	, m_WorkerID()
	, m_HoldTime(0)
{
	m_JobSignal->m_RefCount ++;
}

FiberJob::~FiberJob()
{
}

int32 FiberJob::Execute()
{
	ASSERT(ThreadWorker::GetCurrentThreadFilter() & m_WorkerFilter);

	m_WorkerID = ThreadWorker::GetCurrentThreadID();	
	SetStatus(Job::Status::STATUS_RUNNING);
	int32 result = m_Job->Excute();
	SetStatus(result == 0 ? Job::Status::STATUS_SUCCESS : Job::Status::STATUS_FAILED);
	return result;
}

FiberJobPtr FiberJob::PostSuccessor(std::shared_ptr<Job> job, uint64 worker)
{
	return m_Scheduler->PostJob(job, m_JobSignal, worker);
}

FiberJobPtr FiberJob::PostCompletor(std::shared_ptr<Job> job)
{
	return m_Scheduler->PostJob(job, m_JobSignal, ThreadWorker::GetCurrentThreadFilter());
}


JobSignal::JobSignal()
	: m_Scheduler(nullptr)
	, m_RefCount(0)
{
}

JobSignal::JobSignal(FiberScheduler* scheduler)
	: m_Scheduler(scheduler)
	, m_RefCount(0)
{
}

void JobSignal::Trigger(int32 result)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	ASSERT(IsValid());

	if (--m_RefCount > 0)
	{
		return;
	}

	for (auto& job : m_NextJobs)
	{
		job->SetPreResult(result);
		m_Scheduler->PushJob(job);
	}
	for (auto& trigger : m_Triggers)
	{
		trigger->Trigger(result);
	}
	m_NextJobs.clear();
	m_Triggers.clear();
	ASSERT(m_RefCount == 0);
}

bool JobSignal::IsValid()
{
	return m_RefCount > 0;
}

void JobSignal::AddTrigger(JobSignalPtr signal, bool inc)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	if (!IsValid())
		return;

	if (inc)
		signal->m_RefCount ++;
	
	ASSERT(std::find(m_Triggers.begin(), m_Triggers.end(), signal) == m_Triggers.end());
	m_Triggers.push_back(signal);
}

void JobSignal::PushJob(FiberJobPtr job, bool lock)
{
	if (lock) m_Mutex.lock();
	ASSERT(IsValid());
	m_NextJobs.push_back(job);
	if (lock) m_Mutex.unlock();
}

//------------------------------------------------------------------------------