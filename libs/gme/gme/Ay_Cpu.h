


#ifndef AY_CPU_H
#define AY_CPU_H

#include "blargg_endian.h"

typedef blargg_long cpu_time_t;


void ay_cpu_out( class Ay_Cpu*, cpu_time_t, unsigned addr, int data );
int ay_cpu_in( class Ay_Cpu*, unsigned addr );

class Ay_Cpu {
public:

	void reset( void* mem_64k );
	


	bool run( cpu_time_t end_time );
	

	cpu_time_t time() const             { return state->time + state->base; }
	

	void set_time( cpu_time_t t )       { state->time = t - state->base; }
	void adjust_time( int delta )       { state->time += delta; }
	
	typedef BOOST::uint8_t uint8_t;
	typedef BOOST::uint16_t uint16_t;
	
	#if BLARGG_BIG_ENDIAN
		struct regs_t { uint8_t b, c, d, e, h, l, flags, a; };
	#else
		struct regs_t { uint8_t c, b, e, d, l, h, a, flags; };
	#endif
	BOOST_STATIC_ASSERT( sizeof (regs_t) == 8 );
	
	struct pairs_t { uint16_t bc, de, hl, fa; };
	

	struct registers_t {
		uint16_t pc;
		uint16_t sp;
		uint16_t ix;
		uint16_t iy;
		union {
			regs_t b;  
			pairs_t w;  
		};
		union {
			regs_t b;
			pairs_t w;
		} alt;
		uint8_t iff1;
		uint8_t iff2;
		uint8_t r;
		uint8_t i;
		uint8_t im;
	};

	

	enum { cpu_padding = 0x100 };
	
public:
	Ay_Cpu();
private:
	uint8_t szpc [0x200];
	uint8_t* mem;
	cpu_time_t end_time_;
	struct state_t {
		cpu_time_t base;
		cpu_time_t time;
	};
	state_t* state;  
	state_t state_;
	void set_end_time( cpu_time_t t );
public:
	registers_t r;
};

inline void Ay_Cpu::set_end_time( cpu_time_t t )
{
	cpu_time_t delta = state->base - t;
	state->base = t;
	state->time += delta;
}

#endif
