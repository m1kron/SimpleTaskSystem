#pragma once

#include <sts\private_headers\common\NamespaceMacros.h>
#include <sts\lowlevel\synchro\ManualResetEvent.h>
#include <sts\tasking\TaskingCommon.h>
#include <sts\structures\LockfreePtrQueue.h>
#include <sts\lowlevel\thread\Thread.h>

NAMESPACE_STS_BEGIN

class Task;
class TaskWorkersPool;
class TaskManager;

class TaskWorkerThread : public ThreadBase
{
public:
	TaskWorkerThread( TaskManager* task_manager, TaskWorkersPool* pool, unsigned pool_index );

	TaskWorkerThread( TaskWorkerThread&& other ) = delete;
	TaskWorkerThread( const TaskWorkerThread& ) = delete;
	TaskWorkerThread& operator=( const TaskWorkerThread& ) = delete;

	// Adds task to lock free queue. Returns true if success.
	bool AddTask( Task* task );

	// Signals to stop work.
	void FinishWork();

	// Returns true if thread has finish it's thread function.
	bool HasFinishedWork() const;

	// Wake ups thread;
	void WakeUp();

	// Tries to steal task from worker queue. Returns nullptr if failed.
	Task* TryToStealTask();
private:
	// Main thread function.
	void ThreadFunction() override;

	// Loops through all other workers and tries to steal a task from them.
	Task* StealTaskFromOtherWorkers();

	LockFreePtrQueue< Task, TASK_POOL_SIZE / 2 > m_pendingTaskQueue;	
	ManualResetEvent m_hasWorkToDoEvent;
	TaskWorkersPool* m_workersPool;
	TaskManager* m_taskManager;
	unsigned m_poolIndex;
	bool m_shouldFinishWork;
	bool m_hasFinishWork;
};

////////////////////////////////////////////////////////////////
//
// INLINES:
//
////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
inline bool TaskWorkerThread::AddTask( Task* task )
{
	// Add new task to local queue.
	bool return_val = m_pendingTaskQueue.Push( task );

	return return_val;
}

////////////////////////////////////////////////////////
inline void TaskWorkerThread::FinishWork()
{
	m_shouldFinishWork = true;
	WakeUp();
}

////////////////////////////////////////////////////////
inline bool TaskWorkerThread::HasFinishedWork() const
{
	return m_hasFinishWork;
}

////////////////////////////////////////////////////////
inline void TaskWorkerThread::WakeUp()
{
	m_hasWorkToDoEvent.SetEvent();
}

////////////////////////////////////////////////////////
inline Task* TaskWorkerThread::TryToStealTask()
{
	return m_pendingTaskQueue.Pop();
}

NAMESPACE_STS_END