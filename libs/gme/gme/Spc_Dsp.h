


#ifndef SPC_DSP_H
#define SPC_DSP_H

#include "blargg_common.h"

struct Spc_Dsp {
public:
	typedef BOOST::uint8_t uint8_t;
	

	

	void init( void* ram_64k );



	typedef short sample_t;
	void set_output( sample_t* out, int out_size );




	int sample_count() const;


	

	void reset();


	void soft_reset();
	


	int  read ( int addr ) const;
	void write( int addr, int data );



	void run( int clock_count );





	enum { voice_count = 8 };
	void mute_voices( int mask );


	void disable_surround( bool disable = true );


	

	enum { register_count = 128 };
	void load( uint8_t const regs [register_count] );




	enum {
	    r_mvoll = 0x0C, r_mvolr = 0x1C,
	    r_evoll = 0x2C, r_evolr = 0x3C,
	    r_kon   = 0x4C, r_koff  = 0x5C,
	    r_flg   = 0x6C, r_endx  = 0x7C,
	    r_efb   = 0x0D, r_pmon  = 0x2D,
	    r_non   = 0x3D, r_eon   = 0x4D,
	    r_dir   = 0x5D, r_esa   = 0x6D,
	    r_edl   = 0x7D,
	    r_fir   = 0x0F  
	};


	enum {
		v_voll   = 0x00, v_volr   = 0x01,
		v_pitchl = 0x02, v_pitchh = 0x03,
		v_srcn   = 0x04, v_adsr0  = 0x05,
		v_adsr1  = 0x06, v_gain   = 0x07,
		v_envx   = 0x08, v_outx   = 0x09
	};

public:
	enum { extra_size = 16 };
	sample_t* extra()               { return m.extra; }
	sample_t const* out_pos() const { return m.out; }
public:
	BLARGG_DISABLE_NOTHROW
	
	typedef BOOST::int8_t   int8_t;
	typedef BOOST::int16_t int16_t;
	
	enum { echo_hist_size = 8 };
	
	enum env_mode_t { env_release, env_attack, env_decay, env_sustain };
	enum { brr_buf_size = 12 };
	struct voice_t
	{
		int buf [brr_buf_size*2]; 
		int* buf_pos;            
		int interp_pos;          
		int brr_addr;            
		int brr_offset;          
		int kon_delay;           
		env_mode_t env_mode;
		int env;                 
		int hidden_env;          
		int volume [2];          
		int enabled;             
	};
private:
	struct state_t
	{
		uint8_t regs [register_count];
		

		int echo_hist [echo_hist_size * 2] [2];
		int (*echo_hist_pos) [2];  
		
		int every_other_sample;  
		int kon;                 
		int noise;
		int echo_offset;         
		int echo_length;         
		int phase;               
		unsigned counters [4];
		
		int new_kon;
		int t_koff;
		
		voice_t voices [voice_count];
		
		unsigned* counter_select [32];
		

		uint8_t* ram;  
		int mute_mask;
		int surround_threshold;
		sample_t* out;
		sample_t* out_end;
		sample_t* out_begin;
		sample_t extra [extra_size];
	};
	state_t m;
	
	void init_counter();
	void run_counter( int );
	void soft_reset_common();
	void write_outline( int addr, int data );
	void update_voice_vol( int addr );
};

#include <assert.h>

inline int Spc_Dsp::sample_count() const { return m.out - m.out_begin; }

inline int Spc_Dsp::read( int addr ) const
{
	assert( (unsigned) addr < register_count );
	return m.regs [addr];
}

inline void Spc_Dsp::update_voice_vol( int addr )
{
	int l = (int8_t) m.regs [addr + v_voll];
	int r = (int8_t) m.regs [addr + v_volr];
	
	if ( l * r < m.surround_threshold )
	{

		l ^= l >> 7;
		r ^= r >> 7;
	}
	
	voice_t& v = m.voices [addr >> 4];
	int enabled = v.enabled;
	v.volume [0] = l & enabled;
	v.volume [1] = r & enabled;
}

inline void Spc_Dsp::write( int addr, int data )
{
	assert( (unsigned) addr < register_count );
	
	m.regs [addr] = (uint8_t) data;
	int low = addr & 0x0F;
	if ( low < 0x2 )  
	{
		update_voice_vol( low ^ addr );
	}
	else if ( low == 0xC )
	{
		if ( addr == r_kon )
			m.new_kon = (uint8_t) data;
		
		if ( addr == r_endx )  
			m.regs [r_endx] = 0;
	}
}

inline void Spc_Dsp::disable_surround( bool disable )
{
	m.surround_threshold = disable ? 0 : -0x4000;
}

#define SPC_NO_COPY_STATE_FUNCS 1

#define SPC_LESS_ACCURATE 1

#endif
