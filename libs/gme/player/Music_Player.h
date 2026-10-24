


#ifndef MUSIC_PLAYER_H
#define MUSIC_PLAYER_H

#include "gme.h"

class Music_Player {
public:

	gme_err_t init( long sample_rate = 44100 );
	

	gme_err_t load_file( const char* path );
	

	gme_err_t start_track( int track );
	

	void stop();
	



	int track_count() const;
	

	gme_info_t const& track_info() const { return *track_info_; }
	

	void pause( int );
	

	bool track_ended() const;
	

	Music_Emu* emu() const { return emu_; }
	

	void set_stereo_depth( double );
	

	void enable_accuracy( bool );
	

	void set_tempo( double );
	

	void mute_voices( int );
	

	typedef short sample_t;
	void set_scope_buffer( sample_t* buf, int size ) { scope_buf = buf; scope_buf_size = size; }
	
public:
	Music_Player();
	~Music_Player();
private:
	Music_Emu* emu_;
	sample_t* scope_buf;
	long sample_rate;
	int scope_buf_size;
	bool paused;
	gme_info_t* track_info_;
	
	void suspend();
	void resume();
	static void fill_buffer( void*, sample_t*, int );
};

#endif
