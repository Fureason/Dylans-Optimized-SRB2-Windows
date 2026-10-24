

#include "gme/Music_Emu.h"

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
	

	gme_type_t file_type;
	handle_error( gme_identify_file( filename, &file_type ) );
	if ( !file_type )
		handle_error( "Unsupported music type" );
	

	Music_Emu* emu = file_type->new_emu();
	if ( !emu )
		handle_error( "Out of memory" );
	handle_error( emu->set_sample_rate( sample_rate ) );
	

	handle_error( emu->load_file( filename ) );
	

	handle_error( emu->start_track( track ) );
	

	Wave_Writer wave( sample_rate, "out.wav" );
	wave.enable_stereo();
	

	while ( emu->tell() < 10 * 1000L )
	{

		const long size = 1024;  
		short buf [size];
		

		handle_error( emu->play( size, buf ) );
		

		wave.write( buf, size );
	}
	

	delete emu;
	
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
