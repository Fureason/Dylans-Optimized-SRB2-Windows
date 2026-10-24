 

#ifndef WAVE_WRITER_H
#define WAVE_WRITER_H

#ifdef __cplusplus
	extern "C" {
#endif

 
void wave_open( long sample_rate, const char* filename );
void wave_enable_stereo( void );
void wave_write( const short* buf, long count );
long wave_sample_count( void );
void wave_close( void );

#ifdef __cplusplus
	}
#endif

#ifdef __cplusplus
#include <stddef.h>
#include <stdio.h>

 
class Wave_Writer {
public:
	typedef short sample_t;
	


	Wave_Writer( long sample_rate, char const* filename = "out.wav" );
	

	void enable_stereo();
	


	void write( const sample_t*, long count, int skip = 1 );
	


	void write( const float*, long count, int skip = 1 );
	

	long sample_count() const;
	

	void close();
	
	~Wave_Writer();
public:

	void stereo( bool b ) { chan_count = b ? 2 : 1; }
private:
	enum { buf_size = 32768 * 2 };
	unsigned char* buf;
	FILE*   file;
	long    sample_count_;
	long    rate;
	long    buf_pos;
	int     chan_count;
	
	void flush();
};

inline void Wave_Writer::enable_stereo() { chan_count = 2; }

inline long Wave_Writer::sample_count() const { return sample_count_; }

#endif

#endif
