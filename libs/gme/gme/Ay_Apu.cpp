

#include "Ay_Apu.h"

/* Copyright (C) 2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"








unsigned const inaudible_freq = 16384;

int const period_factor = 16;

static byte const amp_table [16] =
{
#define ENTRY( n ) byte (n * Ay_Apu::amp_range + 0.5)


	ENTRY(0.000000),ENTRY(0.007813),ENTRY(0.011049),ENTRY(0.015625),
	ENTRY(0.022097),ENTRY(0.031250),ENTRY(0.044194),ENTRY(0.062500),
	ENTRY(0.088388),ENTRY(0.125000),ENTRY(0.176777),ENTRY(0.250000),
	ENTRY(0.353553),ENTRY(0.500000),ENTRY(0.707107),ENTRY(1.000000),
	
	  
                                                         
 
                                                  
                                                                 
                                                                 
                                                                 
                                                                 
                       
                                                                 
                                                                 
                                                                 
                                                                 
   
#undef ENTRY
};

static byte const modes [8] =
{
#define MODE( a0,a1, b0,b1, c0,c1 ) \
		(a0 | a1<<1 | b0<<2 | b1<<3 | c0<<4 | c1<<5)
	MODE( 1,0, 1,0, 1,0 ),
	MODE( 1,0, 0,0, 0,0 ),
	MODE( 1,0, 0,1, 1,0 ),
	MODE( 1,0, 1,1, 1,1 ),
	MODE( 0,1, 0,1, 0,1 ),
	MODE( 0,1, 1,1, 1,1 ),
	MODE( 0,1, 1,0, 0,1 ),
	MODE( 0,1, 0,0, 0,0 ),
};

Ay_Apu::Ay_Apu()
{

	for ( int m = 8; m--; )
	{
		byte* out = env.modes [m];
		int flags = modes [m];
		for ( int x = 3; --x >= 0; )
		{
			int amp = flags & 1;
			int end = flags >> 1 & 1;
			int step = end - amp;
			amp *= 15;
			for ( int y = 16; --y >= 0; )
			{
				*out++ = amp_table [amp];
				amp += step;
			}
			flags >>= 2;
		}
	}
	
	output( 0 );
	volume( 1.0 );
	reset();
}

void Ay_Apu::reset()
{
	last_time   = 0;
	noise.delay = 0;
	noise.lfsr  = 1;
	
	osc_t* osc = &oscs [osc_count];
	do
	{
		osc--;
		osc->period   = period_factor;
		osc->delay    = 0;
		osc->last_amp = 0;
		osc->phase    = 0;
	}
	while ( osc != oscs );
	
	for ( int i = sizeof regs; --i >= 0; )
		regs [i] = 0;
	regs [7] = 0xFF;
	write_data_( 13, 0 );
}

void Ay_Apu::write_data_( int addr, int data )
{
	assert( (unsigned) addr < reg_count );
	
	if ( (unsigned) addr >= 14 )
	{
		#ifdef debug_printf
			debug_printf( "Wrote to I/O port %02X\n", (int) addr );
		#endif
	}
	

	if ( addr == 13 )
	{
		if ( !(data & 8) )  
			data = (data & 4) ? 15 : 9;
		env.wave = env.modes [data - 7];
		env.pos = -48;
		env.delay = 0;  
	}
	regs [addr] = data;
	

	int i = addr >> 1;
	if ( i < osc_count )
	{
		blip_time_t period = (regs [i * 2 + 1] & 0x0F) * (0x100L * period_factor) +
				regs [i * 2] * period_factor;
		if ( !period )
			period = period_factor;
		

		osc_t& osc = oscs [i];
		if ( (osc.delay += period - osc.period) < 0 )
			osc.delay = 0;
		osc.period = period;
	}
	

}

int const noise_off = 0x08;
int const tone_off  = 0x01;

void Ay_Apu::run_until( blip_time_t final_end_time )
{
	require( final_end_time >= last_time );
	

	blip_time_t const noise_period_factor = period_factor * 2;  
	blip_time_t noise_period = (regs [6] & 0x1F) * noise_period_factor;
	if ( !noise_period )
		noise_period = noise_period_factor;
	blip_time_t const old_noise_delay = noise.delay;
	blargg_ulong const old_noise_lfsr = noise.lfsr;
	

	blip_time_t const env_period_factor = period_factor * 2;  
	blip_time_t env_period = (regs [12] * 0x100L + regs [11]) * env_period_factor;
	if ( !env_period )
		env_period = env_period_factor;  
	if ( !env.delay )
		env.delay = env_period;
	

	for ( int index = 0; index < osc_count; index++ )
	{
		osc_t* const osc = &oscs [index];
		int osc_mode = regs [7] >> index;
		

		Blip_Buffer* const osc_output = osc->output;
		if ( !osc_output )
			continue;
		osc_output->set_modified();
		

		int half_vol = 0;
		blip_time_t inaudible_period = (blargg_ulong) (osc_output->clock_rate() +
				inaudible_freq) / (inaudible_freq * 2);
		if ( osc->period <= inaudible_period && !(osc_mode & tone_off) )
		{
			half_vol = 1;  
			osc_mode |= tone_off;
		}
		

		blip_time_t start_time = last_time;
		blip_time_t end_time   = final_end_time;
		int const vol_mode = regs [0x08 + index];
		int volume = amp_table [vol_mode & 0x0F] >> half_vol;
		int osc_env_pos = env.pos;
		if ( vol_mode & 0x10 )
		{
			volume = env.wave [osc_env_pos] >> half_vol;

			if ( !(regs [13] & 1) || osc_env_pos < -32 )
			{
				end_time = start_time + env.delay;
				if ( end_time >= final_end_time )
					end_time = final_end_time;
				


			}
			else if ( !volume )
			{
				osc_mode = noise_off | tone_off;
			}
		}
		else if ( !volume )
		{
			osc_mode = noise_off | tone_off;
		}
		

		blip_time_t const period = osc->period;
		blip_time_t time = start_time + osc->delay;
		if ( osc_mode & tone_off )  
		{
			blargg_long count = (final_end_time - time + period - 1) / period;
			time += count * period;
			osc->phase ^= count & 1;
		}
		

		blip_time_t ntime = final_end_time;
		blargg_ulong noise_lfsr = 1;
		if ( !(osc_mode & noise_off) )
		{
			ntime = start_time + old_noise_delay;
			noise_lfsr = old_noise_lfsr;


		}
		







		



		while ( 1 )
		{

			int amp = 0;
			if ( (osc_mode | osc->phase) & 1 & (osc_mode >> 3 | noise_lfsr) )
				amp = volume;
			{
				int delta = amp - osc->last_amp;
				if ( delta )
				{
					osc->last_amp = amp;
					synth_.offset( start_time, delta, osc_output );
				}
			}
			



			if ( ntime < end_time || time < end_time )
			{


				int delta = amp * 2 - volume;
				int delta_non_zero = delta != 0;
				int phase = osc->phase | (osc_mode & tone_off); assert( tone_off == 0x01 );
				do
				{

					blip_time_t end = end_time;
					if ( end_time > time ) end = time;
					if ( phase & delta_non_zero )
					{
						while ( ntime <= end )  
						{
							int changed = noise_lfsr + 1;
							noise_lfsr = (-(noise_lfsr & 1) & 0x12000) ^ (noise_lfsr >> 1);
							if ( changed & 2 )
							{
								delta = -delta;
								synth_.offset( ntime, delta, osc_output );
							}
							ntime += noise_period;
						}
					}
					else
					{

						blargg_long remain = end - ntime;
						blargg_long count = remain / noise_period;
						if ( remain >= 0 )
							ntime += noise_period + count * noise_period;
					}
					

					end = end_time;
					if ( end_time > ntime ) end = ntime;
					if ( noise_lfsr & delta_non_zero )
					{
						while ( time < end )
						{
							delta = -delta;
							synth_.offset( time, delta, osc_output );
							time += period;

						}

						phase = unsigned (-delta) >> (CHAR_BIT * sizeof (unsigned) - 1);

					}
					else
					{


						
						while ( time < end )
						{
							time += period;
							phase ^= 1;
						}
					}
				}
				while ( time < end_time || ntime < end_time );
				
				osc->last_amp = (delta + volume) >> 1;
				if ( !(osc_mode & tone_off) )
					osc->phase = phase;
			}
			
			if ( end_time >= final_end_time )
				break;  
			

			if ( ++osc_env_pos >= 0 )
				osc_env_pos -= 32;
			volume = env.wave [osc_env_pos] >> half_vol;
			
			start_time = end_time;
			end_time += env_period;
			if ( end_time > final_end_time )
				end_time = final_end_time;
		}
		osc->delay = time - final_end_time;
		
		if ( !(osc_mode & noise_off) )
		{
			noise.delay = ntime - final_end_time;
			noise.lfsr = noise_lfsr;
		}
	}
	

	

	blip_time_t remain = final_end_time - last_time - env.delay;
	if ( remain >= 0 )
	{
		blargg_long count = (remain + env_period) / env_period;
		env.pos += count;
		if ( env.pos >= 0 )
			env.pos = (env.pos & 31) - 32;
		remain -= count * env_period;
		assert( -remain <= env_period );
	}
	env.delay = -remain;
	assert( env.delay > 0 );
	assert( env.pos < 0 );
	
	last_time = final_end_time;
}
