// Job.h
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Types.h"
#include "Misc.h"
#include <functional>
#include <atomic>
#include <assert.h>


// Class Job
//------------------------------------------------------------------------------
class Job
{
public:
	enum class Priority : uint8
	{
		PRIO_TOP = 0,
		PRIO_RENDER,
		PRIO_COMPUTE,
		PRIO_IO,
		PRIO_EDITOR,
		PRIO_LOW,
		PRIO_MAX
	};

	enum class Status : uint8
	{
		STATUS_INVALID = 0,
		STATUS_CREATED,
		STATUS_READY,
		STATUS_RUNNING,
		STATUS_SUSPEND,
		STATUS_SUCCESS,
		STATUS_FAILED,
		STATUS_EXPIRED		
	};

	explicit Job();
	virtual ~Job();

	Job(const Job& job) = delete;
	
	FORCE_INLINE Status	GetStatus() const { return m_Status; }
	FORCE_INLINE bool   IsFinished() const { return m_Status == Status::STATUS_SUCCESS || m_Status == Status::STATUS_FAILED; }
	FORCE_INLINE bool   IsTimeout() const { return m_Status == Status::STATUS_EXPIRED; }
	FORCE_INLINE bool   IsRunning() const { return m_Status == Status::STATUS_RUNNING || m_Status == Status::STATUS_SUSPEND || m_Status == Status::STATUS_READY; }
	FORCE_INLINE bool   IsValid() const { return m_Status != Status::STATUS_INVALID; }
	FORCE_INLINE bool   IsReady() const { return m_Status == Status::STATUS_READY; }
	FORCE_INLINE void   Reset() { m_Status = Status::STATUS_INVALID; }

	FORCE_INLINE void   SetStatus(Status statu) { m_Status = statu; }
	FORCE_INLINE uint8  GetPriority() const { return (uint8)m_Prio; }
	FORCE_INLINE void   Abort() { m_Aborted = true; OnAborted(); }

	virtual int32 Excute() = 0;	

	virtual void OnAborted() {}
	virtual void OnFinished(Status status) {}
	virtual void SetPreResult(int32 preResult) {}

protected:
	Status                 m_Status;
	Priority               m_Prio;
	std::atomic<bool>      m_Aborted;
};


// Class FuncJob
//------------------------------------------------------------------------------
class FuncJob : public Job
{
public:
	template<typename Functor>
	FuncJob(Functor&& f)
	{
		PRAGMA_DISABLE_PUSH_MSVC(4996);
		if constexpr (std::is_same<typename std::result_of<Functor()>::type, void>::value)
			m_NoneResultExecutor = std::forward<Functor>(f);
		else
			m_Executor = std::forward<Functor>(f);
		PRAGMA_DISABLE_POP_MSVC(4996);
	}

	virtual int32 Excute() 
	{
		if (m_Executor)
		{
			return m_Executor();
		}
		else
		{
			m_NoneResultExecutor(); 
			return 0;
		}
	}	

private:
	std::function<int32()> m_Executor;
	std::function<void()>  m_NoneResultExecutor;
};

// Class FuncCompletorJob
//------------------------------------------------------------------------------
class FuncCompletorJob : public Job
{
public:
	template<typename Functor>
	FuncCompletorJob(Functor&& d1) : m_Completor(std::forward<Functor>(d1)), m_PreResult(0) {}

	virtual int32 Excute() { m_Completor(m_PreResult); return 0; }
	virtual void  SetPreResult(bool preResult) { m_PreResult = preResult; }

private:
	bool                       m_PreResult;
	std::function<void(int32)> m_Completor;
};


//------------------------------------------------------------------------------
