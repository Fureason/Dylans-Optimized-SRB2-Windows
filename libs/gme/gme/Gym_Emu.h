



#ifndef GYM_EMU_H
#define GYM_EMU_H

#include "Dual_Resampler.h"
#include "Ym2612_Emu.h"
#include "Music_Emu.h"
#include "Sms_Apu.h"

class Gym_Emu : public Music_Emu, private Dual_Resampler {
public:

	enum { header_size = 428 };
	struct header_t
	{
	    char tag [4];
	    char song [32];
	    char game [32];
	    char copyright [32];
	    char emulator [32];
	    char dumper [32];
	    char comment [256];
	    byte loop_start [4];  
	    byte packed [4];
	};
	

	header_t const& header() const { return header_; }
	
	static gme_type_t static_type() { return gme_gym_type; }
	
public:

	using Music_Emu::load;
	blargg_err_t load( header_t const& h, Data_Reader& in )  
			{ return load_remaining_( &h, sizeof h, in ); }
	enum { gym_rate = 60 }; 
	long track_length() const;  

public:
	Gym_Emu();
	~Gym_Emu();
protected:
	blargg_err_t load_mem_( byte const*, long );
	blargg_err_t track_info_( track_info_t*, int track ) const;
	blargg_err_t set_sample_rate_( long sample_rate );
	blargg_err_t start_track_( int );
	blargg_err_t play_( long count, sample_t* );
	void mute_voices_( int );
	void set_tempo_( double );
	int play_frame( blip_time_t blip_time, int sample_count, sample_t* buf );
private:

	const byte* data;
	const byte* loop_begin;
	const byte* pos;
	const byte* data_end;
	blargg_long loop_remain;  
	header_t header_;
	double fm_sample_rate;
	blargg_long clocks_per_frame;
	void parse_frame();
	

	int dac_amp;
	int prev_dac_count;
	bool dac_enabled;
	bool dac_muted;
	void run_dac( int );
	

	Blip_Buffer blip_buf;
	Ym2612_Emu fm;
	Blip_Synth<blip_med_quality,1> dac_synth;
	Sms_Apu apu;
	byte dac_buf [1024];
};

#endif
