// Fiber.cpp
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Fiber/FiberScheduler.h"
#include "Fiber/Fiber.h"
#include "Fiber/FiberJob.h"
#include "Fiber/FiberWorker.h"
#include <string>
#include <algorithm>


/*static*/ void FiberScheduler::Poll(void* data)
{
	FiberDesc* self = (FiberDesc*)data;
	FiberScheduler* sche = self->m_Scheduler;

	FiberWorker* worker = FiberWorker::GetCurrentThreadWorker();
	ASSERT(worker->GetThreadID() == FiberWorker::GetCurrentThreadID());
	while (!worker->IsStopped()) 
	{
		FiberDesc* fiber = nullptr;
		FiberJobPtr job;
		while (!worker->IsStopped())
		{
			std::unique_lock<std::mutex> lock(sche->m_JobLock);
			int32 remainMS = 0;
			{
				job = sche->PopPendingJob(worker->GetThreadID());
				if (job.get()) break;
			}
			{
				fiber = sche->PopLoopFiber(worker->GetThreadID(), remainMS);
				if (fiber) break;
			}
			{
				fiber = sche->PopFiber(worker->GetThreadID());
				if (fiber) break;
			}
			{
				job = sche->PopJob(worker->GetThreadFilterID());
				if (job.get()) break;
			}
			ASSERT(remainMS >= 0);
			worker->Sleep(lock, (uint32)remainMS);
		}
		if (worker->IsStopped())
			break;

		if (fiber) 
		{
			worker->m_CurrentFiber = fiber;
			ASSERT(!self->m_CurrentJob.get());
			sche->FreeFiber(self);
			Fiber::SwitchTo(fiber->m_Fiber);
			worker = FiberWorker::GetCurrentThreadWorker();
			worker->m_CurrentFiber = self;			
		}
		else 
		{
			self->m_CurrentJob = job;
			int32 result = 0;
			if (job->IsTimeout())
				job->SetStatus(Job::Status::STATUS_EXPIRED);
			else
				result = job->Execute();
			self->m_CurrentJob = nullptr;
			job->GetSignal()->Trigger(result);
			worker = FiberWorker::GetCurrentThreadWorker();
		}
	}
	sche->FreeFiber(self);
	Fiber::SwitchTo(FiberWorker::GetCurrentThreadWorker()->m_MainFiber);
}

FiberScheduler::FiberScheduler()
	: m_JobAllocator(new std::pmr::synchronized_pool_resource())
	, m_FiberAllocator(new std::pmr::synchronized_pool_resource())
	, m_SignalAllocator(new std::pmr::synchronized_pool_resource())
{
	m_Workers.reserve(64);
	m_LoopFibers.resize(64);
	m_PendingJobs.resize(64);
	m_ReadyFibers.resize(64);
}

FiberScheduler::~FiberScheduler()
{
	ShutDown();
}

void FiberScheduler::InitWorker(uint8 count)
{
	uint32 workerCount = TCLAMP(count, (uint8)THREAD_COUNT_MIN, (uint8)THREAD_COUNT_MAX);
	for (SIZET i = 0; i < workerCount; ++i) {
		std::string name = "TurboWorker_" + std::to_string(i);
		FiberWorker* worker = new FiberWorker(name.c_str(), (uint32)i);
		worker->SetScheduler(this);
		m_Workers.push_back(worker);
		worker->Init();
	}
}

void FiberScheduler::ShutDown()
{
	std::for_each(m_Workers.begin(), m_Workers.end(), [](auto& worker) { worker->SetStopped(); });
	std::for_each(m_Workers.begin(), m_Workers.end(), [](auto& worker) {
		while (!worker->IsFinished())
			worker->WakeUp();
		delete worker;
	});
	m_Workers.clear();

	std::for_each(m_FreeFibers.begin(), m_FreeFibers.end(), [this](auto& fiber) { 
		Fiber::DestroyFiber(fiber->m_Fiber);
		fiber->~FiberDesc(); 
		m_FiberAllocator.deallocate(fiber, 1); 
	});
	m_FreeFibers.clear();

	m_Jobs.clear();
	m_ReadyFibers.clear();
	m_LoopFibers.clear();
}

FiberJobPtr FiberScheduler::PostJob(std::shared_ptr<Job> job, uint64 worker)
{
	return _PostJob(job, worker, true);
}

FiberJobPtr FiberScheduler::PostJob(std::shared_ptr<Job> job, JobSignalPtr signal, uint64 worker)
{
	return _PostJob(job, signal, worker, true);
}

void FiberScheduler::YieldFor(JobSignalPtr signal)
{
	std::unique_lock<std::mutex> lock(signal->m_Mutex);
	if (!signal->IsValid())
		return;
	lock.unlock();
	FiberDesc* selfFiber = FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber;

	_PostJob(std::allocate_shared<FuncJob>(m_JobAllocator, [this, selfFiber]() {
		std::lock_guard<std::mutex> lock(m_JobLock);
		uint32 workerID = selfFiber->m_CurrentJob->GetWorkerID();
		m_ReadyFibers[workerID].push_back(selfFiber);
		m_Workers[workerID]->WakeUp();
		return 0;
	}), signal, ThreadWorkerFilter::E_WORKER_ON_ANY, true);

	FiberDesc* newFiber = FetchFiber(true);
	FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber = newFiber;
	Fiber::SwitchTo(newFiber->m_Fiber);
	FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber = selfFiber;
}

void FiberScheduler::YieldPoll(uint32 intervalMS)
{
	bool hasJob = HasJobReady(ThreadWorker::GetCurrentThreadFilter());
	if (intervalMS == 0 && !hasJob)
		return;
	
	FiberDesc* selfFiber = FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber;
	selfFiber->StartCounter();
	selfFiber->m_LoopMS = intervalMS;
	{				
		uint32 currentWorkerID = ThreadWorker::GetCurrentThreadID();
		ASSERT(!m_LoopFibers[currentWorkerID]);
		m_LoopFibers[currentWorkerID] = selfFiber;
		std::lock_guard<std::mutex> lock(m_JobLock);
		_PushJobPending(1, ThreadWorker::GetCurrentThreadFilter());
	}

	FiberDesc* newFiber = FetchFiber(false);
	FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber = newFiber;
	Fiber::SwitchTo(newFiber->m_Fiber);
	FiberWorker::GetCurrentThreadWorker()->m_CurrentFiber = selfFiber;
}

void FiberScheduler::WakeUpWorkers(uint64 workerFilter)
{
	std::for_each(m_Workers.begin(), m_Workers.end(), [workerFilter](auto& worker) {
		if (worker->GetThreadFilterID() & workerFilter)
			worker->WakeUp(); 
	});
}

FiberDesc* FiberScheduler::FetchFiber(bool lock)
{
	if (lock) m_Lock.lock();
	if (m_FreeFibers.empty())
	{
		FiberDesc* fiber = new (m_FiberAllocator.allocate(1)) FiberDesc();
		fiber->StartCounter();
		fiber->m_Fiber = Fiber::CreateFiber(64 * 1024, FiberScheduler::Poll, fiber);
		fiber->m_Scheduler = this;
		if (lock) m_Lock.unlock();
		return fiber;
	}
	FiberDesc* fiber = m_FreeFibers.back();
	m_FreeFibers.pop_back();	

	if (lock) m_Lock.unlock();
	return fiber;
}

void FiberScheduler::FreeFiber(FiberDesc* fiber, bool lock)
{
	if (lock) m_Lock.lock();
	ASSERT(fiber->m_Fiber);
	fiber->m_Scheduler = nullptr;
	fiber->m_CurrentJob = nullptr;
	fiber->m_LoopMS = 0;
	m_FreeFibers.push_back(fiber);
	if (lock) m_Lock.unlock();
}

JobSignalPtr FiberScheduler::FetchSignal()
{
	return std::allocate_shared<JobSignal>(m_SignalAllocator, this);
}

void FiberScheduler::AddPreCondition(JobSignalPtr signal, JobSignalPtr condition)
{	
	condition->AddTrigger(signal);
}

void FiberScheduler::PushJob(FiberJobPtr fiberJob, bool lock)
{	
	if (lock) m_JobLock.lock();
	fiberJob->SetStatus(Job::Status::STATUS_READY);
	auto& jobQueue = m_Jobs[fiberJob->GetWorkerFilter()];
	if (jobQueue.empty())
		jobQueue.resize((int)Job::Priority::PRIO_MAX);
	jobQueue[fiberJob->m_Job->GetPriority()].push_back(fiberJob);
	if (lock) m_JobLock.unlock();
	WakeUpWorkers(fiberJob->GetWorkerFilter());
}

FiberJobPtr FiberScheduler::PopJob(uint64 workerFilter)
{
	for (auto& jobQueue : m_Jobs)
	{
		if (jobQueue.first & workerFilter)
		{
			for (auto& jobs : jobQueue.second)
			{
				if (!jobs.empty())
				{
					FiberJobPtr job = jobs.back();
					jobs.pop_back();
					return job;
				}
			}
		}
	}
	return nullptr;
}

FiberDesc* FiberScheduler::PopFiber(uint32 workerID)
{
	auto& fiberQueue = m_ReadyFibers[workerID];
	if (!fiberQueue.empty())
	{
		FiberDesc* readyFiber = fiberQueue.back();
		fiberQueue.pop_back();
		return readyFiber;
	}
	return nullptr;
}

FiberDesc* FiberScheduler::PopLoopFiber(uint32 workerID, int32& remainMS)
{
	FiberDesc* fiber = m_LoopFibers[workerID];
	if (fiber)
	{
		remainMS = fiber->GetRemainTime();
		if (remainMS <= 0)
		{
			remainMS = 0;
			m_LoopFibers[workerID] = nullptr;
			return fiber;
		}
	}
	return nullptr;
}

FiberWorker* FiberScheduler::GetWorkerByID(uint32 id)
{
	for (auto& worker : m_Workers)
	{
		if (worker->GetThreadID() == id)
			return worker;
	}
	return nullptr;
}

bool FiberScheduler::HasJobReady(uint64 workerFilter, bool lock)
{
	if (lock) m_JobLock.lock();
	for (auto& jobQueue : m_Jobs)
	{
		if (jobQueue.first & workerFilter)
		{
			for (auto& jobs : jobQueue.second)
			{
				if (!jobs.empty())
				{
					if (lock) m_JobLock.unlock();
					return true;
				}
			}
		}
	}
	if (lock) m_JobLock.unlock();
	return false;
}

FiberJobPtr FiberScheduler::PopPendingJob(uint32 workerID)
{
	if (!m_PendingJobs[workerID].empty())
	{
		FiberJobPtr job = m_PendingJobs[workerID].back();
		m_PendingJobs[workerID].pop_back();
		return job;
	}
	return nullptr;
}

FiberJobPtr FiberScheduler::_PostJob(std::shared_ptr<Job> job, uint64 worker, bool lock)
{
	FiberJobPtr fiberJob = std::allocate_shared<FiberJob>(m_JobAllocator, FiberJob(this, job));
	fiberJob->m_WorkerFilter = worker;
	fiberJob->StartCounter();
	PushJob(fiberJob, lock);
	return fiberJob;
}

FiberJobPtr FiberScheduler::_PostJob(std::shared_ptr<Job> job, JobSignalPtr signal, uint64 worker, bool lock)
{
	FiberJobPtr fiberJob = std::allocate_shared<FiberJob>(m_JobAllocator, FiberJob(this, job));
	fiberJob->m_WorkerFilter = worker;
	fiberJob->StartCounter();
	std::lock_guard<std::mutex> signalLock(signal->m_Mutex);
	if (signal->IsValid())
		signal->PushJob(fiberJob, false);
	else
		PushJob(fiberJob, lock);
	return fiberJob;
}

void FiberScheduler::_PushJobPending(int32 count, uint64 workerFilter)
{
	while (--count >= 0)
	{
		FiberJobPtr job = PopJob(workerFilter);
		if (job.get())
		{
			m_PendingJobs[ThreadWorker::GetCurrentThreadID()].push_back(job);
		}
	}
}

//------------------------------------------------------------------------------