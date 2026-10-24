


#ifndef SPC_FILTER_H
#define SPC_FILTER_H

#include "blargg_common.h"

struct SPC_Filter {
public:
	

	typedef short sample_t;
	void run( sample_t* io, int count );
	



	void clear();
	


	enum { gain_unit = 0x100 };
	void set_gain( int gain );
	

	void enable( bool b );
	

	enum { bass_none =  0 };
	enum { bass_norm =  8 };  
	enum { bass_max  = 31 };
	void set_bass( int bass );
	
public:
	SPC_Filter();
	BLARGG_DISABLE_NOTHROW
private:
	enum { gain_bits = 8 };
	int gain;
	int bass;
	bool enabled;
	struct chan_t { int p1, pp1, sum; };
	chan_t ch [2];
};

inline void SPC_Filter::enable( bool b )  { enabled = b; }

inline void SPC_Filter::set_gain( int g ) { gain = g; }

inline void SPC_Filter::set_bass( int b ) { bass = b; }

#endif
