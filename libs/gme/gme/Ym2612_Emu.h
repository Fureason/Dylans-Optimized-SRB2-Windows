


#ifndef YM2612_EMU_H
#define YM2612_EMU_H

struct Ym2612_Impl;

class Ym2612_Emu  {
	Ym2612_Impl* impl;
public:
	Ym2612_Emu() { impl = 0; }
	~Ym2612_Emu();
	


	const char* set_rate( double sample_rate, double clock_rate );
	

	void reset();
	

	enum { channel_count = 6 };
	void mute_voices( int mask );
	

	void write0( int addr, int data );
	

	void write1( int addr, int data );
	

	typedef short sample_t;
	enum { out_chan_count = 2 };  
	void run( int pair_count, sample_t* out );
};

#endif
