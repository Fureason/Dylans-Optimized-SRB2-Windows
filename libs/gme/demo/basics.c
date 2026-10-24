 

#include "gme/gme.h"

#include "Wave_Writer.h"  
#include <stdlib.h>
#include <stdio.h>

void handle_error( const char* str );

int main(int argc, char *argv[])
{
	const char *filename = "test.nsf";  
	if ( argc >= 2 )
		filename = argv[1];

	long sample_rate = 44100;  
	 
	int track = argc >= 3 ? atoi(argv[2]) : 0;
	
	 
	Music_Emu* emu;
	handle_error( gme_open_file( filename, &emu, sample_rate ) );
	
	 
	handle_error( gme_start_track( emu, track ) );
	
	 
	wave_open( sample_rate, "out.wav" );
	wave_enable_stereo();
	
	 
	while ( gme_tell( emu ) < 10 * 1000L )
	{
		 
		#define buf_size 1024  
		short buf [buf_size];
		
		 
		handle_error( gme_play( emu, buf_size, buf ) );
		
		 
		wave_write( buf, buf_size );
	}
	
	 
	gme_delete( emu );
	wave_close();
	
	return 0;
}

void handle_error( const char* str )
{
	if ( str )
	{
		printf( "Error: %s\n", str ); getchar();
		exit( EXIT_FAILURE );
	}
}
