#pragma once
#include <sts\private_headers\common\NamespaceMacros.h>
#include <sts\lowlevel\atomic\Atomic.h>
#include <commonlib\compile_time_tools\IsPowerOf2.h>

NAMESPACE_STS_BEGIN

///////////////////////////////////////////////////
// Implementation of lockfree multiple-producer
// multiple-consumer circular FIFO queue.
//  
//  . - empty slot in queue
//  | - ready to read slot ( taken and has data )
//  : - slot is takan, but producer is still writing data to that slot.
//
//	.......||||||||||||||||||||||||:::::::::::::::::......
//		   ^					  ^				   ^
//      read counter     commited counter       write counter
//
template < class T, unsigned SIZE >
class LockFreePtrQueue
{
public:
	// Push item to queue. Increases size by 1.
	// Returns true if success.
	bool Push( T* const item );

	// Take first element from queue. Decreases size by 1 and returns obtained item.
	// Returns nullptr in case of failture.
	T* Pop();

	// Returns size of the queue. Not thread safe.
    unsigned Size_NotThreadSafe() const;

private:
	// Helper function to calculate modulo SIZE of the queue from counter.
	unsigned CounterToIndex( unsigned counter ) const;

    Atomic< unsigned > m_writeCounter;			
	Atomic< unsigned > m_committedWriteCounter;	
	Atomic< unsigned > m_readCounter;
	T* m_queue[ SIZE ];
};

//////////////////////////////////////////////////////////////
//
// INLINES:
//
//////////////////////////////////////////////////////////////
template < class T, unsigned SIZE >
inline bool LockFreePtrQueue<T, SIZE>::Push( T* const item )
{
	// First, check if the queue is full:
	unsigned write_counter = 0;
	do
	{
		write_counter = m_writeCounter.Load( MemoryOrder::Relaxed );
		unsigned read_counter = m_readCounter.Load( MemoryOrder::Acquire ); 

		if( ( read_counter + SIZE + 1 ) == ( write_counter + 1 ) )
		{
			return false; // Queue is full
		}

	// Try to reserve slot, if m_WriteCounter != write_counter, it means that other 
	// thread was faster than our and we have to retry whole operation.
	} while( !m_writeCounter.CompareExchange( write_counter, write_counter + 1, MemoryOrder::Acquire ) );

	// Add stuff to the queue
	m_queue[ CounterToIndex( write_counter ) ] = item;

	// Last thing: we have to commit the change, so every thread knows that we 
	// finished adding new item to the queue. Threads have to commit their data in the same
	// order as they were writing data to the queue - to gain that, every thread has to set committedWriteCounter
	// to be + 1 of write counter that given thread got.
	unsigned expected = write_counter;
	while( !m_committedWriteCounter.CompareExchange( expected, write_counter + 1, MemoryOrder::Release ) )
	{
		ASSERT( expected <= write_counter );///< Actually fatal assert..
		// Remember that in case of failture of CompareExchange, expected value will contain current
		// value of atomic, so we have to set to have wriet_counter once agian.
		expected = write_counter;
		
		// We have to wait for another thread to finish pushing new data.
		// This means that another thread first started to pushing data, but it took him more
		// time than us, so give him time to finish it's job by yielding.
		// [ NOTE ] Remember that in STS we should have max 2 concurent producer( worker thread and main thread )
		// since we are always adding new task to current worker.

		sts::this_thread::YieldThread();
	}

	return true;
}

//////////////////////////////////////////////////////////////
template < class T, unsigned SIZE >
inline T* LockFreePtrQueue<T, SIZE>::Pop()
{
	unsigned current_read_counter = 0;
	T* return_item = nullptr;

	do
	{
		// First check, whether there is anything to pop:
		current_read_counter = m_readCounter.Load( MemoryOrder::Relaxed );
		unsigned current_committed_counter = m_committedWriteCounter.Load( MemoryOrder::Acquire );

		if( current_read_counter == current_committed_counter )
		{
			// Queue is empty(or another threads are still committing their's changes).
			return nullptr;
		}

		// Grab the data.
		unsigned queue_index = CounterToIndex( current_read_counter );
		return_item = m_queue[ queue_index ];

		// [NOTE]: we can't nullptr queue item here, cuz we don't know yet, whether we succeed.
		// ( Other thread can be reading the data now and he will win on increasing read counter ).

	// Try to increase read counter. If failed, it means that another thread has already read it,
	// so we have to retry whole operation.
	} while( !m_readCounter.CompareExchange( current_read_counter, current_read_counter + 1, MemoryOrder::Release ) );

	// [NOTE]: we can't nullptr queue item here, cuz it would require another synchronization between threads
	// ( other thread can be pushing new data right now at the same place if the queue is almost full ).

	// Return the data.
	return return_item;
}	

//////////////////////////////////////////////////////////////
template < class T, unsigned SIZE >
inline unsigned LockFreePtrQueue<T, SIZE>::CounterToIndex( unsigned counter ) const
{
	STATIC_ASSERT( IsPowerOf2< SIZE >::value == 1, "SIZE of LockFreePtrQueue has to be power of 2!" );
	return counter & ( SIZE - 1 ); 
}

//////////////////////////////////////////////////////////////
template < class T, unsigned SIZE >
inline unsigned LockFreePtrQueue<T, SIZE>::Size_NotThreadSafe() const
{
	return ( m_writeCounter - m_readCounter ); 
}

NAMESPACE_STS_END