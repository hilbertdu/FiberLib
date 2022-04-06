# FiberLib
This is a fiber based job system, it's simple designed and easy to use, you can write more efficient multi-thread programming just with lambada and job graph scheme using this job system.


## Feature
- No need poll, no need callback, no need any asynchronous design, very easy to organize your code.
- Support job graph scheme, you can define job prerequistes and successors.
- You can yield yourself in a job at anytime to achieve cooperative scheduling.
- Everything can be jobifiy, you can make a large number of fine-grained jobs, for example post job in a loop and join all of these jobs at end.
- Join a job does not block current thread, which is more efficient than other multi-thread framework.

## Example
```c++
void TestFiber(FiberScheduler* sche)
{
       // post job to any thread
       {
              sche->PostJob([]() { printf("Run Job 1\n"); });
              sche->PostJob([]() { printf("Run Job 2\n"); });
       }
       // post 100 job and wait for these jobs
       {
              auto signal = sche->FetchSignal();
              for (int idx = 0; idx < 100; ++idx)
              {
                     auto job = sche->PostJob([idx]() { printf("Run Job %d/100\n", idx); });
                     sche->AddPreCondition(signal, job->GetSignal());
              }
              sche->YieldFor(signal);
       }

       // post job to specify thread to avoid data race
       {
              auto job1 = sche->PostJob([]() { printf("Run Job on main\n"); }, 
ThreadWorkerFilter::E_WORKER_ON_MAIN);
              auto job2 = sche->PostJob([]() { printf("Run Job on main\n"); }, 
ThreadWorkerFilter::E_WORKER_ON_MAIN);
              auto job3 = sche->PostJob([]() { printf("Run Job on main\n"); }, 
ThreadWorkerFilter::E_WORKER_ON_MAIN);
              sche->YieldFor(job1->GetSignal());
              sche->YieldFor(job2->GetSignal());
              sche->YieldFor(job3->GetSignal());
       }
}

int main(int, char* [])
{
       auto scheduler = new FiberScheduler;
       scheduler->InitWorker(2);
       
       Semaphore semaphore;
       scheduler->PostJob([&]() {
              TestFiber(scheduler);
              semaphore.Notify();
       });
       semaphore.Wait();

       return 0;
}
```

## Some Reference
Parallelizing the Naughty Dog Engine Using Fibers in 2015 GDC Talk