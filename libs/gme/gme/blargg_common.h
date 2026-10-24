


#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#undef BLARGG_COMMON_H

#include "blargg_config.h"
#ifndef BLARGG_COMMON_H
#define BLARGG_COMMON_H


#if __GNUC__ >= 3 || _MSC_VER >= 1100
	#define BLARGG_RESTRICT __restrict
#else
	#define BLARGG_RESTRICT
#endif


#ifndef STATIC_CAST
	#define STATIC_CAST(T,expr) ((T) (expr))
#endif


#ifndef blargg_err_t
	typedef const char* blargg_err_t;
#endif


template<class T>
class blargg_vector {
	T* begin_;
	size_t size_;
public:
	blargg_vector() : begin_( 0 ), size_( 0 ) { }
	~blargg_vector() { free( begin_ ); }
	size_t size() const { return size_; }
	T* begin() const { return begin_; }
	T* end() const { return begin_ + size_; }
	blargg_err_t resize( size_t n )
	{
		void* p = realloc( begin_, n * sizeof (T) );
		if ( !p && n )
			return "Out of memory";
		begin_ = (T*) p;
		size_ = n;
		return 0;
	}
	void clear() { void* p = begin_; begin_ = 0; size_ = 0; free( p ); }
	T& operator [] ( size_t n ) const
	{
		assert( n <= size_ );  
		return begin_ [n];
	}
};

#ifndef BLARGG_DISABLE_NOTHROW

	#if __cplusplus >= 199711 || __GNUC__ >= 3
		#define BLARGG_THROWS( spec ) throw spec
	#else
		#define BLARGG_THROWS( spec )
	#endif
	#define BLARGG_DISABLE_NOTHROW \
		void* operator new ( size_t s ) BLARGG_THROWS(()) { return malloc( s ); }\
		void operator delete ( void* p ) { free( p ); }
	#define BLARGG_NEW new
#else
	#include <new>
	#define BLARGG_NEW new (std::nothrow)
#endif


#define BLARGG_4CHAR( a, b, c, d ) \
	((a&0xFF)*0x1000000L + (b&0xFF)*0x10000L + (c&0xFF)*0x100L + (d&0xFF))


#ifndef BOOST_STATIC_ASSERT
	#ifdef _MSC_VER

		#define BOOST_STATIC_ASSERT( expr ) \
			void blargg_failed_( int (*arg) [2 / (int) !!(expr) - 1] )
	#else


		#define BOOST_STATIC_ASSERT( expr ) \
			void blargg_failed_( int (*arg) [2 / !!(expr) - 1] [__LINE__] )
	#endif
#endif



#ifndef BLARGG_COMPILER_HAS_BOOL
	#if defined (__MWERKS__)
		#if !__option(bool)
			#define BLARGG_COMPILER_HAS_BOOL 0
		#endif
	#elif defined (_MSC_VER)
		#if _MSC_VER < 1100
			#define BLARGG_COMPILER_HAS_BOOL 0
		#endif
	#elif defined (__GNUC__)

	#elif __cplusplus < 199711
		#define BLARGG_COMPILER_HAS_BOOL 0
	#endif
#endif
#if defined (BLARGG_COMPILER_HAS_BOOL) && !BLARGG_COMPILER_HAS_BOOL

	typedef int bool;
	const bool true  = 1;
	const bool false = 0;
#endif



#if INT_MAX < 0x7FFFFFFF || LONG_MAX == 0x7FFFFFFF
	typedef long blargg_long;
#else
	typedef int blargg_long;
#endif

#if UINT_MAX < 0xFFFFFFFF || ULONG_MAX == 0xFFFFFFFF
	typedef unsigned long blargg_ulong;
#else
	typedef unsigned blargg_ulong;
#endif




#if defined (HAVE_STDINT_H)
	#include <stdint.h>
	#define BOOST


#elif defined (HAVE_INTTYPES_H)
	#include <inttypes.h>
	#define BOOST

#else
	struct BOOST
	{
		#if UCHAR_MAX == 0xFF && SCHAR_MAX == 0x7F
			typedef signed char     int8_t;
			typedef unsigned char   uint8_t;
		#else

			typedef struct see_blargg_common_h int8_t;
			typedef struct see_blargg_common_h uint8_t;
		#endif
		
		#if USHRT_MAX == 0xFFFF
			typedef short           int16_t;
			typedef unsigned short  uint16_t;
		#else

			typedef struct see_blargg_common_h int16_t;
			typedef struct see_blargg_common_h uint16_t;
		#endif
		
		#if ULONG_MAX == 0xFFFFFFFF
			typedef long            int32_t;
			typedef unsigned long   uint32_t;
		#elif UINT_MAX == 0xFFFFFFFF
			typedef int             int32_t;
			typedef unsigned int    uint32_t;
		#else

			typedef struct see_blargg_common_h int32_t;
			typedef struct see_blargg_common_h uint32_t;
		#endif
	};
#endif

#if __GNUC__ >= 3
	#define BLARGG_DEPRECATED __attribute__ ((deprecated))
#else
	#define BLARGG_DEPRECATED
#endif




#ifndef BLARGG_PURE
	#define BLARGG_PURE( def ) def
#endif

#endif
#endif
