


#ifndef MUSIC_EMU_H
#define MUSIC_EMU_H

#include "Gme_File.h"
class Multi_Buffer;

struct Music_Emu : public Gme_File {
public:



	blargg_err_t set_sample_rate( long sample_rate );
	

	blargg_err_t start_track( int );
	


	typedef short sample_t;
	blargg_err_t play( long count, sample_t* buf );
	

	

	long sample_rate() const;
	

	int current_track() const;
	

	int voice_count() const;
	

	const char** voice_names() const;
	



	long tell() const;
	

	long tell_samples() const;


	blargg_err_t seek( long msec );
	

	blargg_err_t seek_samples( long n );
	

	blargg_err_t skip( long n );
	

	bool track_ended() const;
	


	void set_fade( long start_msec, long length_msec = 8000 );
	

	void ignore_silence( bool disable = true );
	

	using Gme_File::track_info;
	blargg_err_t track_info( track_info_t* out ) const;
	

	


	void set_tempo( double );
	

	void mute_voice( int index, bool mute = true );
	


	void mute_voices( int mask );
	


	void set_gain( double );
	


	virtual void set_buffer( Multi_Buffer* ) { }
	


	void enable_accuracy( bool enable = true );
	




	typedef gme_equalizer_t equalizer_t;
	

	equalizer_t const& equalizer() const;
	

	void set_equalizer( equalizer_t const& );


	static const equalizer_t make_equalizer( double treble, double bass )
	{
	    const Music_Emu::equalizer_t e = { treble, bass,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	    return e;
	}
	

	static equalizer_t const tv_eq;
	
public:
	Music_Emu();
	~Music_Emu();
protected:
	void set_max_initial_silence( int n )       { max_initial_silence = n; }
	void set_silence_lookahead( int n )         { silence_lookahead = n; }
	void set_voice_count( int n )               { voice_count_ = n; }
	void set_voice_names( const char* const* names );
	void set_track_ended()                      { emu_track_ended_ = true; }
	double gain() const                         { return gain_; }
	double tempo() const                        { return tempo_; }
	void remute_voices();
	
	virtual blargg_err_t set_sample_rate_( long sample_rate ) = 0;
	virtual void set_equalizer_( equalizer_t const& ) { }
	virtual void enable_accuracy_( bool   ) { }
	virtual void mute_voices_( int mask ) = 0;
	virtual void set_tempo_( double ) = 0;
	virtual blargg_err_t start_track_( int ) = 0;  
	virtual blargg_err_t play_( long count, sample_t* out ) = 0;
	virtual blargg_err_t skip_( long count );
protected:
	virtual void unload();
	virtual void pre_load();
	virtual void post_load_();
private:

	equalizer_t equalizer_;
	int max_initial_silence;
	const char** voice_names_;
	int voice_count_;
	int mute_mask_;
	double tempo_;
	double gain_;
	
	long sample_rate_;
	blargg_long msec_to_samples( blargg_long msec ) const;
	

	int current_track_;
	blargg_long out_time;   
	blargg_long emu_time;   
	bool emu_track_ended_;  
	volatile bool track_ended_;
	void clear_track_vars();
	void end_track_if_error( blargg_err_t );
	

	blargg_long fade_start;
	int fade_step;
	void handle_fade( long count, sample_t* out );
	

	int silence_lookahead;  
	bool ignore_silence_;
	long silence_time;      
	long silence_count;     
	long buf_remain;        
	enum { buf_size = 2048 };
	blargg_vector<sample_t> buf;
	void fill_buf();
	void emu_play( long count, sample_t* out );
	
	Multi_Buffer* effects_buffer;
	friend Music_Emu* gme_new_emu( gme_type_t, int );
	friend void gme_set_stereo_depth( Music_Emu*, double );
};


struct Gme_Info_ : Music_Emu
{
	virtual blargg_err_t set_sample_rate_( long sample_rate );
	virtual void set_equalizer_( equalizer_t const& );
	virtual void enable_accuracy_( bool );
	virtual void mute_voices_( int mask );
	virtual void set_tempo_( double );
	virtual blargg_err_t start_track_( int );
	virtual blargg_err_t play_( long count, sample_t* out );
	virtual void pre_load();
	virtual void post_load_();
};

inline blargg_err_t Music_Emu::track_info( track_info_t* out ) const
{
	return track_info( out, current_track_ );
}

inline long Music_Emu::sample_rate() const          { return sample_rate_; }
inline const char** Music_Emu::voice_names() const  { return voice_names_; }
inline int Music_Emu::voice_count() const           { return voice_count_; }
inline int Music_Emu::current_track() const         { return current_track_; }
inline bool Music_Emu::track_ended() const          { return track_ended_; }
inline const Music_Emu::equalizer_t& Music_Emu::equalizer() const { return equalizer_; }

inline void Music_Emu::enable_accuracy( bool b )    { enable_accuracy_( b ); }
inline void Music_Emu::set_tempo_( double t )       { tempo_ = t; }
inline void Music_Emu::remute_voices()              { mute_voices( mute_mask_ ); }
inline void Music_Emu::ignore_silence( bool b )     { ignore_silence_ = b; }
inline blargg_err_t Music_Emu::start_track_( int )  { return 0; }

inline void Music_Emu::set_voice_names( const char* const* names )
{

	voice_names_ = const_cast<const char**> (names);
}

inline void Music_Emu::mute_voices_( int ) { }

inline void Music_Emu::set_gain( double g )
{
	assert( !sample_rate() );  
	gain_ = g;
}

#endif
