 

 
#ifndef GME_H
#define GME_H

#ifdef __cplusplus
	extern "C" {
#endif

#define GME_VERSION 0x000601  

 
typedef const char* gme_err_t;

 
typedef struct Music_Emu Music_Emu;


 

 
gme_err_t gme_open_file( const char path [], Music_Emu** out, int sample_rate );

 
int gme_track_count( Music_Emu const* );

 
gme_err_t gme_start_track( Music_Emu*, int index );

 
gme_err_t gme_play( Music_Emu*, int count, short out [] );

 
void gme_delete( Music_Emu* );


 

                                                                                 
                                                   
void gme_set_fade( Music_Emu*, int start_msec );

 
int gme_track_ended( Music_Emu const* );

 
int gme_tell( Music_Emu const* );

 
int gme_tell_samples( Music_Emu const* );

 
gme_err_t gme_seek( Music_Emu*, int msec );

 
gme_err_t gme_seek_samples( Music_Emu*, int n );


 

                                                                               
                            
enum { gme_info_only = -1 };

                                                                                       
                                                                    
const char* gme_warning( Music_Emu* );

 
gme_err_t gme_load_m3u( Music_Emu*, const char path [] );

                                                                                
                               
void gme_clear_playlist( Music_Emu* );

                                                                        
                           
typedef struct gme_info_t gme_info_t;
gme_err_t gme_track_info( Music_Emu const*, gme_info_t** out, int track );

 
void gme_free_info( gme_info_t* );

struct gme_info_t
{
	 
	int length;			 
	int intro_length;	 
	int loop_length;	 
	
	                                                                          
                                                
	int play_length;
	
	int i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15;  
	
	 
	const char* system;
	const char* game;
	const char* song;
	const char* author;
	const char* copyright;
	const char* comment;
	const char* dumper;
	
	const char *s7,*s8,*s9,*s10,*s11,*s12,*s13,*s14,*s15;  
};


 

                                                                                 
                                       
void gme_set_stereo_depth( Music_Emu*, double depth );

                                                                                
                    
void gme_ignore_silence( Music_Emu*, int ignore );

                                                                               
                                                                   
void gme_set_tempo( Music_Emu*, double tempo );

 
int gme_voice_count( Music_Emu const* );

 
const char* gme_voice_name( Music_Emu const*, int i );

 
void gme_mute_voice( Music_Emu*, int index, int mute );

                                                                              
                                                                    
void gme_mute_voices( Music_Emu*, int muting_mask );

 
 
typedef struct gme_equalizer_t
{
	double treble;  
	double bass;    
	
	double d2,d3,d4,d5,d6,d7,d8,d9;  
} gme_equalizer_t;

 
void gme_equalizer( Music_Emu const*, gme_equalizer_t* out );

 
void gme_set_equalizer( Music_Emu*, gme_equalizer_t const* eq );

 
void gme_enable_accuracy( Music_Emu*, int enabled );


 

 
typedef const struct gme_type_t_* gme_type_t;

 
extern const gme_type_t
	gme_ay_type,
	gme_gbs_type,
	gme_gym_type,
	gme_hes_type,
	gme_kss_type,
	gme_nsf_type,
	gme_nsfe_type,
	gme_sap_type,
	gme_spc_type,
	gme_vgm_type,
	gme_vgz_type;

 
gme_type_t gme_type( Music_Emu const* );

                                                                                      
                                                                           
gme_type_t const* gme_type_list();

 
const char* gme_type_system( gme_type_t );

 
int gme_type_multitrack( gme_type_t );


 

 
extern const char* const gme_wrong_file_type;

 
gme_err_t gme_open_data( void const* data, long size, Music_Emu** out, int sample_rate );

                                                                              
                                                                       
                                 
const char* gme_identify_header( void const* header );

 
gme_type_t gme_identify_extension( const char path_or_extension [] );

                                                                                           
                                                         
gme_err_t gme_identify_file( const char path [], gme_type_t* type_out );

                                                                                           
                                                         
Music_Emu* gme_new_emu( gme_type_t, int sample_rate );

 
gme_err_t gme_load_file( Music_Emu*, const char path [] );

 
gme_err_t gme_load_data( Music_Emu*, void const* data, long size );

                                                                           
                                                                        
typedef gme_err_t (*gme_reader_t)( void* your_data, void* out, int count );
gme_err_t gme_load_custom( Music_Emu*, gme_reader_t, long file_size, void* your_data );

 
gme_err_t gme_load_m3u_data( Music_Emu*, void const* data, long size );


 

                                                                    
                                          
void  gme_set_user_data( Music_Emu*, void* new_user_data );
void* gme_user_data( Music_Emu const* );

                                                                            
                                                  
typedef void (*gme_user_cleanup_t)( void* user_data );
void gme_set_user_cleanup( Music_Emu*, gme_user_cleanup_t func );


#ifdef __cplusplus
	}
#endif

#endif
