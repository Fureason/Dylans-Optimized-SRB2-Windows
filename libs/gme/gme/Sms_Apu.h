


#ifndef SMS_APU_H
#define SMS_APU_H

#include "Sms_Oscs.h"

class Sms_Apu {
public:

	void volume( double );
	

	void treble_eq( const blip_eq_t& );
	


	


	void output( Blip_Buffer* mono );
	void output( Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	



	enum { osc_count = 4 };
	void osc_output( int index, Blip_Buffer* mono );
	void osc_output( int index, Blip_Buffer* center, Blip_Buffer* left, Blip_Buffer* right );
	

	void reset( unsigned noise_feedback = 0, int noise_width = 0 );
	

	void write_ggstereo( blip_time_t, int );
	

	void write_data( blip_time_t, int );
	


	void end_frame( blip_time_t );

public:
	Sms_Apu();
	~Sms_Apu();
private:

	Sms_Apu( const Sms_Apu& );
	Sms_Apu& operator = ( const Sms_Apu& );
	
	Sms_Osc*    oscs [osc_count];
	Sms_Square  squares [3];
	Sms_Square::Synth square_synth;  
	blip_time_t last_time;
	int         latch;
	Sms_Noise   noise;
	unsigned    noise_feedback;
	unsigned    looped_feedback;
	
	void run_until( blip_time_t );
};

struct sms_apu_state_t
{
	unsigned char regs [8] [2];
	unsigned char latch;
};

inline void Sms_Apu::output( Blip_Buffer* b ) { output( b, b, b ); }

inline void Sms_Apu::osc_output( int i, Blip_Buffer* b ) { osc_output( i, b, b, b ); }

#endif
