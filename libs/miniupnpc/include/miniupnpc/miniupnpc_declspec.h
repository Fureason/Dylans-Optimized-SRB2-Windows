#ifndef MINIUPNPC_DECLSPEC_H_INCLUDED
#define MINIUPNPC_DECLSPEC_H_INCLUDED

#if defined(_WIN32) && !defined(MINIUPNP_STATICLIB)
	 
	#ifdef MINIUPNP_EXPORTS
		#define MINIUPNP_LIBSPEC __declspec(dllexport)
	#else
		#define MINIUPNP_LIBSPEC __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && __GNUC__ >= 4
		 
		#define MINIUPNP_LIBSPEC __attribute__ ((visibility ("default")))
	#else
		#define MINIUPNP_LIBSPEC
	#endif
#endif

#endif  

