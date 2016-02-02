#include<sts\tasking\TaskWorker.h>
#include<sts\tasking\Task.h>
#include<sts\private_headers\common\NamespaceMacros.h>
#include<sts\tasking\TaskWorkersPool.h>

NAMESPACE_STS_BEGIN


TaskWorkerThread::TaskWorkerThread( TaskManager* task_manager, TaskWorkersPool* pool, unsigned pool_index )
    : m_workersPool( pool )
	, m_taskManager( task_manager )
	, m_poolIndex( pool_index )
    , m_shouldFinishWork( false )
	, m_hasFinishWork( false )
{
}

///////////////////////////////////////////////////////////
void TaskWorkerThread::ThreadFunction()
{
	while( true )
	{
		// Check if we have any new task to work on. If not, then wait for them.
		m_hasWorkToDoEvent.Wait();
		m_hasWorkToDoEvent.ResetEvent();

		// Finish work if requested:
		if( m_shouldFinishWork )
		{
			m_hasFinishWork = true;
			return;
		}

		// Do all the tasks:
		while( !m_shouldFinishWork )
		{
			Task* task = nullptr;

			// Check if there is any task in the queue.
			task = m_pendingTaskQueue.Pop();

			// Local queue is empty, so try to steal task from other threads.
			if( !task )
				task = StealTaskFromOtherWorkers();

			if( task )
			{
				// We have task, so run it now.
				task->Run( m_taskManager );
			}
			else
				break; // We don't have anything to do, so break and wait for job.
		}
	}
}

////////////////////////////////////////////////////////
Task* TaskWorkerThread::StealTaskFromOtherWorkers()
{
	Task* stealed_task = nullptr;

	unsigned workers_count = m_workersPool->GetPoolSize();
	for( unsigned i = 1; i < workers_count; ++i )
	{
		// Start from thread that is next to this worker in the pool.
		unsigned index = ( i + m_poolIndex ) % workers_count;
		if( stealed_task = m_workersPool->GetWorkerAt( index )->TryToStealTask() )
			return stealed_task;
	}

	return nullptr;
}


NAMESPACE_STS_END