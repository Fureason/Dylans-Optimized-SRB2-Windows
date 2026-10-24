                                                                        

                                           

                        
                         
                                                      
                                     
                        
                                 
                                                    

int const scope_width = 512;

#include "Music_Player.h"
#include "Audio_Scope.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "SDL.h"

void handle_error( const char* );

static bool paused;
static Audio_Scope* scope;
static Music_Player* player;
static short scope_buf [scope_width * 2];

static void init()
{

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) < 0 )
		exit( EXIT_FAILURE );
	atexit( SDL_Quit );
	SDL_EnableKeyRepeat( 500, 80 );
	

	scope = new Audio_Scope;
	if ( !scope )
		handle_error( "Out of memory" );
	if ( scope->init( scope_width, 256 ) )
		handle_error( "Couldn't initialize scope" );
	memset( scope_buf, 0, sizeof scope_buf );
	

	player = new Music_Player;
	if ( !player )
		handle_error( "Out of memory" );
	handle_error( player->init() );
	player->set_scope_buffer( scope_buf, scope_width * 2 );
}

static void start_track( int track, const char* path )
{
	paused = false;
	handle_error( player->start_track( track - 1 ) );
	

	
	long seconds = player->track_info().length / 1000;
	const char* game = player->track_info().game;
	if ( !*game )
	{

		game = strrchr( path, '\\' );  
		if ( !game )
			game = strrchr( path, '/' );  
		if ( !game )
			game = path;
		else
			game++;  
	}
	
	char title [512];
	sprintf( title, "%s: %d/%d %s (%ld:%02ld)",
			game, track, player->track_count(), player->track_info().song,
			seconds / 60, seconds % 60 );
	SDL_WM_SetCaption( title, title );
}

int main( int argc, char** argv )
{
	init();
	

	const char* path = (argc > 1 ? argv [argc - 1] : "test.nsf");
	handle_error( player->load_file( path ) );
	start_track( 1, path );
	

	int track = 1;
	double tempo = 1.0;
	bool running = true;
	double stereo_depth = 0.0;
	bool accurate = false;
	int muting_mask = 0;
	while ( running )
	{
		SDL_Delay( 1000 / 100 );
		

		scope->draw( scope_buf, scope_width, 2 );
		

		if ( player->track_ended() )
		{
			if ( track < player->track_count() )
				start_track( ++track, path );
			else
				player->pause( paused = true );
		}
		

		SDL_Event e;
		while ( SDL_PollEvent( &e ) )
		{
			switch ( e.type )
			{
			case SDL_QUIT:
				running = false;
				break;
			
			case SDL_KEYDOWN:
				int key = e.key.keysym.sym;
				switch ( key )
				{
				case SDLK_q:
				case SDLK_ESCAPE:  
					running = false;
					break;
				
				case SDLK_LEFT:  
					if ( !paused && !--track )
						track = 1;
					start_track( track, path );
					break;
				
				case SDLK_RIGHT:  
					if ( track < player->track_count() )
						start_track( ++track, path );
					break;
				
				case SDLK_MINUS:  
					tempo -= 0.1;
					if ( tempo < 0.1 )
						tempo = 0.1;
					player->set_tempo( tempo );
					break;
				
				case SDLK_EQUALS:  
					tempo += 0.1;
					if ( tempo > 2.0 )
						tempo = 2.0;
					player->set_tempo( tempo );
					break;
				
				case SDLK_SPACE:  
					paused = !paused;
					player->pause( paused );
					break;
				
				case SDLK_a:  
					accurate = !accurate;
					player->enable_accuracy( accurate );
					break;
				
				case SDLK_e:  
					stereo_depth += 0.2;
					if ( stereo_depth > 0.5 )
						stereo_depth = 0;
					player->set_stereo_depth( stereo_depth );
					break;
				
				case SDLK_0:  
					tempo = 1.0;
					muting_mask = 0;
					player->set_tempo( tempo );
					player->mute_voices( muting_mask );
					break;
				
				default:
					if ( SDLK_1 <= key && key <= SDLK_9 )  
					{
						muting_mask ^= 1 << (key - SDLK_1);
						player->mute_voices( muting_mask );
					}
				}
			}
		}
	}
	

	delete player;
	delete scope;
	
	return 0;
}

void handle_error( const char* error )
{
	if ( error )
	{

		char str [256];
		sprintf( str, "Error: %s", error );
		fprintf( stderr, "%s\n", str );
		SDL_WM_SetCaption( str, str );
		

		SDL_Event e;
		do
		{
			while ( !SDL_PollEvent( &e ) ) { }
		}
		while ( e.type != SDL_QUIT && e.type != SDL_KEYDOWN && e.type != SDL_MOUSEBUTTONDOWN );

		exit( EXIT_FAILURE );
	}
}
