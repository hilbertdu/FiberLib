// main.cpp
//------------------------------------------------------------------------------
#include "Fiber/FiberScheduler.h"
#include "Semaphore.h"
#include <assert.h>


static int32              counter(0);
static std::atomic<int32> threadsafe_counter(0);


void TaskAddCounter(int32 loopcount)
{
	while (--loopcount >= 0)
		counter++;
}

void TaskDelCounter(int32 loopcount)
{
	while (--loopcount >= 0)
		counter--;
}

void TaskAddCounterTS(int32 loopcount)
{
	while (--loopcount >= 0)
		threadsafe_counter++;
}

void TaskDelCounterTS(int32 loopcount)
{
	while (--loopcount >= 0)
		threadsafe_counter--;
}

void TestCase1(FiberScheduler* sche)
{
	// Test simple post job
	auto signal = sche->FetchSignal();
	auto job1 = sche->PostJob([]() { TaskAddCounterTS(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
	auto job2 = sche->PostJob([]() { TaskAddCounterTS(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
	sche->AddPreCondition(signal, job1->GetSignal());
	sche->AddPreCondition(signal, job2->GetSignal());
	sche->YieldFor(signal);
	ASSERT(threadsafe_counter == 2000);
}

void TestCase2(FiberScheduler* sche)
{
	{
		// Test successor
		auto job1 = sche->PostJob([]() { TaskAddCounter(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
		auto job2 = sche->PostJob([]() { TaskDelCounter(1000); }, job1->GetSignal(), ThreadWorkerFilter::E_WORKER_ON_ANY);
		sche->YieldFor(job2->GetSignal());
		ASSERT(counter == 0);
	}

	{
		// Test successor
		auto job1 = sche->PostJob([]() { TaskAddCounter(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
		auto job2 = job1->PostSuccessor([]() { TaskDelCounter(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
		sche->YieldFor(job2->GetSignal());
		ASSERT(counter == 0);
	}

	{
		// Test completor
		auto job1 = sche->PostJob([]() { TaskAddCounter(1000); }, ThreadWorkerFilter::E_WORKER_ON_ANY);
		auto job2 = job1->PostCompletor([](int32 status) { TaskDelCounter(1000); });
		auto job3 = job1->PostCompletor([](int32 status) { TaskAddCounter(1000); });
		auto job4 = job1->PostCompletor([](int32 status) { TaskAddCounter(1000); });
		sche->YieldFor(job2->GetSignal());
		ASSERT(counter == 2000);
	}
}

void TestCase3(FiberScheduler* sche)
{
	// a more complex task graph
	threadsafe_counter = 0;
	counter = 0;
	auto Add = [](int32 count) { TaskAddCounter(count); };
	auto Del = [](int32 count) { TaskDelCounter(count); };
	auto AddTS = [](int32 count) { TaskAddCounterTS(count); };
	auto DelTS = [](int32 count) { TaskDelCounterTS(count); };
	auto Eval = []() { counter += threadsafe_counter; };

	//**********************************************************************************************************
	//**********************************************************************************************************
	//          |--->X2------>|
	//     X1-->|             |
	//          |--->X3------>|                                    |-->W1-->|
	//                        |--->Z1(depend on X2,X3,Y2,Y3,Y4)--->|-->W2-->|--->F(depend on W1,W2,W3)
	//          |--->Y2------>|                                    |-->W3-->|
	//     Y1-->|--->Y3------>|
	//          |--->Y4------>|
	//**********************************************************************************************************
	//**********************************************************************************************************

	// safe counter to 20000
	// counter to 10000
	auto jobX1 = sche->PostJob([&]() { Add(10000); }, ThreadWorkerFilter::E_WORKER_ON_MAIN);
	auto jobX2 = jobX1->PostSuccessor([&]() { AddTS(10000); });
	auto jobX3 = jobX1->PostSuccessor([&]() { AddTS(10000); });

	// safe counter to 10000
	// counter to 20000
	auto jobY1 = sche->PostJob([&]() { Add(10000); }, ThreadWorkerFilter::E_WORKER_ON_MAIN);
	auto jobY2 = jobY1->PostSuccessor([&]() { DelTS(2000); });
	auto jobY3 = jobY1->PostSuccessor([&]() { DelTS(3000); });
	auto jobY4 = jobY1->PostSuccessor([&]() { DelTS(5000); });

	auto signal = sche->FetchSignal();
	sche->AddPreCondition(signal, jobX2->GetSignal());
	sche->AddPreCondition(signal, jobX3->GetSignal());
	sche->AddPreCondition(signal, jobY2->GetSignal());
	sche->AddPreCondition(signal, jobY3->GetSignal());
	sche->AddPreCondition(signal, jobY4->GetSignal());

	// safe counter to 50000
	// counter to 50000
	auto jobZ1 = sche->PostJob([&]() { Eval(); }, signal);
	auto jobW1 = jobZ1->PostSuccessor([&]() { AddTS(20000); });
	auto jobW2 = jobZ1->PostSuccessor([&]() { AddTS(20000); });
	auto jobW3 = jobZ1->PostSuccessor([&]() { Add(20000); });

	auto signal2 = sche->FetchSignal();
	sche->AddPreCondition(signal2, jobW1->GetSignal());
	sche->AddPreCondition(signal2, jobW2->GetSignal());
	sche->AddPreCondition(signal2, jobW3->GetSignal());

	// counter to 50000 + 50000
	auto jobF = sche->PostJob([&]() { Eval(); }, signal2);

	sche->YieldFor(jobF->GetSignal());

	ASSERT(threadsafe_counter == 50000);
	ASSERT(counter == 100000);
}

int main(int, char* [])
{
	auto scheduler = new FiberScheduler;
	scheduler->InitWorker(2);

	Semaphore semaphore;
	scheduler->PostJob([&]() {
		TestCase1(scheduler);
		TestCase2(scheduler);
		TestCase3(scheduler);
		semaphore.Notify();
	});
	semaphore.Wait();

	return 0;
}