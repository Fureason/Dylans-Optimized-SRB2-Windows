

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



#if SPC_MORE_ACCURACY
	#define SUSPICIOUS_OPCODE( name ) ((void) 0)
#else
	#define SUSPICIOUS_OPCODE( name ) debug_printf( "SPC: suspicious opcode: " name "\n" )
#endif

#define CPU_READ( time, offset, addr )\
	cpu_read( addr, time + offset )

#define CPU_WRITE( time, offset, addr, data )\
	cpu_write( data, addr, time + offset )

#if SPC_MORE_ACCURACY
	#define CPU_READ_TIMER( time, offset, addr, out )\
		{ out = CPU_READ( time, offset, addr ); }

#else

	#define CPU_READ_TIMER( time, offset, addr_, out )\
	{\
		rel_time_t adj_time = time + offset;\
		int dp_addr = addr_;\
		int ti = dp_addr - (r_t0out + 0xF0);\
		if ( (unsigned) ti < timer_count )\
		{\
			Timer* t = &m.timers [ti];\
			if ( adj_time >= t->next_time )\
				t = run_timer_( t, adj_time );\
			out = t->counter;\
			t->counter = 0;\
		}\
		else\
		{\
			out = ram [dp_addr];\
			int i = dp_addr - 0xF0;\
			if ( (unsigned) i < 0x10 )\
				out = cpu_read_smp_reg( i, adj_time );\
		}\
	}
#endif

#define TIME_ADJ( n )   (n)

#define READ_TIMER( time, addr, out )       CPU_READ_TIMER( rel_time, TIME_ADJ(time), (addr), out )
#define READ(  time, addr )                 CPU_READ ( rel_time, TIME_ADJ(time), (addr) )
#define WRITE( time, addr, data )           CPU_WRITE( rel_time, TIME_ADJ(time), (addr), (data) )

#define DP_ADDR( addr )                     (dp + (addr))

#define READ_DP_TIMER(  time, addr, out )   CPU_READ_TIMER( rel_time, TIME_ADJ(time), DP_ADDR( addr ), out )
#define READ_DP(  time, addr )              READ ( time, DP_ADDR( addr ) )
#define WRITE_DP( time, addr, data )        WRITE( time, DP_ADDR( addr ), data )

#define READ_PROG16( addr )                 GET_LE16( ram + (addr) )

#define SET_PC( n )     (pc = ram + (n))
#define GET_PC()        (pc - ram)
#define READ_PC( pc )   (*(pc))
#define READ_PC16( pc ) GET_LE16( pc )


#define SPC_NO_SP_WRAPAROUND 0

#define SET_SP( v )     (sp = ram + 0x101 + ((uint8_t) v))
#define GET_SP()        (uint8_t (sp - 0x101 - ram))

#if SPC_NO_SP_WRAPAROUND
#define PUSH16( v )     (sp -= 2, SET_LE16( sp, v ))
#define PUSH( v )       (void) (*--sp = (uint8_t) (v))
#define POP( out )      (void) ((out) = *sp++)

#else
#define PUSH16( data )\
{\
	int addr = (sp -= 2) - ram;\
	if ( addr > 0x100 )\
	{\
		SET_LE16( sp, data );\
	}\
	else\
	{\
		ram [(uint8_t) addr + 0x100] = (uint8_t) data;\
		sp [1] = (uint8_t) (data >> 8);\
		sp += 0x100;\
	}\
}

#define PUSH( data )\
{\
	*--sp = (uint8_t) (data);\
	if ( sp - ram == 0x100 )\
		sp += 0x100;\
}

#define POP( out )\
{\
	out = *sp++;\
	if ( sp - ram == 0x201 )\
	{\
		out = sp [-0x101];\
		sp -= 0x100;\
	}\
}

#endif

#define MEM_BIT( rel ) CPU_mem_bit( pc, rel_time + rel )

unsigned Snes_Spc::CPU_mem_bit( uint8_t const* pc, rel_time_t rel_time )
{
	unsigned addr = READ_PC16( pc );
	unsigned t = READ( 0, addr & 0x1FFF ) >> (addr >> 13);
	return t << 8 & 0x100;
}





int const n80 = 0x80;  
int const v40 = 0x40;  
int const p20 = 0x20;  
int const b10 = 0x10;  
int const h08 = 0x08;  
int const i04 = 0x04;  
int const z02 = 0x02;  
int const c01 = 0x01;  

int const nz_neg_mask = 0x880;  

#define GET_PSW( out )\
{\
	out = psw & ~(n80 | p20 | z02 | c01);\
	out |= c  >> 8 & c01;\
	out |= dp >> 3 & p20;\
	out |= ((nz >> 4) | nz) & n80;\
	if ( !(uint8_t) nz ) out |= z02;\
}

#define SET_PSW( in )\
{\
	psw = in;\
	c   = in << 8;\
	dp  = in << 3 & 0x100;\
	nz  = (in << 4 & 0x800) | (~in & z02);\
}

SPC_CPU_RUN_FUNC
{
	uint8_t* const ram = RAM;
	int a = m.cpu_regs.a;
	int x = m.cpu_regs.x;
	int y = m.cpu_regs.y;
	uint8_t const* pc;
	uint8_t* sp;
	int psw;
	int c;
	int nz;
	int dp;
	
	SET_PC( m.cpu_regs.pc );
	SET_SP( m.cpu_regs.sp );
	SET_PSW( m.cpu_regs.psw );
	
	goto loop;
	
	

	
cbranch_taken_loop:
	pc += *(BOOST::int8_t const*) pc;
inc_pc_loop:
	pc++;
loop:
{
	unsigned opcode;
	unsigned data;
	
	check( (unsigned) a < 0x100 );
	check( (unsigned) x < 0x100 );
	check( (unsigned) y < 0x100 );
	
	opcode = *pc;
	if ( (rel_time += m.cycle_table [opcode]) > 0 )
		goto out_of_time;
	
	#ifdef SPC_CPU_OPCODE_HOOK
		SPC_CPU_OPCODE_HOOK( GET_PC(), opcode );
	#endif
	  
                         
                                            \
                    \
  \
                                              \
                                                 \
                                 \
  
 
                                                   
                                       
                                       
   
	

	data = *++pc;
	switch ( opcode )
	{
	


#define BRANCH( cond )\
{\
	pc++;\
	pc += (BOOST::int8_t) data;\
	if ( cond )\
		goto loop;\
	pc -= (BOOST::int8_t) data;\
	rel_time -= 2;\
	goto loop;\
}

	case 0xF0:  
		BRANCH( !(uint8_t) nz )  
	
	case 0xD0:  
		BRANCH( (uint8_t) nz )
	
	case 0x3F:{ 
		int old_addr = GET_PC() + 2;
		SET_PC( READ_PC16( pc ) );
		PUSH16( old_addr );
		goto loop;
	}
	
	case 0x6F: 
		#if SPC_NO_SP_WRAPAROUND
		{
			SET_PC( GET_LE16( sp ) );
			sp += 2;
		}
		#else
		{
			int addr = sp - ram;
			SET_PC( GET_LE16( sp ) );
			sp += 2;
			if ( addr < 0x1FF )
				goto loop;
			
			SET_PC( sp [-0x101] * 0x100 + ram [(uint8_t) addr + 0x100] );
			sp -= 0x100;
		}
		#endif
		goto loop;
	
	case 0xE4:  
		++pc;

		READ_DP_TIMER( 0, data, a = nz );
		goto loop;
	
	case 0xFA:{ 
		int temp;
		READ_DP_TIMER( -2, data, temp );
		data = temp + no_read_before_write ;
	}

	case 0x8F:{ 
		int temp = READ_PC( pc + 1 );
		pc += 2;
		
		#if !SPC_MORE_ACCURACY
		{
			int i = dp + temp;
			ram [i] = (uint8_t) data;
			i -= 0xF0;
			if ( (unsigned) i < 0x10 )  
			{
				REGS [i] = (uint8_t) data;
				


				if ( ((~0x2F00 << (bits_in_int - 16)) << i) < 0 )  
					cpu_write_smp_reg( data, rel_time, i );
			}
		}
		#else
			WRITE_DP( 0, temp, data );
		#endif
		goto loop;
	}
	
	case 0xC4:  
		++pc;
		#if !SPC_MORE_ACCURACY
		{
			int i = dp + data;
			ram [i] = (uint8_t) a;
			i -= 0xF0;
			if ( (unsigned) i < 0x10 )  
			{
				unsigned sel = i - 2;
				REGS [i] = (uint8_t) a;
				
				if ( sel == 1 )  
					dsp_write( a, rel_time );
				else if ( sel > 1 )  
					cpu_write_smp_reg_( a, rel_time, i );
			}
		}
		#else
			WRITE_DP( 0, data, a );
		#endif
		goto loop;
	
#define CASE( n )   case n:



#define ADDR_MODES_( op )\
	CASE( op - 0x02 )  \
		data = x + dp;\
		pc--;\
		goto end_##op;\
	CASE( op + 0x0F )  \
		data = READ_PROG16( data + dp ) + y;\
		goto end_##op;\
	CASE( op - 0x01 )  \
		data = READ_PROG16( ((uint8_t) (data + x)) + dp );\
		goto end_##op;\
	CASE( op + 0x0E )  \
		data += y;\
		goto abs_##op;\
	CASE( op + 0x0D )  \
		data += x;\
	CASE( op - 0x03 )  \
	abs_##op:\
		data += 0x100 * READ_PC( ++pc );\
		goto end_##op;\
	CASE( op + 0x0C )  \
		data = (uint8_t) (data + x);

#define ADDR_MODES_NO_DP( op )\
	ADDR_MODES_( op )\
		data += dp;\
	end_##op:

#define ADDR_MODES( op )\
	ADDR_MODES_( op )\
	CASE( op - 0x04 )  \
		data += dp;\
	end_##op:



	ADDR_MODES_NO_DP( 0xE8 )  
		a = nz = READ( 0, data );
		goto inc_pc_loop;
	
	case 0xBF:{ 
		int temp = x + dp;
		x = (uint8_t) (x + 1);
		a = nz = READ( -1, temp );
		goto loop;
	}
	
	case 0xE8:  
		a  = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xF9:  
		data = (uint8_t) (data + y);
	case 0xF8:  
		READ_DP_TIMER( 0, data, x = nz );
		goto inc_pc_loop;
	
	case 0xE9:  
		data = READ_PC16( pc );
		++pc;
		data = READ( 0, data );
	case 0xCD:  
		x  = data;
		nz = data;
		goto inc_pc_loop;
	
	case 0xFB:  
		data = (uint8_t) (data + x);
	case 0xEB:  

		pc++;
		READ_DP_TIMER( 0, data, y = nz );
		goto loop;
	
	case 0xEC:{ 
		int temp = READ_PC16( pc );
		pc += 2;
		READ_TIMER( 0, temp, y = nz );

		goto loop;
	}
	
	case 0x8D:  
		y  = data;
		nz = data;
		goto inc_pc_loop;
	


	ADDR_MODES_NO_DP( 0xC8 )  
		WRITE( 0, data, a );
		goto inc_pc_loop;
	
	{
		int temp;
	case 0xCC:  
		temp = y;
		goto mov_abs_temp;
	case 0xC9:  
		temp = x;
	mov_abs_temp:
		WRITE( 0, READ_PC16( pc ), temp );
		pc += 2;
		goto loop;
	}
	
	case 0xD9:  
		data = (uint8_t) (data + y);
	case 0xD8:  
		WRITE( 0, data + dp, x );
		goto inc_pc_loop;
	
	case 0xDB:  
		data = (uint8_t) (data + x);
	case 0xCB:  
		WRITE( 0, data + dp, y );
		goto inc_pc_loop;


	
	case 0x7D:  
		a  = x;
		nz = x;
		goto loop;
	
	case 0xDD:  
		a  = y;
		nz = y;
		goto loop;
	
	case 0x5D:  
		x  = a;
		nz = a;
		goto loop;
	
	case 0xFD:  
		y  = a;
		nz = a;
		goto loop;
	
	case 0x9D:  
		x = nz = GET_SP();
		goto loop;
	
	case 0xBD:  
		SET_SP( x );
		goto loop;
	

	
	case 0xAF:  
		WRITE_DP( 0, x, a + no_read_before_write  );
		x = (uint8_t) (x + 1);
		goto loop;
	

	
#define LOGICAL_OP( op, func )\
	ADDR_MODES( op )  \
		data = READ( 0, data );\
	case op:  \
		nz = a func##= data;\
		goto inc_pc_loop;\
	{   unsigned addr;\
	case op + 0x11:  \
		data = READ_DP( -2, y );\
		addr = x + dp;\
		goto addr_##op;\
	case op + 0x01:  \
		data = READ_DP( -3, data );\
	case op + 0x10:{ \
		uint8_t const* addr2 = pc + 1;\
		pc += 2;\
		addr = READ_PC( addr2 ) + dp;\
	}\
	addr_##op:\
		nz = data func READ( -1, addr );\
		WRITE( 0, addr, nz );\
		goto loop;\
	}
	
	LOGICAL_OP( 0x28, & );  
	
	LOGICAL_OP( 0x08, | );  
	
	LOGICAL_OP( 0x48, ^ );  
	


	ADDR_MODES( 0x68 )  
		data = READ( 0, data );
	case 0x68:  
		nz = a - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x79:  
		data = READ_DP( -2, y );
		nz = READ_DP( -1, x ) - data;
		c = ~nz;
		nz &= 0xFF;
		goto loop;
	
	case 0x69:  
		data = READ_DP( -3, data );
	case 0x78:  
		nz = READ_DP( -1, READ_PC( ++pc ) ) - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x3E:  
		data += dp;
		goto cmp_x_addr;
	case 0x1E:  
		data = READ_PC16( pc );
		pc++;
	cmp_x_addr:
		data = READ( 0, data );
	case 0xC8:  
		nz = x - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	case 0x7E:  
		data += dp;
		goto cmp_y_addr;
	case 0x5E:  
		data = READ_PC16( pc );
		pc++;
	cmp_y_addr:
		data = READ( 0, data );
	case 0xAD:  
		nz = y - data;
		c = ~nz;
		nz &= 0xFF;
		goto inc_pc_loop;
	
	{
		int addr;
	case 0xB9:  
	case 0x99:  
		pc--;  
		data = READ_DP( -2, y );
		addr = x + dp;
		goto adc_addr;
	case 0xA9:  
	case 0x89:  
		data = READ_DP( -3, data );
	case 0xB8:  
	case 0x98:  
		addr = READ_PC( ++pc ) + dp;
	adc_addr:
		nz = READ( -1, addr );
		goto adc_data;
		

#undef CASE
#define CASE( n ) case n: case (n) + 0x20:
	ADDR_MODES( 0x88 )  
		data = READ( 0, data );
	case 0xA8:  
	case 0x88:  
		addr = -1;  
		nz = a;
	adc_data: {
		int flags;
		if ( opcode >= 0xA0 )  
			data ^= 0xFF;
		
		flags = data ^ nz;
		nz += data + (c >> 8 & 1);
		flags ^= nz;
		
		psw = (psw & ~(v40 | h08)) |
				(flags >> 1 & h08) |
				((flags + 0x80) >> 2 & v40);
		c = nz;
		if ( addr < 0 )
		{
			a = (uint8_t) nz;
			goto inc_pc_loop;
		}
		WRITE( 0, addr,   nz );
		goto inc_pc_loop;
	}
	
	}
	


#define INC_DEC_REG( reg, op )\
		nz  = reg op;\
		reg = (uint8_t) nz;\
		goto loop;

	case 0xBC: INC_DEC_REG( a, + 1 )  
	case 0x3D: INC_DEC_REG( x, + 1 )  
	case 0xFC: INC_DEC_REG( y, + 1 )  
	
	case 0x9C: INC_DEC_REG( a, - 1 )  
	case 0x1D: INC_DEC_REG( x, - 1 )  
	case 0xDC: INC_DEC_REG( y, - 1 )  

	case 0x9B:  
	case 0xBB:  
		data = (uint8_t) (data + x);
	case 0x8B:  
	case 0xAB:  
		data += dp;
		goto inc_abs;
	case 0x8C:  
	case 0xAC:  
		data = READ_PC16( pc );
		pc++;
	inc_abs:
		nz = (opcode >> 4 & 2) - 1;
		nz += READ( -1, data );
		WRITE( 0, data,   nz );
		goto inc_pc_loop;
	


	case 0x5C:  
		c = 0;
	case 0x7C:{ 
		nz = (c >> 1 & 0x80) | (a >> 1);
		c = a << 8;
		a = nz;
		goto loop;
	}
	
	case 0x1C:  
		c = 0;
	case 0x3C:{ 
		int temp = c >> 8 & 1;
		c = a << 1;
		nz = c | temp;
		a = (uint8_t) nz;
		goto loop;
	}
	
	case 0x0B:  
		c = 0;
		data += dp;
		goto rol_mem;
	case 0x1B:  
		c = 0;
	case 0x3B:  
		data = (uint8_t) (data + x);
	case 0x2B:  
		data += dp;
		goto rol_mem;
	case 0x0C:  
		c = 0;
	case 0x2C:  
		data = READ_PC16( pc );
		pc++;
	rol_mem:
		nz = c >> 8 & 1;
		nz |= (c = READ( -1, data ) << 1);
		WRITE( 0, data,   nz );
		goto inc_pc_loop;
	
	case 0x4B:  
		c = 0;
		data += dp;
		goto ror_mem;
	case 0x5B:  
		c = 0;
	case 0x7B:  
		data = (uint8_t) (data + x);
	case 0x6B:  
		data += dp;
		goto ror_mem;
	case 0x4C:  
		c = 0;
	case 0x6C:  
		data = READ_PC16( pc );
		pc++;
	ror_mem: {
		int temp = READ( -1, data );
		nz = (c >> 1 & 0x80) | (temp >> 1);
		c = temp << 8;
		WRITE( 0, data, nz );
		goto inc_pc_loop;
	}

	case 0x9F:  
		nz = a = (a >> 4) | (uint8_t) (a << 4);
		goto loop;



	case 0xBA:  
		a = READ_DP( -2, data );
		nz = (a & 0x7F) | (a >> 1);
		y = READ_DP( 0, (uint8_t) (data + 1) );
		nz |= y;
		goto inc_pc_loop;
	
	case 0xDA:  
		WRITE_DP( -1, data, a );
		WRITE_DP( 0, (uint8_t) (data + 1), y + no_read_before_write  );
		goto inc_pc_loop;
	


	case 0x3A:  
	case 0x1A:{ 
		int temp;

		data += dp;
		temp = READ( -3, data );
		temp += (opcode >> 4 & 2) - 1;  
		nz = ((temp >> 1) | temp) & 0x7F;
		WRITE( -2, data,   temp );
		

		data = (uint8_t) (data + 1) + dp;
		temp = (uint8_t) ((temp >> 8) + READ( -1, data ));
		nz |= temp;
		WRITE( 0, data, temp );
		
		goto inc_pc_loop;
	}
		
	case 0x7A:  
	case 0x9A:{ 
		int lo = READ_DP( -2, data );
		int hi = READ_DP( 0, (uint8_t) (data + 1) );
		int result;
		int flags;
		
		if ( opcode == 0x9A )  
		{
			lo = (lo ^ 0xFF) + 1;
			hi ^= 0xFF;
		}
		
		lo += a;
		result = y + hi + (lo >> 8);
		flags = hi ^ y ^ result;
		
		psw = (psw & ~(v40 | h08)) |
				(flags >> 1 & h08) |
				((flags + 0x80) >> 2 & v40);
		c = result;
		a = (uint8_t) lo;
		result = (uint8_t) result;
		y = result;
		nz = (((lo >> 1) | lo) & 0x7F) | result;
		
		goto inc_pc_loop;
	}
	
	case 0x5A: {  
		int temp = a - READ_DP( -1, data );
		nz = ((temp >> 1) | temp) & 0x7F;
		temp = y + (temp >> 8);
		temp -= READ_DP( 0, (uint8_t) (data + 1) );
		nz |= temp;
		c  = ~temp;
		nz &= 0xFF;
		goto inc_pc_loop;
	}
	


	case 0xCF: {  
		unsigned temp = y * a;
		a = (uint8_t) temp;
		nz = ((temp >> 1) | temp) & 0x7F;
		y = (uint8_t) (temp >> 8);
		nz |= y;
		goto loop;
	}
	
	case 0x9E:  
	{
		unsigned ya = y * 0x100 + a;
		
		psw &= ~(h08 | v40);
		
		if ( y >= x )
			psw |= v40;
		
		if ( (y & 15) >= (x & 15) )
			psw |= h08;
		
		if ( y < x * 2 )
		{
			a = ya / x;
			y = ya - a * x;
		}
		else
		{
			a = 255 - (ya - x * 0x200) / (256 - x);
			y = x   + (ya - x * 0x200) % (256 - x);
		}
		
		nz = (uint8_t) a;
		a = (uint8_t) a;
		y = (uint8_t) y;
		
		goto loop;
	}
	

	
	case 0xDF:  
		SUSPICIOUS_OPCODE( "DAA" );
		if ( a > 0x99 || c & 0x100 )
		{
			a += 0x60;
			c = 0x100;
		}
		
		if ( (a & 0x0F) > 9 || psw & h08 )
			a += 0x06;
		
		nz = a;
		a = (uint8_t) a;
		goto loop;
	
	case 0xBE:  
		SUSPICIOUS_OPCODE( "DAS" );
		if ( a > 0x99 || !(c & 0x100) )
		{
			a -= 0x60;
			c = 0;
		}
		
		if ( (a & 0x0F) > 9 || !(psw & h08) )
			a -= 0x06;
		
		nz = a;
		a = (uint8_t) a;
		goto loop;
	


	case 0x2F:  
		pc += (BOOST::int8_t) data;
		goto inc_pc_loop;
	
	case 0x30:  
		BRANCH( (nz & nz_neg_mask) )
	
	case 0x10:  
		BRANCH( !(nz & nz_neg_mask) )
	
	case 0xB0:  
		BRANCH( c & 0x100 )
	
	case 0x90:  
		BRANCH( !(c & 0x100) )
	
	case 0x70:  
		BRANCH( psw & v40 )
	
	case 0x50:  
		BRANCH( !(psw & v40) )
	
	#define CBRANCH( cond )\
	{\
		pc++;\
		if ( cond )\
			goto cbranch_taken_loop;\
		rel_time -= 2;\
		goto inc_pc_loop;\
	}
	
	case 0x03:  
	case 0x23:
	case 0x43:
	case 0x63:
	case 0x83:
	case 0xA3:
	case 0xC3:
	case 0xE3:
		CBRANCH( READ_DP( -4, data ) >> (opcode >> 5) & 1 )
	
	case 0x13:  
	case 0x33:
	case 0x53:
	case 0x73:
	case 0x93:
	case 0xB3:
	case 0xD3:
	case 0xF3:
		CBRANCH( !(READ_DP( -4, data ) >> (opcode >> 5) & 1) )
	
	case 0xDE:  
		data = (uint8_t) (data + x);

	case 0x2E:{ 
		int temp;

		READ_DP_TIMER( -4, data, temp );
		CBRANCH( temp != a )
	}
	
	case 0x6E: {  
		unsigned temp = READ_DP( -4, data ) - 1;
		WRITE_DP( -3, (uint8_t) data,   temp + no_read_before_write  );
		CBRANCH( temp )
	}
	
	case 0xFE:  
		y = (uint8_t) (y - 1);
		BRANCH( y )
	
	case 0x1F:  
		SET_PC( READ_PC16( pc ) + x );

	case 0x5F:  
		SET_PC( READ_PC16( pc ) );
		goto loop;
	

	
	case 0x0F:{ 
		int temp;
		int ret_addr = GET_PC();
		SUSPICIOUS_OPCODE( "BRK" );
		SET_PC( READ_PROG16( 0xFFDE ) );  
		PUSH16( ret_addr );
		GET_PSW( temp );
		psw = (psw | b10) & ~i04;
		PUSH( temp );
		goto loop;
	}
	
	case 0x4F:{ 
		int ret_addr = GET_PC() + 1;
		SET_PC( 0xFF00 | data );
		PUSH16( ret_addr );
		goto loop;
	}
	
	case 0x01:  
	case 0x11:
	case 0x21:
	case 0x31:
	case 0x41:
	case 0x51:
	case 0x61:
	case 0x71:
	case 0x81:
	case 0x91:
	case 0xA1:
	case 0xB1:
	case 0xC1:
	case 0xD1:
	case 0xE1:
	case 0xF1: {
		int ret_addr = GET_PC();
		SET_PC( READ_PROG16( 0xFFDE - (opcode >> 3) ) );
		PUSH16( ret_addr );
		goto loop;
	}
	


	{
		int temp;
	case 0x7F:  
		temp = *sp;
		SET_PC( GET_LE16( sp + 1 ) );
		SET_SP( GET_SP() + 3 );
		goto set_psw;
	case 0x8E:  
		POP( temp );
	set_psw:
		SET_PSW( temp );
		goto loop;
	}
	
	case 0x0D: {  
		int temp;
		GET_PSW( temp );
		PUSH( temp );
		goto loop;
	}

	case 0x2D:  
		PUSH( a );
		goto loop;
	
	case 0x4D:  
		PUSH( x );
		goto loop;
	
	case 0x6D:  
		PUSH( y );
		goto loop;
	
	case 0xAE:  
		POP( a );
		goto loop;
	
	case 0xCE:  
		POP( x );
		goto loop;
	
	case 0xEE:  
		POP( y );
		goto loop;
	


	case 0x02:  
	case 0x22:
	case 0x42:
	case 0x62:
	case 0x82:
	case 0xA2:
	case 0xC2:
	case 0xE2:
	case 0x12:  
	case 0x32:
	case 0x52:
	case 0x72:
	case 0x92:
	case 0xB2:
	case 0xD2:
	case 0xF2: {
		int bit = 1 << (opcode >> 5);
		int mask = ~bit;
		if ( opcode & 0x10 )
			bit = 0;
		data += dp;
		WRITE( 0, data, (READ( -1, data ) & mask) | bit );
		goto inc_pc_loop;
	}
		
	case 0x0E:  
	case 0x4E:  
		data = READ_PC16( pc );
		pc += 2;
		{
			unsigned temp = READ( -2, data );
			nz = (uint8_t) (a - temp);
			temp &= ~a;
			if ( opcode == 0x0E )
				temp |= a;
			WRITE( 0, data, temp );
		}
		goto loop;
	
	case 0x4A:  
		c &= MEM_BIT( 0 );
		pc += 2;
		goto loop;
	
	case 0x6A:  
		c &= ~MEM_BIT( 0 );
		pc += 2;
		goto loop;
	
	case 0x0A:  
		c |= MEM_BIT( -1 );
		pc += 2;
		goto loop;
	
	case 0x2A:  
		c |= ~MEM_BIT( -1 );
		pc += 2;
		goto loop;
	
	case 0x8A:  
		c ^= MEM_BIT( -1 );
		pc += 2;
		goto loop;
	
	case 0xEA:  
		data = READ_PC16( pc );
		pc += 2;
		{
			unsigned temp = READ( -1, data & 0x1FFF );
			temp ^= 1 << (data >> 13);
			WRITE( 0, data & 0x1FFF, temp );
		}
		goto loop;
	
	case 0xCA:  
		data = READ_PC16( pc );
		pc += 2;
		{
			unsigned temp = READ( -2, data & 0x1FFF );
			unsigned bit = data >> 13;
			temp = (temp & ~(1 << bit)) | ((c >> 8 & 1) << bit);
			WRITE( 0, data & 0x1FFF, temp + no_read_before_write  );
		}
		goto loop;
	
	case 0xAA:  
		c = MEM_BIT( 0 );
		pc += 2;
		goto loop;
	


	case 0x60:  
		c = 0;
		goto loop;
		
	case 0x80:  
		c = ~0;
		goto loop;
	
	case 0xED:  
		c ^= 0x100;
		goto loop;
		
	case 0xE0:  
		psw &= ~(v40 | h08);
		goto loop;
	
	case 0x20:  
		dp = 0;
		goto loop;
	
	case 0x40:  
		dp = 0x100;
		goto loop;
	
	case 0xA0:  
		SUSPICIOUS_OPCODE( "EI" );
		psw |= i04;
		goto loop;
	
	case 0xC0:  
		SUSPICIOUS_OPCODE( "DI" );
		psw &= ~i04;
		goto loop;
	


	case 0x00:  
		goto loop;
	
	case 0xFF:{ 

		unsigned addr = GET_PC() - 1;
		if ( addr >= 0x10000 )
		{
			addr &= 0xFFFF;
			SET_PC( addr );
			debug_printf( "SPC: PC wrapped around\n" );
			goto loop;
		}
	}

	case 0xEF:  
		SUSPICIOUS_OPCODE( "STOP/SLEEP" );
		--pc;
		rel_time = 0;
		m.cpu_error = "SPC emulation error";
		goto stop;
	}  
	
	assert( 0 );  
}   
out_of_time:
	rel_time -= m.cycle_table [*pc];  
stop:
	

	if ( GET_PC() >= 0x10000 )
		debug_printf( "SPC: PC wrapped around\n" );
	m.cpu_regs.pc = (uint16_t) GET_PC();
	m.cpu_regs.sp = ( uint8_t) GET_SP();
	m.cpu_regs.a  = ( uint8_t) a;
	m.cpu_regs.x  = ( uint8_t) x;
	m.cpu_regs.y  = ( uint8_t) y;
	{
		int temp;
		GET_PSW( temp );
		m.cpu_regs.psw = (uint8_t) temp;
	}
}
SPC_CPU_RUN_FUNC_END
