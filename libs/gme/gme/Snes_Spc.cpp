



#include "Snes_Spc.h"

#include <string.h>

/* Copyright (C) 2004-2007 Shay Green. This module is free software; you
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

#define RAM         (m.ram.ram)
#define REGS        (m.smp_regs [0])
#define REGS_IN     (m.smp_regs [1])


#define IF_0_THEN_256( n ) ((uint8_t) ((n) - 1) + 1)




blargg_err_t Snes_Spc::init()
{
	memset( &m, 0, sizeof m );
	dsp.init( RAM );
	
	m.tempo = tempo_unit;
	


	m.rom [0x3E] = 0xFF;
	m.rom [0x3F] = 0xC0;
	
	static unsigned char const cycle_table [128] =
	{ 
	    0x28,0x47,0x34,0x36,0x26,0x54,0x54,0x68,  
	    0x48,0x47,0x45,0x56,0x55,0x65,0x22,0x46,  
	    0x28,0x47,0x34,0x36,0x26,0x54,0x54,0x74,  
	    0x48,0x47,0x45,0x56,0x55,0x65,0x22,0x38,  
	    0x28,0x47,0x34,0x36,0x26,0x44,0x54,0x66,  
	    0x48,0x47,0x45,0x56,0x55,0x45,0x22,0x43,  
	    0x28,0x47,0x34,0x36,0x26,0x44,0x54,0x75,  
	    0x48,0x47,0x45,0x56,0x55,0x55,0x22,0x36,  
	    0x28,0x47,0x34,0x36,0x26,0x54,0x52,0x45,  
	    0x48,0x47,0x45,0x56,0x55,0x55,0x22,0xC5,  
	    0x38,0x47,0x34,0x36,0x26,0x44,0x52,0x44,  
	    0x48,0x47,0x45,0x56,0x55,0x55,0x22,0x34,  
	    0x38,0x47,0x45,0x47,0x25,0x64,0x52,0x49,  
	    0x48,0x47,0x56,0x67,0x45,0x55,0x22,0x83,  
	    0x28,0x47,0x34,0x36,0x24,0x53,0x43,0x40,  
	    0x48,0x47,0x45,0x56,0x34,0x54,0x22,0x60,  
	};
	

	for ( int i = 0; i < 128; i++ )
	{
		int n = cycle_table [i];
		m.cycle_table [i * 2 + 0] = n >> 4;
		m.cycle_table [i * 2 + 1] = n & 0x0F;
	}
	
	#if SPC_LESS_ACCURATE
		memcpy( reg_times, reg_times_, sizeof reg_times );
	#endif
	
	reset();
	return 0;
}

void Snes_Spc::init_rom( uint8_t const in [rom_size] )
{
	memcpy( m.rom, in, sizeof m.rom );
}

void Snes_Spc::set_tempo( int t )
{
	m.tempo = t;
	int const timer2_shift = 4;  
	int const other_shift  = 3;  
	
	#if SPC_DISABLE_TEMPO
		m.timers [2].prescaler = timer2_shift;
		m.timers [1].prescaler = timer2_shift + other_shift;
		m.timers [0].prescaler = timer2_shift + other_shift;
	#else
		if ( !t )
			t = 1;
		int const timer2_rate  = 1 << timer2_shift;
		int rate = (timer2_rate * tempo_unit + (t >> 1)) / t;
		if ( rate < timer2_rate / 4 )
			rate = timer2_rate / 4;  
		m.timers [2].prescaler = rate;
		m.timers [1].prescaler = rate << other_shift;
		m.timers [0].prescaler = rate << other_shift;
	#endif
}



void Snes_Spc::timers_loaded()
{
	int i;
	for ( i = 0; i < timer_count; i++ )
	{
		Timer* t = &m.timers [i];
		t->period  = IF_0_THEN_256( REGS [r_t0target + i] );
		t->enabled = REGS [r_control] >> i & 1;
		t->counter = REGS_IN [r_t0out + i] & 0x0F;
	}
	
	set_tempo( m.tempo );
}


void Snes_Spc::load_regs( uint8_t const in [reg_count] )
{
	memcpy( REGS, in, reg_count );
	memcpy( REGS_IN, REGS, reg_count );
	

	REGS_IN [r_test    ] = 0;
	REGS_IN [r_control ] = 0;
	REGS_IN [r_t0target] = 0;
	REGS_IN [r_t1target] = 0;
	REGS_IN [r_t2target] = 0;
}



void Snes_Spc::ram_loaded()
{
	m.rom_enabled = 0;
	load_regs( &RAM [0xF0] );
	

	memset( m.ram.padding1, cpu_pad_fill, sizeof m.ram.padding1 );
	memset( m.ram.padding2, cpu_pad_fill, sizeof m.ram.padding2 );
}


void Snes_Spc::regs_loaded()
{
	enable_rom( REGS [r_control] & 0x80 );
	timers_loaded();
}

void Snes_Spc::reset_time_regs()
{
	m.cpu_error     = 0;
	m.echo_accessed = 0;
	m.spc_time      = 0;
	m.dsp_time      = 0;
	#if SPC_LESS_ACCURATE
		m.dsp_time = clocks_per_sample + 1;
	#endif
	
	for ( int i = 0; i < timer_count; i++ )
	{
		Timer* t = &m.timers [i];
		t->next_time = 1;
		t->divider   = 0;
	}
	
	regs_loaded();
	
	m.extra_clocks = 0;
	reset_buf();
}

void Snes_Spc::reset_common( int timer_counter_init )
{
	int i;
	for ( i = 0; i < timer_count; i++ )
		REGS_IN [r_t0out + i] = timer_counter_init;
	

	memset( &m.cpu_regs, 0, sizeof m.cpu_regs );
	m.cpu_regs.pc = rom_addr;
	
	REGS [r_test   ] = 0x0A;
	REGS [r_control] = 0xB0;  
	for ( i = 0; i < port_count; i++ )
		REGS_IN [r_cpuio0 + i] = 0;
	
	reset_time_regs();
}

void Snes_Spc::soft_reset()
{
	reset_common( 0 );
	dsp.soft_reset();
}

void Snes_Spc::reset()
{
	memset( RAM, 0xFF, 0x10000 );
	ram_loaded();
	reset_common( 0x0F );
	dsp.reset();
}

char const Snes_Spc::signature [signature_size + 1] =
		"SNES-SPC700 Sound File Data v0.30\x1A\x1A";

blargg_err_t Snes_Spc::load_spc( void const* data, long size )
{
	spc_file_t const* const spc = (spc_file_t const*) data;
	

	assert( sizeof (spc_file_t) == spc_min_file_size + 0x80 );
	

	if ( size < signature_size || memcmp( spc, signature, 27 ) )
		return "Not an SPC file";
	
	if ( size < spc_min_file_size )
		return "Corrupt SPC file";
	

	m.cpu_regs.pc  = spc->pch * 0x100 + spc->pcl;
	m.cpu_regs.a   = spc->a;
	m.cpu_regs.x   = spc->x;
	m.cpu_regs.y   = spc->y;
	m.cpu_regs.psw = spc->psw;
	m.cpu_regs.sp  = spc->sp;
	

	memcpy( RAM, spc->ram, 0x10000 );
	ram_loaded();
	

	dsp.load( spc->dsp );
	
	reset_time_regs();
	
	return 0;
}

void Snes_Spc::clear_echo()
{
	if ( !(dsp.read( Spc_Dsp::r_flg ) & 0x20) )
	{
		int addr = 0x100 * dsp.read( Spc_Dsp::r_esa );
		int end  = addr + 0x800 * (dsp.read( Spc_Dsp::r_edl ) & 0x0F);
		if ( end > 0x10000 )
			end = 0x10000;
		memset( &RAM [addr], 0xFF, end - addr );
	}
}




void Snes_Spc::reset_buf()
{

	sample_t* out = m.extra_buf;
	while ( out < &m.extra_buf [extra_size / 2] )
		*out++ = 0;
	
	m.extra_pos = out;
	m.buf_begin = 0;
	
	dsp.set_output( 0, 0 );
}

void Snes_Spc::set_output( sample_t* out, int size )
{
	require( (size & 1) == 0 );  
	
	m.extra_clocks &= clocks_per_sample - 1;
	if ( out )
	{
		sample_t const* out_end = out + size;
		m.buf_begin = out;
		m.buf_end   = out_end;
		

		sample_t const* in = m.extra_buf;
		while ( in < m.extra_pos && out < out_end )
			*out++ = *in++;
		

		if ( out >= out_end )
		{

			out     = dsp.extra();
			out_end = &dsp.extra() [extra_size];
			

			while ( in < m.extra_pos )
				*out++ = *in++;
			assert( out <= out_end );
		}
		
		dsp.set_output( out, out_end - out );
	}
	else
	{
		reset_buf();
	}
}

void Snes_Spc::save_extra()
{

	sample_t const* main_end = m.buf_end;      
	sample_t const* dsp_end  = dsp.out_pos();  
	if ( m.buf_begin <= dsp_end && dsp_end <= main_end )
	{
		main_end = dsp_end;
		dsp_end  = dsp.extra();  
	}
	

	sample_t* out = m.extra_buf;
	sample_t const* in;
	for ( in = m.buf_begin + sample_count(); in < main_end; in++ )
		*out++ = *in;
	for ( in = dsp.extra(); in < dsp_end ; in++ )
		*out++ = *in;
	
	m.extra_pos = out;
	assert( out <= &m.extra_buf [extra_size] );
}

blargg_err_t Snes_Spc::play( int count, sample_t* out )
{
	require( (count & 1) == 0 );  
	if ( count )
	{
		set_output( out, count );
		end_frame( count * (clocks_per_sample / 2) );
	}
	
	const char* err = m.cpu_error;
	m.cpu_error = 0;
	return err;
}

blargg_err_t Snes_Spc::skip( int count )
{
	#if SPC_LESS_ACCURATE
	if ( count > 2 * sample_rate * 2 )
	{
		set_output( 0, 0 );
		

		time_t end = count;
		count = (count & 3) + 1 * sample_rate * 2;
		end = (end - count) * (clocks_per_sample / 2);
		
		m.skipped_kon  = 0;
		m.skipped_koff = 0;
		


		int old_dsp_time = m.dsp_time + m.spc_time;
		m.dsp_time = end - m.spc_time + skipping_time;
		end_frame( end );
		m.dsp_time = m.dsp_time - skipping_time + old_dsp_time;
		
		dsp.write( Spc_Dsp::r_koff, m.skipped_koff & ~m.skipped_kon );
		dsp.write( Spc_Dsp::r_kon , m.skipped_kon );
		clear_echo();
	}
	#endif
	
	return play( count, 0 );
}
