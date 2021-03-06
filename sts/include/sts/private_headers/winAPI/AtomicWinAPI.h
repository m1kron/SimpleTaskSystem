#pragma once

#include <windows.h>
#include <sts\private_headers\common\NamespaceMacros.h>
#include <commonlib\tools\Tools.h>
#include <sts\lowlevel\atomic\MemoryOrder.h>

NAMESPACE_STS_BEGIN
NAMESPACE_WINAPI_BEGIN

//////////////////////////////////////////////////
// Memory barrier
inline void FullMemoryBarrier()
{
	MemoryBarrier();
}

//////////////////////////////////////////////////
//
// IMPLEMENTATION FOR 32 bit INTEGRAL TYPES:
//
//////////////////////////////////////////////////

class Atomic32Impl
{
public:
	typedef LONG TAtomicType;

	Atomic32Impl();

	LONG Load( MemoryOrder order = MemoryOrder::SeqCst ) const;
	void Store( LONG value, MemoryOrder order = MemoryOrder::SeqCst );
	bool CompareExchange( LONG& expected_val, LONG value_to_set, MemoryOrder order = MemoryOrder::SeqCst );
	LONG FetchAdd( LONG value ); 
	LONG FetchSub( LONG value );
	LONG Increment();
	LONG Decrement();
	LONG FetchAnd( LONG value );
	LONG FetchOr( LONG value );

private:
	STS_ALIGNED( 4 ) volatile LONG m_value;
};

//////////////////////////////////////////////////
//
// INLINES:
//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
inline Atomic32Impl::Atomic32Impl() 
	: m_value() 
{
	ASSERT( IsAligned< 4 >( this ) );
}

inline LONG Atomic32Impl::Load( MemoryOrder order ) const
{
	LONG val = m_value;

	if( order == MemoryOrder::SeqCst )
		FullMemoryBarrier();

	return val;
}

//////////////////////////////////////////////////
inline void Atomic32Impl::Store( LONG value, MemoryOrder order )
{
	if ( order != MemoryOrder::SeqCst ) 
		m_value = value;
	else
		InterlockedExchange( &m_value, value );
}

//////////////////////////////////////////////////
inline bool Atomic32Impl::CompareExchange( LONG& expected_val, LONG value_to_set, MemoryOrder order )
{
	LONG prev_val = expected_val;
	expected_val = InterlockedCompareExchange( &m_value, value_to_set, expected_val );

	return prev_val == expected_val;
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::FetchAdd( LONG value )
{
	return InterlockedAdd( &m_value, value );
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::FetchSub( LONG value )
{
	return InterlockedAdd( &m_value, -value );
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::Increment()
{
	return InterlockedIncrement( &m_value );
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::Decrement()
{
	return InterlockedDecrement( &m_value );
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::FetchAnd( LONG value )
{
	return InterlockedAnd( &m_value, value );
}

//////////////////////////////////////////////////
inline LONG Atomic32Impl::FetchOr( LONG value )
{
	return InterlockedOr( &m_value, value );
}

NAMESPACE_WINAPI_END
NAMESPACE_STS_END