


#ifndef EFFECTS_BUFFER_H
#define EFFECTS_BUFFER_H

#include "Multi_Buffer.h"


class Effects_Buffer : public Multi_Buffer {
public:


	Effects_Buffer( bool center_only = false );
	







	

	struct config_t {
		double pan_1;            
		double pan_2;
		double echo_delay;       
		double echo_level;       
		double reverb_delay;     
		double delay_variance;   
		double reverb_level;     
		bool effects_enabled;    
		config_t();
	};
	

	virtual void config( const config_t& );
	void set_depth( double );
	
public:
	~Effects_Buffer();
	blargg_err_t set_sample_rate( long samples_per_sec, int msec = blip_default_length );
	void clock_rate( long );
	void bass_freq( int );
	void clear();
	channel_t channel( int, int );
	void end_frame( blip_time_t );
	long read_samples( blip_sample_t*, long );
	long samples_avail() const;
private:
	typedef long fixed_t;
	
	enum { max_buf_count = 7 };
	Blip_Buffer bufs [max_buf_count];
	enum { chan_types_count = 3 };
	channel_t chan_types [3];
	config_t config_;
	long stereo_remain;
	long effect_remain;
	int buf_count;
	bool effects_enabled;
	
	blargg_vector<blip_sample_t> reverb_buf;
	blargg_vector<blip_sample_t> echo_buf;
	int reverb_pos;
	int echo_pos;
	
	struct {
		fixed_t pan_1_levels [2];
		fixed_t pan_2_levels [2];
		int echo_delay_l;
		int echo_delay_r;
		fixed_t echo_level;
		int reverb_delay_l;
		int reverb_delay_r;
		fixed_t reverb_level;
	} chans;
	
	void mix_mono( blip_sample_t*, blargg_long );
	void mix_stereo( blip_sample_t*, blargg_long );
	void mix_enhanced( blip_sample_t*, blargg_long );
	void mix_mono_enhanced( blip_sample_t*, blargg_long );
};

#endif
