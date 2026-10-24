


#ifndef FIR_RESAMPLER_H
#define FIR_RESAMPLER_H

#include "blargg_common.h"
#include <string.h>

class Fir_Resampler_ {
public:
	

	


	double time_ratio( double factor, double rolloff = 0.999, double gain = 1.0 );
	

	double ratio() const { return ratio_; }
	

	
	typedef short sample_t;
	

	blargg_err_t buffer_size( int );
	


	void clear();
	

	int max_write() const { return buf.end() - write_pos; }
	

	sample_t* buffer() { return write_pos; }
	

	void write( long count );
	

	int written() const { return write_pos - &buf [write_offset]; }
	

	int skip_input( long count );
	

	

	int input_needed( blargg_long count ) const;
	

	int avail() const { return avail_( write_pos - &buf [width_ * stereo] ); }
	
public:
	~Fir_Resampler_();
protected:
	enum { stereo = 2 };
	enum { max_res = 32 };
	blargg_vector<sample_t> buf;
	sample_t* write_pos;
	int res;
	int imp_phase;
	int const width_;
	int const write_offset;
	blargg_ulong skip_bits;
	int step;
	int input_per_cycle;
	double ratio_;
	sample_t* impulses;
	
	Fir_Resampler_( int width, sample_t* );
	int avail_( blargg_long input_count ) const;
};



template<int width>
class Fir_Resampler : public Fir_Resampler_ {
	BOOST_STATIC_ASSERT( width >= 4 && width % 2 == 0 );
	short impulses [max_res] [width];
public:
	Fir_Resampler() : Fir_Resampler_( width, impulses [0] ) { }
	

	typedef short sample_t;
	int read( sample_t* out, blargg_long count );
};



inline void Fir_Resampler_::write( long count )
{
	write_pos += count;
	assert( write_pos <= buf.end() );
}

template<int width>
int Fir_Resampler<width>::read( sample_t* out_begin, blargg_long count )
{
	sample_t* out = out_begin;
	const sample_t* in = buf.begin();
	sample_t* end_pos = write_pos;
	blargg_ulong skip = skip_bits >> imp_phase;
	sample_t const* imp = impulses [imp_phase];
	int remain = res - imp_phase;
	int const step = this->step;
	
	count >>= 1;
	
	if ( end_pos - in >= width * stereo )
	{
		end_pos -= width * stereo;
		do
		{
			count--;
			

			blargg_long l = 0;
			blargg_long r = 0;
			
			const sample_t* i = in;
			if ( count < 0 )
				break;
			
			for ( int n = width / 2; n; --n )
			{
				int pt0 = imp [0];
				l += pt0 * i [0];
				r += pt0 * i [1];
				int pt1 = imp [1];
				imp += 2;
				l += pt1 * i [2];
				r += pt1 * i [3];
				i += 4;
			}
			
			remain--;
			
			l >>= 15;
			r >>= 15;
			
			in += (skip * stereo) & stereo;
			skip >>= 1;
			in += step;
			
			if ( !remain )
			{
				imp = impulses [0];
				skip = skip_bits;
				remain = res;
			}
			
			out [0] = (sample_t) l;
			out [1] = (sample_t) r;
			out += 2;
		}
		while ( in <= end_pos );
	}
	
	imp_phase = res - remain;
	
	int left = write_pos - in;
	write_pos = &buf [left];
	memmove( buf.begin(), in, left * sizeof *in );
	
	return out - out_begin;
}

#endif
