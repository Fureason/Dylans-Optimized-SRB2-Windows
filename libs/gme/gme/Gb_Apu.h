


#ifndef GB_APU_H
#define GB_APU_H

#include "Gb_Oscs.h"

class Gb_Apu {
public:
	

	void volume( double );
	

	void treble_eq( const blip_eq_t& );
	


	


	void output( Blip_Buffer* mono );
	void output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	



	enum { osc_count = 4 };
	void osc_output( int index, Blip_Buffer* mono );
	void osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	

	void reset();
	

	enum { start_addr = 0xFF10 };
	enum { end_addr   = 0xFF3F };
	enum { register_count = end_addr - start_addr + 1 };
	

	void write_register( blip_time_t, unsigned addr, int data );
	

	int read_register( blip_time_t, unsigned addr );
	


	void end_frame( blip_time_t );
	
	void set_tempo( double );
	
public:
	Gb_Apu();
private:

	Gb_Apu( const Gb_Apu& );
	Gb_Apu& operator = ( const Gb_Apu& );
	
	Gb_Osc*     oscs [osc_count];
	blip_time_t   next_frame_time;
	blip_time_t   last_time;
	blip_time_t frame_period;
	double      volume_unit;
	int         frame_count;
	
	Gb_Square   square1;
	Gb_Square   square2;
	Gb_Wave     wave;
	Gb_Noise    noise;
	BOOST::uint8_t regs [register_count];
	Gb_Square::Synth square_synth;  
	Gb_Wave::Synth   other_synth;   
	
	void update_volume();
	void run_until( blip_time_t );
	void write_osc( int index, int reg, int data );
};

inline void Gb_Apu::output( Blip_Buffer* b ) { output( b, b, b ); }
	
inline void Gb_Apu::osc_output( int i, Blip_Buffer* b ) { osc_output( i, b, b, b ); }

inline void Gb_Apu::volume( double vol )
{
	volume_unit = 0.60 / osc_count / 15   / 2   / 8   * vol;
	update_volume();
}

#endif
