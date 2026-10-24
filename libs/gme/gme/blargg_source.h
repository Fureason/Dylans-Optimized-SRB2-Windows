                                                                                     
                                                                           
                                                                            
                            

#ifndef BLARGG_SOURCE_H
#define BLARGG_SOURCE_H




#include <assert.h>





#undef require
#define require( expr ) assert( expr )




#undef likely
#undef unlikely
#ifdef __GNUC__
    #define likely( x ) __builtin_expect(x, 1)
    #define unlikely( x ) __builtin_expect(x, 0)
#else
    #define likely( x ) (x)
    #define unlikely( x ) (x)
#endif




static inline void blargg_dprintf_( const char*, ... ) { }
#undef debug_printf
#define debug_printf (1) ? (void) 0 : blargg_dprintf_




#undef check
#define check( expr ) ((void) 0)


#undef RETURN_ERR
#define RETURN_ERR( expr ) do {                         \
		blargg_err_t blargg_return_err_ = (expr);               \
		if ( blargg_return_err_ ) return blargg_return_err_;    \
	} while ( 0 )


#undef CHECK_ALLOC
#define CHECK_ALLOC( ptr ) do { if ( (ptr) == 0 ) return "Out of memory"; } while ( 0 )


#undef min
#undef max

#define DEF_MIN_MAX( type ) \
	static inline type min( type x, type y ) { if ( x < y ) return x; return y; }\
	static inline type max( type x, type y ) { if ( y < x ) return x; return y; }

DEF_MIN_MAX( int )
DEF_MIN_MAX( unsigned )
DEF_MIN_MAX( long )
DEF_MIN_MAX( unsigned long )
DEF_MIN_MAX( float )
DEF_MIN_MAX( double )

#undef DEF_MIN_MAX

  
                                                                                   
                                                      

               
                               
                 
                        
 
             
           
          
 

                 
                        
 
             
           
          
 
  


#undef byte
#define byte byte_
typedef unsigned char byte;


#ifndef BLARGG_EXPORT
    #if defined (_WIN32) && defined(BLARGG_BUILD_DLL)
        #define BLARGG_EXPORT __declspec(dllexport)
    #elif defined (LIBGME_VISIBILITY)
        #define BLARGG_EXPORT __attribute__((visibility ("default")))
    #else
        #define BLARGG_EXPORT
    #endif
#endif


#define BLARGG_CHECK_ALLOC CHECK_ALLOC
#define BLARGG_RETURN_ERR RETURN_ERR


#ifdef BLARGG_SOURCE_BEGIN
	#include BLARGG_SOURCE_BEGIN
#endif

#endif
