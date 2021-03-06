#include <sts\private_headers\winAPI\ThreadImplWinAPI.h>
#include <commonlib\Macros.h>
#include <sts\lowlevel\thread\Thread.h>

NAMESPACE_STS_BEGIN
NAMESPACE_WINAPI_BEGIN

//////////////////////////////////////////////////////
void YieldThread()
{
	::SwitchToThread(); // SwitchToThread performs context switch only if there is waiting thread on current processors.
}

//////////////////////////////////////////////////////
THREAD_ID GetThreadID()
{
	return ::GetCurrentThreadId();
}

//////////////////////////////////////////////////////
void SleepFor( unsigned miliseconds )
{
	::Sleep( miliseconds );
}

//////////////////////////////////////////////////////
// static function used to run by win thread
DWORD WINAPI ThreadFunction( LPVOID lpParam ) 
{
	ThreadBase* thread = static_cast<ThreadBase*> ( lpParam );
	thread->ThreadFunction();
	return 0;
}

//////////////////////////////////////////////////////
ThreadImpl::ThreadImpl()
	: m_threadHandle( NULL )
	, m_id( 0 )
{
}

///////////////////////////////////////////////////
ThreadImpl::ThreadImpl( ThreadImpl&& other )
	: m_threadHandle( other.m_threadHandle )
{
	other.m_threadHandle = NULL;
}

///////////////////////////////////////////////////
ThreadImpl::~ThreadImpl()
{
	::CloseHandle( m_threadHandle );
}
///////////////////////////////////////////////////
void ThreadImpl::StartThread( ThreadBase* thread )
{
	m_threadHandle = ::CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		ThreadFunction,         // thread function name
		thread,				    // argument to thread function 
		0,                      // use default creation flags 
		&m_id );				// returns the thread identifier 

	ASSERT( m_threadHandle != NULL );
}

///////////////////////////////////////////////////
void ThreadImpl::Join()
{
	DWORD ret = ::WaitForSingleObject( m_threadHandle, INFINITE );
	ASSERT( ret == WAIT_OBJECT_0 );
}

///////////////////////////////////////////////////
void ThreadImpl::Detach()
{
	::CloseHandle( m_threadHandle );
	m_threadHandle = NULL;
}

///////////////////////////////////////////////////
THREAD_ID ThreadImpl::GetThreadID() const
{
	return m_id;
}

///////////////////////////////////////////////////
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

void ThreadImpl::SetThreadName( const char* thread_name )
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = thread_name;
	info.dwThreadID = ::GetThreadId( m_threadHandle );
	info.dwFlags = 0;

	__try
	{
		RaiseException( 0x406D1388, 0, sizeof( info ) / sizeof( DWORD ), (ULONG_PTR*)&info );
	}
	__except( EXCEPTION_CONTINUE_EXECUTION ) {}
}

NAMESPACE_WINAPI_END
NAMESPACE_STS_END