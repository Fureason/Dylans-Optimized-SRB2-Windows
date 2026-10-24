/*
 * libopenmpt_ext.hpp
 * ------------------
 * Purpose: libopenmpt public c++ interface for libopenmpt extensions
 * Notes  :
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#ifndef LIBOPENMPT_EXT_HPP
#define LIBOPENMPT_EXT_HPP

#include "libopenmpt_config.h"
#include "libopenmpt.hpp"

   
                                                           
  
                                                           
  
                                                              
  
                          
  
   

 

                                  
    
  

namespace openmpt {

class module_ext_impl;

class LIBOPENMPT_CXX_API module_ext : public module {
	
private:
	module_ext_impl * ext_impl;
private:

	module_ext( const module_ext & );
	void operator = ( const module_ext & );
public:
	module_ext( std::istream & stream, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const std::vector<char> & data, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const char * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	module_ext( const void * data, std::size_t size, std::ostream & log = std::clog, const std::map< std::string, std::string > & ctls = detail::initial_ctls_map() );
	virtual ~module_ext();

public:


	                                                                                  
              
                                                                
                                              
                                                                                                                                                   
                         
                                                                           
             
                                 
      
        
                              
         
           
                                                                       
                                                                                      
   
	void * get_interface( const std::string & interface_id );

};  

namespace ext {

#define LIBOPENMPT_DECLARE_EXT_CXX_INTERFACE(name) \
	static const char name ## _id [] = # name ; \
	class name; \
 

#define LIBOPENMPT_EXT_CXX_INTERFACE(name) \
	protected: \
		name () {} \
		virtual ~ name () {} \
	public: \
 


#ifndef LIBOPENMPT_EXT_INTERFACE_PATTERN_VIS
#define LIBOPENMPT_EXT_INTERFACE_PATTERN_VIS
#endif

LIBOPENMPT_DECLARE_EXT_CXX_INTERFACE(pattern_vis)

class pattern_vis {

	LIBOPENMPT_EXT_CXX_INTERFACE(pattern_vis)


	enum effect_type {

		effect_unknown = 0,
		effect_general = 1,
		effect_global = 2,
		effect_volume = 3,
		effect_panning = 4,
		effect_pitch = 5

	};  


	   
                                                             
                                                              
                                                                      
                                                                                                                           
                                                                     
   
	virtual effect_type get_pattern_row_channel_volume_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const = 0;


	   
                                                             
                                                              
                                                                      
                                                                                                                           
                                                                            
   
	virtual effect_type get_pattern_row_channel_effect_type( std::int32_t pattern, std::int32_t row, std::int32_t channel ) const = 0;

};  


#ifndef LIBOPENMPT_EXT_INTERFACE_INTERACTIVE
#define LIBOPENMPT_EXT_INTERFACE_INTERACTIVE
#endif

LIBOPENMPT_DECLARE_EXT_CXX_INTERFACE(interactive)

class interactive {

	LIBOPENMPT_EXT_CXX_INTERFACE(interactive)


	   
                                                       
                                                                                                                              
                                                                        
                                         
   
	virtual void set_current_speed( std::int32_t speed ) = 0;


	   
                                                                                                                              
                                                                                                                              
                                                                                                                                                                                   
                                         
   
	virtual void set_current_tempo( std::int32_t tempo ) = 0;


	   
                                                                                       
                                                                                                                               
                                                                                                                                                                                        
                                                  
   
	virtual void set_tempo_factor( double factor ) = 0;


	   
                                    
                                                  
   
	virtual double get_tempo_factor( ) const = 0;
	

	   
                                                                                       
                                                                                                                               
                                                                                                                                                                                            
                                                                                                                  
                                                  
   
	virtual void set_pitch_factor( double factor ) = 0;


	   
                                    
                                                  
   
	virtual double get_pitch_factor( ) const = 0;


	   
                                                          
                                                                                                                               
                                                                                                                                                                                                  
                                                   
   
	virtual void set_global_volume( double volume ) = 0;


	   
                                                        
                                                   
   
	virtual double get_global_volume( ) const = 0;
	

	   
                                                                                                           
                                                           
                                                                                                                                          
                                                                            
                                                    
   
	virtual void set_channel_volume( std::int32_t channel, double volume ) = 0;


	   
                                                                                                                 
                                                         
                                                                                                                                
                                                    
   
	virtual double get_channel_volume( std::int32_t channel ) const = 0;


	   
                                                                                                                
                                                                    
                                                                                                                                
                                                         
   
	virtual void set_channel_mute_status( std::int32_t channel, bool mute ) = 0;


	   
                                                                                                                      
                                                                            
                                                                                                                                
                                                         
   
	virtual bool get_channel_mute_status( std::int32_t channel ) const = 0;
	

	   
                                                                                                                                                                                                                                
                                                                    
                                                                                                                                   
                                                            
   
	virtual void set_instrument_mute_status( std::int32_t instrument, bool mute ) = 0;


	   
                                                                                                                                                                                                                                      
                                                                               
                                                                                                                                   
                                                            
   
	virtual bool get_instrument_mute_status( std::int32_t instrument ) const = 0;


	   
                                                                                                                                                                                                                      
                                                                      
                                                                                      
                                                                                                                  
                                                                                                                                   
                                                                                                                                           
                                           
   
	virtual std::int32_t play_note( std::int32_t instrument, std::int32_t note, double volume, double panning ) = 0;


	   
                                                                  
                                                                                                                  
                                           
   
	virtual void stop_note( std::int32_t channel ) = 0;

};  


 



#undef LIBOPENMPT_DECLARE_EXT_CXX_INTERFACE
#undef LIBOPENMPT_EXT_CXX_INTERFACE

}  

}  

   
    
  

#endif  
