


#ifndef NES_APU_H
#define NES_APU_H

#include "blargg_common.h"

typedef blargg_long nes_time_t;  
typedef unsigned nes_addr_t;  

#include "Nes_Oscs.h"

struct apu_state_t;
class Nes_Buffer;

class Nes_Apu {
public:

	void output( Blip_Buffer* );
	



	void dmc_reader( int (*callback)( void* user_data, nes_addr_t ), void* user_data = NULL );
	



	

	enum { start_addr = 0x4000 };
	enum { end_addr   = 0x4017 };
	void write_register( nes_time_t, nes_addr_t, int data );
	

	enum { status_addr = 0x4015 };
	int read_status( nes_time_t );
	



	void end_frame( nes_time_t );
	






	void reset( bool pal_mode = false, int initial_dmc_dac = 0 );
	

	void set_tempo( double );
	

	void save_state( apu_state_t* out ) const;
	void load_state( apu_state_t const& );
	

	void volume( double );
	

	void treble_eq( const blip_eq_t& );
	




	enum { osc_count = 5 };
	void osc_output( int index, Blip_Buffer* buffer );
	



	void irq_notifier( void (*callback)( void* user_data ), void* user_data = NULL );
	



	enum { no_irq = INT_MAX / 2 + 1 };
	enum { irq_waiting = 0 };
	nes_time_t earliest_irq( nes_time_t ) const;
	



	int count_dmc_reads( nes_time_t t, nes_time_t* last_read = NULL ) const;
	

	nes_time_t next_dmc_read_time() const;
	


	void run_until( nes_time_t );
	
public:
	Nes_Apu();
	BLARGG_DISABLE_NOTHROW
private:
	friend class Nes_Nonlinearizer;
	void enable_nonlinear( double volume );
	static double nonlinear_tnd_gain() { return 0.75; }
private:
	friend struct Nes_Dmc;
	

	Nes_Apu( const Nes_Apu& );
	Nes_Apu& operator = ( const Nes_Apu& );
	
	Nes_Osc*            oscs [osc_count];
	Nes_Square          square1;
	Nes_Square          square2;
	Nes_Noise           noise;
	Nes_Triangle        triangle;
	Nes_Dmc             dmc;
	
	double tempo_;
	nes_time_t last_time;  
	nes_time_t last_dmc_time;
	nes_time_t earliest_irq_;
	nes_time_t next_irq;
	int frame_period;
	int frame_delay;  
	int frame;  
	int osc_enables;
	int frame_mode;
	bool irq_flag;
	void (*irq_notifier_)( void* user_data );
	void* irq_data;
	Nes_Square::Synth square_synth;  
	
	void irq_changed();
	void state_restored();
	void run_until_( nes_time_t );
	

	friend class Nes_Core;
};

inline void Nes_Apu::osc_output( int osc, Blip_Buffer* buf )
{
	assert( (unsigned) osc < osc_count );
	oscs [osc]->output = buf;
}

inline nes_time_t Nes_Apu::earliest_irq( nes_time_t ) const
{
	return earliest_irq_;
}

inline void Nes_Apu::dmc_reader( int (*func)( void*, nes_addr_t ), void* user_data )
{
	dmc.prg_reader_data = user_data;
	dmc.prg_reader = func;
}

inline void Nes_Apu::irq_notifier( void (*func)( void* user_data ), void* user_data )
{
	irq_notifier_ = func;
	irq_data = user_data;
}

inline int Nes_Apu::count_dmc_reads( nes_time_t time, nes_time_t* last_read ) const
{
	return dmc.count_reads( time, last_read );
}
	
inline nes_time_t Nes_Dmc::next_read_time() const
{
	if ( length_counter == 0 )
		return Nes_Apu::no_irq;  
	
	return apu->last_dmc_time + delay + long (bits_remain - 1) * period;
}

inline nes_time_t Nes_Apu::next_dmc_read_time() const { return dmc.next_read_time(); }

#endif
