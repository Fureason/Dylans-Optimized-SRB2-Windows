/*
 * libopenmpt.hpp
 * --------------
 * Purpose: libopenmpt public c++ interface
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_HPP
#define LIBOPENMPT_HPP

#include "libopenmpt_config.h"

#include <exception>
#include <iosfwd>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <cstdint>

   
                                        
  
                                               
  
                                                                   
  
                                                                           
                                                              
                      
                                                                              
                                                                           
                  
  
                                          
  
                                                               
                                                                      
                                                     
                                                                 
                      
  
                                          
  
                                                          
  
                                                                               
                                                                              
                                  
          
                                                                             
                                                                             
                                                                               
                                              
                                                                           
                                                                        
                                                                            
                                                                         
                                                                        
  
                                                      
                                                      
                                                                                                                         
                                                                                                                        
                                                                                                                        
  
                                                                          
                                                                             
                 
  
                                                     
  
                                                          
                                               
  
                                                                                
                                                                         
                         
  
                                                                             
                                                                         
                                                                             
                                                                                
                                                                                
                                                                           
           
                                                                            
                                                                            
                                                                                
                                                                                
                                                    
  
                                                                            
  
                                
                                                       
                                                                            
                            
                                                                           
                                                             
                                                                               
                                                                   
                                                            
                                                                           
  
                                                  
  
                                                                               
                                                                         
                                                                           
                                             
  
                                                          
  
                      
  
                                           
  
                                      
  
   

 

                              
    
  

namespace openmpt {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4275)
#endif

   
                                                                                                                                                                                                    
                               
  
class LIBOPENMPT_CXX_API exception : public std::exception {
private:
	char * text;
public:
	exception( const std::string & text ) noexcept;
	exception( const exception & other ) noexcept;
	exception( exception && other ) noexcept;
	exception & operator = ( const exception & other ) noexcept;
	exception & operator = ( exception && other ) noexcept;
	virtual ~exception() noexcept;
	const char * what() const noexcept override;
};  
#if defined(_MSC_VER)
#pragma warning(pop)
#endif


   
                                        
                                                                        
                                                                                                    
  
LIBOPENMPT_CXX_API std::uint32_t get_library_version();


   
                                         
                                                                                          
  
LIBOPENMPT_CXX_API std::uint32_t get_core_version();

namespace string {


LIBOPENMPT_DEPRECATED static const char library_version  LIBOPENMPT_ATTR_DEPRECATED [] = "library_version";

LIBOPENMPT_DEPRECATED static const char library_features LIBOPENMPT_ATTR_DEPRECATED [] = "library_features";

LIBOPENMPT_DEPRECATED static const char core_version     LIBOPENMPT_ATTR_DEPRECATED [] = "core_version";

LIBOPENMPT_DEPRECATED static const char build            LIBOPENMPT_ATTR_DEPRECATED [] = "build";

LIBOPENMPT_DEPRECATED static const char credits          LIBOPENMPT_ATTR_DEPRECATED [] = "credits";

LIBOPENMPT_DEPRECATED static const char contact          LIBOPENMPT_ATTR_DEPRECATED [] = "contact";
//! Return the libopenmpt license from openmpt::string::get(). \deprecated Please use `"license"` directly.
LIBOPENMPT_DEPRECATED static const char license          LIBOPENMPT_ATTR_DEPRECATED [] = "license";


/*!
  \param key Key to query.
         Possible keys are:
          -  "library_version": verbose library version string
          -  "library_version_major": libopenmpt major version number
          -  "library_version_minor": libopenmpt minor version number
          -  "library_version_patch": libopenmpt patch version number
          -  "library_version_prerel": libopenmpt pre-release version string
          -  "library_version_is_release": "1" if the version is an officially released version
          -  "library_features": verbose library features string
          -  "core_version": verbose OpenMPT core version string
          -  "source_url": original source code URL
          -  "source_date": original source code date
          -  "source_revision": original source code revision
          -  "source_is_modified": "1" if the original source has been modified
          -  "source_has_mixed_revisions": "1" if the original source has been compiled from different various revision
          -  "source_is_package": "1" if the original source has been obtained from a source pacakge instead of source code version control
          -  "build": information about the current build (e.g. the build date or compiler used)
          -  "build_compiler": information about the compiler used to build libopenmpt
          -  "credits": all contributors
          -  "contact": contact information about libopenmpt
          -  "license": the libopenmpt license
          -  "url": libopenmpt website URL
          -  "support_forum_url": libopenmpt support and discussions forum URL
          -  "bugtracker_url": libopenmpt bug and issue tracker URL

  \return A (possibly multi-line) string containing the queried information. If no information is available, the string is empty.
*/
LIBOPENMPT_CXX_API std::string get( const std::string & key );

}  


   
                                                                                                                                  
  
LIBOPENMPT_CXX_API std::vector<std::string> get_supported_extensions();


   
                                                                                      
                                                                            
  
LIBOPENMPT_CXX_API bool is_extension_supported( const std::string & extension );


   
                                     
                                                                                                                                                                                                                                                                                                                           
                                                      
                                          
                                                                                                                                                                                                                          
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
                              
                                  
              
  
LIBOPENMPT_CXX_API double could_open_probability( std::istream & stream, double effort = 1.0, std::ostream & log = std::clog );


   
                                                                   
  
LIBOPENMPT_ATTR_DEPRECATED LIBOPENMPT_CXX_API LIBOPENMPT_DEPRECATED double could_open_propability( std::istream & stream, double effort = 1.0, std::ostream & log = std::clog );


   
                                  
              
  
LIBOPENMPT_CXX_API std::size_t probe_file_header_get_recommended_size();


static const std::uint64_t probe_file_header_flags_modules    = 0x1ul;


static const std::uint64_t probe_file_header_flags_containers = 0x2ul;


static const std::uint64_t probe_file_header_flags_default    = probe_file_header_flags_modules | probe_file_header_flags_containers;


static const std::uint64_t probe_file_header_flags_none       = 0x0ul;


enum probe_file_header_result {
	probe_file_header_result_success      =  1,
	probe_file_header_result_failure      =  0,
	probe_file_header_result_wantmoredata = -1
};


   
                                                                                                                                                                  
                                         
                                                     
                                                     
                                                                                                                                                                                                                                  
                                                                                                                                                                                                         
                                                                                                
                                                                                   
                                                                                                                   
                                                       
                                       
              
  
LIBOPENMPT_CXX_API int probe_file_header( std::uint64_t flags, const std::uint8_t * data, std::size_t size, std::uint64_t filesize );


   
                                                                                                                                                                  
                                         
                                                     
                                                                                                                                                                                                     
                                                                                                                                                                                                                     
                                                                                                                                                                                                         
                                                                                                
                                                                                   
                                                                                                                   
                                                       
                                       
              
  
LIBOPENMPT_CXX_API int probe_file_header( std::uint64_t flags, const std::uint8_t * data, std::size_t size );


   
                                                                                                                                                                  
                                     
                                                                             
                                                                                                                                                                                                         
                                                                                                
                                                                                   
                                                                                                                   
                                                       
                                       
              
  
LIBOPENMPT_CXX_API int probe_file_header( std::uint64_t flags, std::istream & stream );

class module_impl;

class module_ext;

namespace detail {

typedef std::map< std::string, std::string > initial_ctls_map;

}  

class LIBOPENMPT_CXX_API module {

	friend class module_ext;

public:


	enum render_param {

		   
                                                               
                             
                                             
    
		RENDER_MASTERGAIN_MILLIBEL        = 1,

		   
                                                                                                      
                               
                                           
    
		RENDER_STEREOSEPARATION_PERCENT   = 2,

		   
                                                                                                
                                                                          
                                                                                                                                    
                               
                          
                                            
                              
                             
                                   
    
		RENDER_INTERPOLATIONFILTER_LENGTH = 3,

		   
                                                                                             
                                                                           
                                             
                                                                                               
                                                   
    
		RENDER_VOLUMERAMPING_STRENGTH     = 4
	};


	enum command_index {
		command_note        = 0,
		command_instrument  = 1,
		command_volumeffect = 2,
		command_effect      = 3,
		command_volume      = 4,
		command_parameter   = 5
	};

private:
	module_impl * impl;
private:

	module( const module & );
	void operator = ( const module & );
private:

	module();
	void set_impl( module_impl * i );
public:

	   
                                                                                                                                                                                                                                                                                         
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( std::istream & stream, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                            
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const std::vector<std::uint8_t> & data, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                                    
                                                  
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const std::uint8_t * beg, const std::uint8_t * end, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                            
                                        
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const std::uint8_t * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                            
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const std::vector<char> & data, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                                    
                                                  
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const char * beg, const char * end, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                            
                                        
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const char * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	   
                                            
                                        
                                                                                                                                                      
                                                                          
                                                                                                                             
                                                                                                       
                                 
   
	module( const void * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	virtual ~module();
public:


	   
                                                                              
                                                                                                                                                      
                                                                                                                   
                                                                                                                                                                                                                                                                                               
   
	void select_subsong( std::int32_t subsong );

	   
                                                                                                                       
                                                                                                             
               
   
	std::int32_t get_selected_subsong() const;

	   
                                   
                         
                                                    
                                                   
                                        
   
	void set_repeat_count( std::int32_t repeat_count );

	   
                       
                         
                                                    
                                                   
                                        
   
	std::int32_t get_repeat_count() const;


	   
                                                               
   
	double get_duration_seconds() const;


	   
                                                                                                                          
                                                    
                                            
   
	double set_position_seconds( double seconds );

	   
                                            
                                            
   
	double get_position_seconds() const;


	   
                                                                                                      
                                                
                                            
                                                    
                                            
                                            
   
	double set_position_order_row( std::int32_t order, std::int32_t row );


	   
                                                                      
                                              
                                                                                                      
                                    
                                        
   
	std::int32_t get_render_param( int param ) const;

	   
                                                                    
                                          
                                                                                                                               
                                    
                                        
   
	void set_render_param( int param, std::int32_t value );

	 

	   
                                                                                                        
                                                             
                                                                                                   
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                         
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * mono );

	   
                                                                                                        
                                                             
                                                                                            
                                                                                              
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                         
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right );

	   
                                                                                                        
                                                             
                                                                                            
                                                                                              
                                                                                                      
                                                                                                        
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                         
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, std::int16_t * left, std::int16_t * right, std::int16_t * rear_left, std::int16_t * rear_right );

	   
                                                                                                        
                                                             
                                                                                                   
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                                            
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, float * mono );

	   
                                                                                                        
                                                             
                                                                                            
                                                                                              
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                                            
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right );

	   
                                                                                                        
                                                             
                                                                                            
                                                                                              
                                                                                                      
                                                                                                        
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                                            
                                       
   
	std::size_t read( std::int32_t samplerate, std::size_t count, float * left, float * right, float * rear_left, float * rear_right );

	   
                                                                                                        
                                                             
                                                                                                                                             
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                         
                                       
   
	std::size_t read_interleaved_stereo( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_stereo );

	   
                                                                                                        
                                                             
                                                                                                                                                        
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                         
                                       
   
	std::size_t read_interleaved_quad( std::int32_t samplerate, std::size_t count, std::int16_t * interleaved_quad );

	   
                                                                                                        
                                                             
                                                                                                                                             
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                                            
                                       
   
	std::size_t read_interleaved_stereo( std::int32_t samplerate, std::size_t count, float * interleaved_stereo );

	   
                                                                                                        
                                                             
                                                                                                                                                        
                                                  
                                              
                                                                                         
                                                                                                                                                                                                                                                                           
                                                                                                                                            
                                       
   
	std::size_t read_interleaved_quad( std::int32_t samplerate, std::size_t count, float * interleaved_quad );
	 


	   
                                                                        
                                    
   
	std::vector<std::string> get_metadata_keys() const;

	   
                                                                                                             
                              
                                                     
                                                                                             
                                                                                                                                                    
                                                                                                                                                                                            
                                                                                           
                                                                                                              
                                                                                            
                                          
                                 
                                                                       
                                                                                                                                                                                 
                                                                                                                                                       
                                                                                        
                                        
                                         
   
	std::string get_metadata( const std::string & key ) const;


	   
                                              
   
	std::int32_t get_current_speed() const;

	   
                                                                                                                    
   
	std::int32_t get_current_tempo() const;

	   
                                                                      
   
	std::int32_t get_current_order() const;

	   
                                                    
   
	std::int32_t get_current_pattern() const;

	   
                                                                        
   
	std::int32_t get_current_row() const;

	   
                                                                           
   
	std::int32_t get_current_playing_channels() const;


	   
                                                               
                                          
                                                                                                                                             
   
	float get_current_channel_vu_mono( std::int32_t channel ) const;

	   
                                                               
                                          
                                                                                                                                             
   
	float get_current_channel_vu_left( std::int32_t channel ) const;

	   
                                                               
                                          
                                                                                                                                             
   
	float get_current_channel_vu_right( std::int32_t channel ) const;

	   
                                                               
                                          
                                                                                                                                             
   
	float get_current_channel_vu_rear_left( std::int32_t channel ) const;

	   
                                                               
                                          
                                                                                                                                             
   
	float get_current_channel_vu_rear_right( std::int32_t channel ) const;


	   
                                                                                                                                                                                                                                  
                                                                                                                 
   
	std::int32_t get_num_subsongs() const;

	   
                                                                                                      
                                                                                                                                                                                                                                                                                                                                                                                  
   
	std::int32_t get_num_channels() const;

	   
                                                                      
   
	std::int32_t get_num_orders() const;

	   
                                                         
   
	std::int32_t get_num_patterns() const;

	   
                                                                                                                                                
   
	std::int32_t get_num_instruments() const;

	   
                                                    
   
	std::int32_t get_num_samples() const;


	   
                              
                                                                                                                
   
	std::vector<std::string> get_subsong_names() const;

	   
                             
                                        
   
	std::vector<std::string> get_channel_names() const;

	   
                           
                                      
   
	std::vector<std::string> get_order_names() const;

	   
                             
                                        
   
	std::vector<std::string> get_pattern_names() const;

	   
                                
                                           
   
	std::vector<std::string> get_instrument_names() const;

	   
                            
                                       
   
	std::vector<std::string> get_sample_names() const;


	   
                                                                       
                                                                                       
   
	std::int32_t get_order_pattern( std::int32_t order ) const;


	   
                                                                  
                                                                                                 
   
	std::int32_t get_pattern_num_rows( std::int32_t pattern ) const;


	   
                                                             
                                                              
                                                                      
                                                                                                          
                                                                        
   
	std::uint8_t get_pattern_row_channel_command( std::int32_t pattern, std::int32_t row, std::int32_t channel, int command ) const;


	   
                                                             
                                                              
                                                                      
                                                                       
                                                                                                       
                                                             
   
	std::string format_pattern_row_channel_command( std::int32_t pattern, std::int32_t row, std::int32_t channel, int command ) const;


	   
                                                             
                                                              
                                                                      
                                                                                                          
                                                                                                                                                                 
                                                                                                                                                                          
                                                 
                               
                             
                                
                                
                                      
                                                
                                                   
                                                
                                                   
                                                       
   
	std::string highlight_pattern_row_channel_command( std::int32_t pattern, std::int32_t row, std::int32_t channel, int command ) const;


	   
                                                             
                                                              
                                                                      
                                                                                             
                                                                                                      
                                                                    
                                                     
   
	std::string format_pattern_row_channel( std::int32_t pattern, std::int32_t row, std::int32_t channel, std::size_t width = 0, bool pad = true ) const;

	   
                                                             
                                                              
                                                                      
                                                                                             
                                                                                                      
                                                                                                                                                            
                                                  
   
	std::string highlight_pattern_row_channel( std::int32_t pattern, std::int32_t row, std::int32_t channel, std::size_t width = 0, bool pad = true ) const;


	   
                                                      
                                               
                                                                                
                                                                                  
                                                                    
                                                                                                                                                    
                                                                                                                                                                
                                                                                                                                                                       
                                                                                 
                                                                                                                                                 
                                                                                                                                                                          
                                                                                                                                            
                                                                                               
                                                                                               
                                                                                                                                                                                                                                                                                
                                                                                                                                
                                                                                                                                   
                                        
                                                                              
                                                                                                   
                                                                                   

                                                                                                                                                                                                                                                                                                                                                                                     
   
	std::vector<std::string> get_ctls() const;


	   
                                                          
                                    
                                
   
	std::string ctl_get( const std::string & ctl ) const;

	   
                                                    
                                             
                                                                                                                                                                                                                  
                                
   
	void ctl_set( const std::string & ctl, const std::string & value );



};  

}  

   
    
  

#endif  
