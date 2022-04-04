// Semaphore
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include <mutex>
#include <condition_variable>
#include "Misc.h"


class Semaphore {
public:
	Semaphore(int count_ = 0) : m_Count(count_) {}

	FORCE_INLINE void Notify() 
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Count++;
		m_CV.notify_one();
	}
	FORCE_INLINE void Wait() 
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		while (m_Count == 0)
		{
			m_CV.wait(lock);
		}
		m_Count--;
	}

private:
	std::mutex              m_Mutex;
	std::condition_variable m_CV;
	int                     m_Count;
};


//------------------------------------------------------------------------------
