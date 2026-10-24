

#include "Gb_Cpu.h"

#include <string.h>



/* Copyright (C) 2003-2006 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "gb_cpu_io.h"

#include "blargg_source.h"





















#if BLARGG_NONPORTABLE
	#define PAGE_OFFSET( addr ) (addr)
#else
	#define PAGE_OFFSET( addr ) ((addr) & (page_size - 1))
#endif

inline void Gb_Cpu::set_code_page( int i, uint8_t* p )
{
	state->code_map [i] = p - PAGE_OFFSET( i * (blargg_long) page_size );
}

void Gb_Cpu::reset( void* unmapped )
{
	check( state == &state_ );
	state = &state_;
	
	state_.remain = 0;
	
	for ( int i = 0; i < page_count + 1; i++ )
		set_code_page( i, (uint8_t*) unmapped );
	
	memset( &r, 0, sizeof r );

	
	blargg_verify_byte_order();
}

void Gb_Cpu::map_code( gb_addr_t start, unsigned size, void* data )
{

	require( start % page_size == 0 );
	require( size % page_size == 0 );
	
	unsigned first_page = start / page_size;
	for ( unsigned i = size / page_size; i--; )
		set_code_page( first_page + i, (uint8_t*) data + i * page_size );
}

#define READ( addr )            CPU_READ( this, (addr), s.remain )
#define WRITE( addr, data )     {CPU_WRITE( this, (addr), (data), s.remain );}
#define READ_FAST( addr, out )  CPU_READ_FAST( this, (addr), s.remain, out )
#define READ_PROG( addr )       (s.code_map [(addr) >> page_shift] [PAGE_OFFSET( addr )])

unsigned const z_flag = 0x80;
unsigned const n_flag = 0x40;
unsigned const h_flag = 0x20;
unsigned const c_flag = 0x10;

bool Gb_Cpu::run( blargg_long cycle_count )
{
	state_.remain = blargg_ulong (cycle_count + clocks_per_instr) / clocks_per_instr;
	state_t s;
	this->state = &s;
	memcpy( &s, &this->state_, sizeof s );
	
	typedef BOOST::uint16_t uint16_t;
	
#if BLARGG_BIG_ENDIAN
	#define R8( n ) (r8_ [n]) 
#elif BLARGG_LITTLE_ENDIAN
	#define R8( n ) (r8_ [(n) ^ 1]) 
#else
	#error "Byte order of CPU must be known"
#endif
	
	union {
		core_regs_t rg;  
		
		struct {
			BOOST::uint16_t bc, de, hl, unused;  
		} rp;
		
		uint8_t r8_ [8];  
		BOOST::uint16_t r16 [4];  
	};
	BOOST_STATIC_ASSERT( sizeof rg == 8 && sizeof rp == 8 );
	
	rg = r;
	unsigned pc = r.pc;
	unsigned sp = r.sp;
	unsigned flags = r.flags;
	
loop:
	
	check( (unsigned long) pc < 0x10000 );
	check( (unsigned long) sp < 0x10000 );
	check( (flags & ~0xF0) == 0 );
	
	uint8_t const* instr = s.code_map [pc >> page_shift];
	unsigned op;
	

	#if BLARGG_NONPORTABLE
		op = instr [pc];
		pc++;
		instr += pc;
	#else
		instr += PAGE_OFFSET( pc );
		op = *instr++;
		pc++;
	#endif
	
#define GET_ADDR()  GET_LE16( instr )
	
	if ( !--s.remain )
		goto stop;
	
	unsigned data;
	data = *instr;
	
	#ifdef GB_CPU_LOG_H
		gb_cpu_log( "new", pc - 1, op, data, instr [1] );
	#endif
	
	switch ( op )
	{


#define BRANCH( cond )\
{\
	pc++;\
	int offset = (BOOST::int8_t) data;\
	if ( !(cond) ) goto loop;\
	pc = uint16_t (pc + offset);\
	goto loop;\
}



	case 0x20:  
		BRANCH( !(flags & z_flag) )
	
	case 0x21:  
		rp.hl = GET_ADDR();
		pc += 2;
		goto loop;
	
	case 0x28:  
		BRANCH( flags & z_flag )
	
	{
		unsigned temp;
	case 0xF0:  
		temp = data | 0xFF00;
		pc++;
		goto ld_a_ind_comm;
	
	case 0xF2:  
		temp = rg.c | 0xFF00;
		goto ld_a_ind_comm;
	
	case 0x0A:  
		temp = rp.bc;
		goto ld_a_ind_comm;
	
	case 0x3A:  
		temp = rp.hl;
		rp.hl = temp - 1;
		goto ld_a_ind_comm;
	
	case 0x1A:  
		temp = rp.de;
		goto ld_a_ind_comm;
	
	case 0x2A:  
		temp = rp.hl;
		rp.hl = temp + 1;
		goto ld_a_ind_comm;
		
	case 0xFA:  
		temp = GET_ADDR();
		pc += 2;
	ld_a_ind_comm:
		READ_FAST( temp, rg.a );
		goto loop;
	}
	
	case 0xBE:  
		data = READ( rp.hl );
		goto cmp_comm;
	
	case 0xB8:  
	case 0xB9:  
	case 0xBA:  
	case 0xBB:  
	case 0xBC:  
	case 0xBD:  
		data = R8( op & 7 );
		goto cmp_comm;
	
	case 0xFE:  
		pc++;
	cmp_comm:
		op = rg.a;
		data = op - data;
	sub_set_flags:
		flags = ((op & 15) - (data & 15)) & h_flag;
		flags |= (data >> 4) & c_flag;
		flags |= n_flag;
		if ( data & 0xFF )
			goto loop;
		flags |= z_flag;
		goto loop;

	case 0x46:  
	case 0x4E:  
	case 0x56:  
	case 0x5E:  
	case 0x66:  
	case 0x6E:  
	case 0x7E:{ 
		unsigned addr = rp.hl;
		READ_FAST( addr, R8( (op >> 3) & 7 ) );
		goto loop;
	}
	
	case 0xC4:  
		pc += 2;
		if ( flags & z_flag )
			goto loop;
	call:
		pc -= 2;
	case 0xCD:  
		data = pc + 2;
		pc = GET_ADDR();
	push:
		sp = (sp - 1) & 0xFFFF;
		WRITE( sp, data >> 8 );
		sp = (sp - 1) & 0xFFFF;
		WRITE( sp, data & 0xFF );
		goto loop;
	
	case 0xC8:  
		if ( !(flags & z_flag) )
			goto loop;
	case 0xC9:  
	ret:
		pc = READ( sp );
		pc += 0x100 * READ( sp + 1 );
		sp = (sp + 2) & 0xFFFF;
		goto loop;
	
	case 0x00:  
	case 0x40:  
	case 0x49:  
	case 0x52:  
	case 0x5B:  
	case 0x64:  
	case 0x6D:  
	case 0x7F:  
		goto loop;
	


	case 0xCB:
		pc++;

		switch ( data ) {
			
		{
			int temp;
		case 0x46:  
		case 0x4E:
		case 0x56:
		case 0x5E:
		case 0x66:
		case 0x6E:
		case 0x76:
		case 0x7E:
			{
				unsigned addr = rp.hl;
				READ_FAST( addr, temp );
				goto bit_comm;
			}
		
		case 0x40: case 0x41: case 0x42: case 0x43:  
		case 0x44: case 0x45: case 0x47: case 0x48:
		case 0x49: case 0x4A: case 0x4B: case 0x4C:
		case 0x4D: case 0x4F: case 0x50: case 0x51:
		case 0x52: case 0x53: case 0x54: case 0x55:
		case 0x57: case 0x58: case 0x59: case 0x5A:
		case 0x5B: case 0x5C: case 0x5D: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63:
		case 0x64: case 0x65: case 0x67: case 0x68:
		case 0x69: case 0x6A: case 0x6B: case 0x6C:
		case 0x6D: case 0x6F: case 0x70: case 0x71:
		case 0x72: case 0x73: case 0x74: case 0x75:
		case 0x77: case 0x78: case 0x79: case 0x7A:
		case 0x7B: case 0x7C: case 0x7D: case 0x7F:
			temp = R8( data & 7 );
		bit_comm:
			int bit = (~data >> 3) & 7;
			flags &= ~n_flag;
			flags |= h_flag | z_flag;
			flags ^= (temp << bit) & z_flag;
			goto loop;
		}
		
		case 0x86:  
		case 0x8E:
		case 0x96:
		case 0x9E:
		case 0xA6:
		case 0xAE:
		case 0xB6:
		case 0xBE:
		case 0xC6:  
		case 0xCE:
		case 0xD6:
		case 0xDE:
		case 0xE6:
		case 0xEE:
		case 0xF6:
		case 0xFE: {
			int temp = READ( rp.hl );
			int bit = 1 << ((data >> 3) & 7);
			temp &= ~bit;
			if ( !(data & 0x40) )
				bit = 0;
			WRITE( rp.hl, temp | bit );
			goto loop;
		}
		
		case 0xC0: case 0xC1: case 0xC2: case 0xC3:  
		case 0xC4: case 0xC5: case 0xC7: case 0xC8:
		case 0xC9: case 0xCA: case 0xCB: case 0xCC:
		case 0xCD: case 0xCF: case 0xD0: case 0xD1:
		case 0xD2: case 0xD3: case 0xD4: case 0xD5:
		case 0xD7: case 0xD8: case 0xD9: case 0xDA:
		case 0xDB: case 0xDC: case 0xDD: case 0xDF:
		case 0xE0: case 0xE1: case 0xE2: case 0xE3:
		case 0xE4: case 0xE5: case 0xE7: case 0xE8:
		case 0xE9: case 0xEA: case 0xEB: case 0xEC:
		case 0xED: case 0xEF: case 0xF0: case 0xF1:
		case 0xF2: case 0xF3: case 0xF4: case 0xF5:
		case 0xF7: case 0xF8: case 0xF9: case 0xFA:
		case 0xFB: case 0xFC: case 0xFD: case 0xFF:
			R8( data & 7 ) |= 1 << ((data >> 3) & 7);
			goto loop;

		case 0x80: case 0x81: case 0x82: case 0x83:  
		case 0x84: case 0x85: case 0x87: case 0x88:
		case 0x89: case 0x8A: case 0x8B: case 0x8C:
		case 0x8D: case 0x8F: case 0x90: case 0x91:
		case 0x92: case 0x93: case 0x94: case 0x95:
		case 0x97: case 0x98: case 0x99: case 0x9A:
		case 0x9B: case 0x9C: case 0x9D: case 0x9F:
		case 0xA0: case 0xA1: case 0xA2: case 0xA3:
		case 0xA4: case 0xA5: case 0xA7: case 0xA8:
		case 0xA9: case 0xAA: case 0xAB: case 0xAC:
		case 0xAD: case 0xAF: case 0xB0: case 0xB1:
		case 0xB2: case 0xB3: case 0xB4: case 0xB5:
		case 0xB7: case 0xB8: case 0xB9: case 0xBA:
		case 0xBB: case 0xBC: case 0xBD: case 0xBF:
			R8( data & 7 ) &= ~(1 << ((data >> 3) & 7));
			goto loop;
		
		{
			int temp;
		case 0x36:  
			temp = READ( rp.hl );
			goto swap_comm;
		
		case 0x30:  
		case 0x31:  
		case 0x32:  
		case 0x33:  
		case 0x34:  
		case 0x35:  
		case 0x37:  
			temp = R8( data & 7 );
		swap_comm:
			op = (temp >> 4) | (temp << 4);
			flags = 0;
			goto shift_comm;
		}
		


		case 0x06:  
		case 0x16:  
		case 0x26:  
			op = READ( rp.hl );
			goto rl_comm;
		
		case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x27:  
		case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07:  
		case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17:  
			op = R8( data & 7 );
			goto rl_comm;
		
		case 0x3E:  
			data += 0x10;  
		case 0x1E:  
		case 0x0E:  
		case 0x2E:  
			op = READ( rp.hl );
			goto rr_comm;
		
		case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3F:  
			data += 0x10;  
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F:  
		case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F:  
		case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2F:  
			op = R8( data & 7 );
			goto rr_comm;
		
	}  
	assert( false );  

	case 0x07:  
	case 0x17:  
		data = op;
		op = rg.a;
	rl_comm:
		op <<= 1;
		op |= ((data & flags) >> 4) & 1;  
		flags = (op >> 4) & c_flag;  
		if ( data < 0x10 )  
			op |= op >> 8;

		goto shift_comm;
	
	case 0x0F:  
	case 0x1F:  
		data = op;
		op = rg.a;
	rr_comm:
		op |= (data & flags) << 4;  
		flags = (op << 4) & c_flag;  
		if ( data < 0x10 )  
			op |= op << 8;
		op >>= 1;
		if ( data & 0x20 )  
			op |= (op << 1) & 0x80;
	shift_comm:
		data &= 7;
		if ( !(op & 0xFF) )
			flags |= z_flag;
		if ( data == 6 )
			goto write_hl_op_ff;
		R8( data ) = op;
		goto loop;



	case 0x70:  
	case 0x71:  
	case 0x72:  
	case 0x73:  
	case 0x74:  
	case 0x75:  
	case 0x77:  
		op = R8( op & 7 );
	write_hl_op_ff:
		WRITE( rp.hl, op & 0xFF );
		goto loop;

	case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:  
	case 0x48: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F:
	case 0x50: case 0x51: case 0x53: case 0x54: case 0x55: case 0x57:
	case 0x58: case 0x59: case 0x5A: case 0x5C: case 0x5D: case 0x5F:
	case 0x60: case 0x61: case 0x62: case 0x63: case 0x65: case 0x67:
	case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6F:
	case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D:
		R8( (op >> 3) & 7 ) = R8( op & 7 );
		goto loop;

	case 0x08:  
		data = GET_ADDR();
		pc += 2;
		WRITE( data, sp&0xFF );
		data++;
		WRITE( data, sp >> 8 );
		goto loop;
	
	case 0xF9:  
		sp = rp.hl;
		goto loop;

	case 0x31:  
		sp = GET_ADDR();
		pc += 2;
		goto loop;
	
	case 0x01:  
	case 0x11:  
		r16 [op >> 4] = GET_ADDR();
		pc += 2;
		goto loop;
	
	{
		unsigned temp;
	case 0xE0:  
		temp = data | 0xFF00;
		pc++;
		goto write_data_rg_a;
	
	case 0xE2:  
		temp = rg.c | 0xFF00;
		goto write_data_rg_a;

	case 0x32:  
		temp = rp.hl;
		rp.hl = temp - 1;
		goto write_data_rg_a;
	
	case 0x02:  
		temp = rp.bc;
		goto write_data_rg_a;
	
	case 0x12:  
		temp = rp.de;
		goto write_data_rg_a;
	
	case 0x22:  
		temp = rp.hl;
		rp.hl = temp + 1;
		goto write_data_rg_a;
		
	case 0xEA:  
		temp = GET_ADDR();
		pc += 2;
	write_data_rg_a:
		WRITE( temp, rg.a );
		goto loop;
	}
	
	case 0x06:  
		rg.b = data;
		pc++;
		goto loop;
	
	case 0x0E:  
		rg.c = data;
		pc++;
		goto loop;
	
	case 0x16:  
		rg.d = data;
		pc++;
		goto loop;
	
	case 0x1E:  
		rg.e = data;
		pc++;
		goto loop;
	
	case 0x26:  
		rg.h = data;
		pc++;
		goto loop;
	
	case 0x2E:  
		rg.l = data;
		pc++;
		goto loop;
	
	case 0x36:  
		WRITE( rp.hl, data );
		pc++;
		goto loop;
	
	case 0x3E:  
		rg.a = data;
		pc++;
		goto loop;



	case 0x03:  
	case 0x13:  
	case 0x23:  
		r16 [op >> 4]++;
		goto loop;
	
	case 0x33:  
		sp = (sp + 1) & 0xFFFF;
		goto loop;

	case 0x0B:  
	case 0x1B:  
	case 0x2B:  
		r16 [op >> 4]--;
		goto loop;
	
	case 0x3B:  
		sp = (sp - 1) & 0xFFFF;
		goto loop;
	
	case 0x34:  
		op = rp.hl;
		data = READ( op );
		data++;
		WRITE( op, data & 0xFF );
		goto inc_comm;
	
	case 0x04:  
	case 0x0C:  
	case 0x14:  
	case 0x1C:  
	case 0x24:  
	case 0x2C:  
	case 0x3C:  
		op = (op >> 3) & 7;
		R8( op ) = data = R8( op ) + 1;
	inc_comm:
		flags = (flags & c_flag) | (((data & 15) - 1) & h_flag) | ((data >> 1) & z_flag);
		goto loop;
	
	case 0x35:  
		op = rp.hl;
		data = READ( op );
		data--;
		WRITE( op, data & 0xFF );
		goto dec_comm;
	
	case 0x05:  
	case 0x0D:  
	case 0x15:  
	case 0x1D:  
	case 0x25:  
	case 0x2D:  
	case 0x3D:  
		op = (op >> 3) & 7;
		data = R8( op ) - 1;
		R8( op ) = data;
	dec_comm:
		flags = (flags & c_flag) | n_flag | (((data & 15) + 0x31) & h_flag);
		if ( data & 0xFF )
			goto loop;
		flags |= z_flag;
		goto loop;



	{
		blargg_ulong temp;  
		unsigned prev;
		
	case 0xF8:  
		temp = BOOST::int8_t (data);  
		pc++;
		flags = 0;
		temp += sp;
		prev = sp;
		goto add_16_hl;
	
	case 0xE8:  
		temp = BOOST::int8_t (data);  
		pc++;
		flags = 0;
		temp += sp;
		prev = sp;
		sp = temp & 0xFFFF;
		goto add_16_comm;

	case 0x39:  
		temp = sp;
		goto add_hl_comm;
	
	case 0x09:  
	case 0x19:  
	case 0x29:  
		temp = r16 [op >> 4];
	add_hl_comm:
		prev = rp.hl;
		temp += prev;
		flags &= z_flag;
	add_16_hl:
		rp.hl = temp;
	add_16_comm:
		flags |= (temp >> 12) & c_flag;
		flags |= (((temp & 0x0FFF) - (prev & 0x0FFF)) >> 7) & h_flag;
		goto loop;
	}
	
	case 0x86:  
		data = READ( rp.hl );
		goto add_comm;
	
	case 0x80:  
	case 0x81:  
	case 0x82:  
	case 0x83:  
	case 0x84:  
	case 0x85:  
	case 0x87:  
		data = R8( op & 7 );
		goto add_comm;
	
	case 0xC6:  
		pc++;
	add_comm:
		flags = rg.a;
		data += flags;
		flags = ((data & 15) - (flags & 15)) & h_flag;
		flags |= (data >> 4) & c_flag;
		rg.a = data;
		if ( data & 0xFF )
			goto loop;
		flags |= z_flag;
		goto loop;



	case 0x8E:  
		data = READ( rp.hl );
		goto adc_comm;
	
	case 0x88:  
	case 0x89:  
	case 0x8A:  
	case 0x8B:  
	case 0x8C:  
	case 0x8D:  
	case 0x8F:  
		data = R8( op & 7 );
		goto adc_comm;
	
	case 0xCE:  
		pc++;
	adc_comm:
		data += (flags >> 4) & 1;
		data &= 0xFF;  
		goto add_comm;

	case 0x96:  
		data = READ( rp.hl );
		goto sub_comm;
	
	case 0x90:  
	case 0x91:  
	case 0x92:  
	case 0x93:  
	case 0x94:  
	case 0x95:  
	case 0x97:  
		data = R8( op & 7 );
		goto sub_comm;
	
	case 0xD6:  
		pc++;
	sub_comm:
		op = rg.a;
		data = op - data;
		rg.a = data;
		goto sub_set_flags;

	case 0x9E:  
		data = READ( rp.hl );
		goto sbc_comm;
	
	case 0x98:  
	case 0x99:  
	case 0x9A:  
	case 0x9B:  
	case 0x9C:  
	case 0x9D:  
	case 0x9F:  
		data = R8( op & 7 );
		goto sbc_comm;
	
	case 0xDE:  
		pc++;
	sbc_comm:
		data += (flags >> 4) & 1;
		data &= 0xFF;  
		goto sub_comm;



	case 0xA0:  
	case 0xA1:  
	case 0xA2:  
	case 0xA3:  
	case 0xA4:  
	case 0xA5:  
		data = R8( op & 7 );
		goto and_comm;
	
	case 0xA6:  
		data = READ( rp.hl );
		pc--;
	case 0xE6:  
		pc++;
	and_comm:
		rg.a &= data;
	case 0xA7:  
		flags = h_flag | (((rg.a - 1) >> 1) & z_flag);
		goto loop;

	case 0xB0:  
	case 0xB1:  
	case 0xB2:  
	case 0xB3:  
	case 0xB4:  
	case 0xB5:  
		data = R8( op & 7 );
		goto or_comm;
	
	case 0xB6:  
		data = READ( rp.hl );
		pc--;
	case 0xF6:  
		pc++;
	or_comm:
		rg.a |= data;
	case 0xB7:  
		flags = ((rg.a - 1) >> 1) & z_flag;
		goto loop;

	case 0xA8:  
	case 0xA9:  
	case 0xAA:  
	case 0xAB:  
	case 0xAC:  
	case 0xAD:  
		data = R8( op & 7 );
		goto xor_comm;
	
	case 0xAE:  
		data = READ( rp.hl );
		pc--;
	case 0xEE:  
		pc++;
	xor_comm:
		data ^= rg.a;
		rg.a = data;
		data--;
		flags = (data >> 1) & z_flag;
		goto loop;
	
	case 0xAF:  
		rg.a = 0;
		flags = z_flag;
		goto loop;



	case 0xF1:  
	case 0xC1:  
	case 0xD1:  
	case 0xE1:  
		data = READ( sp );
		r16 [(op >> 4) & 3] = data + 0x100 * READ( sp + 1 );
		sp = (sp + 2) & 0xFFFF;
		if ( op != 0xF1 )
			goto loop;
		flags = rg.flags & 0xF0;
		goto loop;
	
	case 0xC5:  
		data = rp.bc;
		goto push;
	
	case 0xD5:  
		data = rp.de;
		goto push;
	
	case 0xE5:  
		data = rp.hl;
		goto push;
	
	case 0xF5:  
		data = (flags << 8) | rg.a;
		goto push;


	
	case 0xFF:
		if ( pc == idle_addr + 1 )
			goto stop;
	case 0xC7: case 0xCF: case 0xD7: case 0xDF:   
	case 0xE7: case 0xEF: case 0xF7:
		data = pc;
		pc = (op & 0x38) + rst_base;
		goto push;
	
	case 0xCC:  
		pc += 2;
		if ( flags & z_flag )
			goto call;
		goto loop;
	
	case 0xD4:  
		pc += 2;
		if ( !(flags & c_flag) )
			goto call;
		goto loop;
	
	case 0xDC:  
		pc += 2;
		if ( flags & c_flag )
			goto call;
		goto loop;

	case 0xD9:  

		goto ret;
	
	case 0xC0:  
		if ( !(flags & z_flag) )
			goto ret;
		goto loop;
	
	case 0xD0:  
		if ( !(flags & c_flag) )
			goto ret;
		goto loop;
	
	case 0xD8:  
		if ( flags & c_flag )
			goto ret;
		goto loop;

	case 0x18:  
		BRANCH( true )
	
	case 0x30:  
		BRANCH( !(flags & c_flag) )
	
	case 0x38:  
		BRANCH( flags & c_flag )
	
	case 0xE9:  
		pc = rp.hl;
		goto loop;

	case 0xC3:  
		pc = GET_ADDR();
		goto loop;
	
	case 0xC2:  
		pc += 2;
		if ( !(flags & z_flag) )
			goto jp_taken;
		goto loop;
	
	case 0xCA:  
		pc += 2;
		if ( !(flags & z_flag) )
			goto loop;
	jp_taken:
		pc -= 2;
		pc = GET_ADDR();
		goto loop;
	
	case 0xD2:  
		pc += 2;
		if ( !(flags & c_flag) )
			goto jp_taken;
		goto loop;
	
	case 0xDA:  
		pc += 2;
		if ( flags & c_flag )
			goto jp_taken;
		goto loop;



	case 0x2F:  
		rg.a = ~rg.a;
		flags |= n_flag | h_flag;
		goto loop;

	case 0x3F:  
		flags = (flags ^ c_flag) & ~(n_flag | h_flag);
		goto loop;

	case 0x37:  
		flags = (flags | c_flag) & ~(n_flag | h_flag);
		goto loop;

	case 0xF3:  

		goto loop;

	case 0xFB:  

		goto loop;



	case 0xDD: case 0xD3: case 0xDB: case 0xE3: case 0xE4:  
	case 0xEB: case 0xEC: case 0xF4: case 0xFD: case 0xFC:
	case 0x10:  
	case 0x27:  
	case 0xBF:
	case 0xED:  
	case 0x76:  
		s.remain++;
		goto stop;
	}
	

	assert( false );
	
stop:
	pc--;
	

	STATIC_CAST(core_regs_t&,r) = rg;
	r.pc = pc;
	r.sp = sp;
	r.flags = flags;
	
	this->state = &state_;
	memcpy( &this->state_, &s, sizeof this->state_ );
	
	return s.remain > 0;
}
