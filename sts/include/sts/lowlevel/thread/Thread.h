#pragma once

#include <sts\private_headers\thread\ThreadPlatform.h>
#include <commonlib\Macros.h>

NAMESPACE_STS_BEGIN

typedef PlatformAPI::THREAD_ID THREAD_ID;

/////////////////////////////////////////////////////
// Helper functions:
namespace this_thread
{
	// Yields thread that called this function
	void YieldThread();

	// Returns thread id of thread that called this function.
	THREAD_ID GetThreadID();

	// Sleeps on calling thread by specified amount of time in miliseconds.
	void SleepFor( unsigned miliseconds );
}

////////////////////////////////////////////////////////
// Base class for all thread in the system. To create new thread, one has to
// derive from this class and implement ThreadFunction().
class ThreadBase : private PlatformAPI::ThreadImpl
{
	BASE_CLASS( PlatformAPI::ThreadImpl );

public:
	// To start thread instantly pass 'true'.
	ThreadBase( bool start_thread = false );
	ThreadBase( ThreadBase&& other );

	ThreadBase( const ThreadBase& ) = delete;
	ThreadBase& operator=( const ThreadBase& ) = delete;

	// Joins thread.
	void Join();

	// Detaches thread.
	void Detach();

	// Set debug thread name.
	void SetThreadName( const char* thread_name );

	// Starts the thread.
	void StartThread();

	// Returns thread id.
	THREAD_ID GetThreadID() const;

	// Function that will be called by thread.
	virtual void ThreadFunction() = 0;
};

///////////////////////////////////////////////////////////
//
// INLINES:
//
///////////////////////////////////////////////////////////

inline ThreadBase::ThreadBase( bool start_thread )
{
	if( start_thread )
	    StartThread();
}

///////////////////////////////////////////////////////////
inline ThreadBase::ThreadBase( ThreadBase&& other )
	: __base( std::forward< __base >( other ) ) 
{
}


///////////////////////////////////////////////////////////
inline void ThreadBase::StartThread()
{
	__base::StartThread( this );
}

///////////////////////////////////////////////////////////
inline void ThreadBase::Join()
{
	__base::Join();
}

///////////////////////////////////////////////////////////
inline void ThreadBase::Detach()
{
	__base::Detach();
}

///////////////////////////////////////////////////////////
inline void ThreadBase::SetThreadName( const char* thread_name )
{
	__base::SetThreadName( thread_name );
}

///////////////////////////////////////////////////////////
inline THREAD_ID ThreadBase::GetThreadID() const
{
	return __base::GetThreadID();
}

namespace this_thread
{

///////////////////////////////////////////////////////////
inline void YieldThread()
{ 
	PlatformAPI::YieldThread(); 
}

///////////////////////////////////////////////////////////
inline THREAD_ID GetThreadID()
{
	return PlatformAPI::GetThreadID();
}

///////////////////////////////////////////////////////////
inline void SleepFor( unsigned miliseconds )
{
	PlatformAPI::SleepFor( miliseconds );
}

}

NAMESPACE_STS_END