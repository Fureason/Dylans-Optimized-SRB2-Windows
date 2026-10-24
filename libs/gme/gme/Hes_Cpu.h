


#ifndef HES_CPU_H
#define HES_CPU_H

#include "blargg_common.h"

typedef blargg_long hes_time_t;  
typedef unsigned hes_addr_t;  
enum { future_hes_time = INT_MAX / 2 + 1 };

class Hes_Cpu {
public:
	typedef BOOST::uint8_t uint8_t;
	
	void reset();
	
	enum { page_size = 0x2000 };
	enum { page_shift = 13 };
	enum { page_count = 8 };
	void set_mmr( int reg, int bank );
	
	uint8_t const* get_code( hes_addr_t );
	
	uint8_t ram [page_size];
	

	struct registers_t {
		BOOST::uint16_t pc;
		uint8_t a;
		uint8_t x;
		uint8_t y;
		uint8_t status;
		uint8_t sp;
	};
	registers_t r;
	

	uint8_t mmr [page_count + 1];
	


	bool run( hes_time_t end_time );
	

	hes_time_t time() const             { return state->time + state->base; }
	void set_time( hes_time_t t )       { state->time = t - state->base; }
	void adjust_time( int delta )       { state->time += delta; }
	
	hes_time_t irq_time() const         { return irq_time_; }
	void set_irq_time( hes_time_t );
	
	hes_time_t end_time() const         { return end_time_; }
	void set_end_time( hes_time_t );
	
	void end_frame( hes_time_t );
	



	enum { idle_addr = 0x1FFF };
	

	enum { cpu_padding = 8 };
	
public:
	Hes_Cpu() { state = &state_; }
	enum { irq_inhibit = 0x04 };
private:

	Hes_Cpu( const Hes_Cpu& );
	Hes_Cpu& operator = ( const Hes_Cpu& );
	
	struct state_t {
		uint8_t const* code_map [page_count + 1];
		hes_time_t base;
		blargg_long time;
	};
	state_t* state;  
	state_t state_;
	hes_time_t irq_time_;
	hes_time_t end_time_;
	
	void set_code_page( int, void const* );
	inline int update_end_time( hes_time_t end, hes_time_t irq );
};

inline BOOST::uint8_t const* Hes_Cpu::get_code( hes_addr_t addr )
{
	return state->code_map [addr >> page_shift] + addr
	#if !BLARGG_NONPORTABLE
		% (unsigned) page_size
	#endif
	;
}

inline int Hes_Cpu::update_end_time( hes_time_t t, hes_time_t irq )
{
	if ( irq < t && !(r.status & irq_inhibit) ) t = irq;
	int delta = state->base - t;
	state->base = t;
	return delta;
}

inline void Hes_Cpu::set_irq_time( hes_time_t t )
{
	state->time += update_end_time( end_time_, (irq_time_ = t) );
}

inline void Hes_Cpu::set_end_time( hes_time_t t )
{
	state->time += update_end_time( (end_time_ = t), irq_time_ );
}

inline void Hes_Cpu::end_frame( hes_time_t t )
{
	assert( state == &state_ );
	state_.base -= t;
	if ( irq_time_ < future_hes_time ) irq_time_ -= t;
	if ( end_time_ < future_hes_time ) end_time_ -= t;
}

#endif
