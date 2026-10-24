



#ifndef GB_CPU_H
#define GB_CPU_H

#include "blargg_common.h"
#include "blargg_endian.h"

typedef unsigned gb_addr_t;  

class Gb_Cpu {
	enum { clocks_per_instr = 4 };
public:
	typedef BOOST::uint8_t uint8_t;
	

	void reset( void* unmapped = 0 );
	


	enum { page_size = 0x2000 };
	void map_code( gb_addr_t start, unsigned size, void* code );
	
	uint8_t* get_code( gb_addr_t );
	

	void push_byte( int );
	

	struct core_regs_t {
	#if BLARGG_BIG_ENDIAN
		uint8_t b, c, d, e, h, l, flags, a;
	#else
		uint8_t c, b, e, d, l, h, a, flags;
	#endif
	};
	
	struct registers_t : core_regs_t {
		long pc;  
		BOOST::uint16_t sp;
	};
	registers_t r;
	


	

	gb_addr_t rst_base;
	

	enum { idle_addr = 0xF00D };
	


	bool run( blargg_long count );
	

	blargg_long remain() const { return state->remain * clocks_per_instr; }
	

	enum { cpu_padding = 8 };
	
public:
	Gb_Cpu() : rst_base( 0 ) { state = &state_; }
	enum { page_shift = 13 };
	enum { page_count = 0x10000 >> page_shift };
private:

	Gb_Cpu( const Gb_Cpu& );
	Gb_Cpu& operator = ( const Gb_Cpu& );
	
	struct state_t {
		uint8_t* code_map [page_count + 1];
		blargg_long remain;
	};
	state_t* state;  
	state_t state_;
	
	void set_code_page( int, uint8_t* );
};

inline BOOST::uint8_t* Gb_Cpu::get_code( gb_addr_t addr )
{
	return state->code_map [addr >> page_shift] + addr
	#if !BLARGG_NONPORTABLE
		% (unsigned) page_size
	#endif
	;
}

#endif
