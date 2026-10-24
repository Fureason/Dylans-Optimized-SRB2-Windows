<?php
/*
 * Project:     MagpieRSS: a simple RSS integration tool
 * File:        rss_cache.inc, a simple, rolling(no GC), cache 
 *              for RSS objects, keyed on URL.
 * Author:      Kellan Elliott-McCrea <kellan@protest.net>
 * Version:     0.51
 * License:     GPL
 *
 * The lastest version of MagpieRSS can be obtained from:
 * http://magpierss.sourceforge.net
 *
 * For questions, help, comments, discussion, etc., please join the
 * Magpie mailing list:
 * http://lists.sourceforge.net/lists/listinfo/magpierss-general
 *
 */

class RSSCache {
    var $BASE_CACHE = './cache';     
    var $MAX_AGE    = 3600;          
    var $ERROR      = "";            
    
    function RSSCache ($base='', $age='') {
        if ( $base ) {
            $this->BASE_CACHE = $base;
        }
        if ( $age ) {
            $this->MAX_AGE = $age;
        }
        

        if ( ! file_exists( $this->BASE_CACHE ) ) {
            $status = @mkdir( $this->BASE_CACHE, 0755 );
            

            if ( ! $status ) {
                $this->error(
                    "Cache couldn't make dir '" . $this->BASE_CACHE . "'."
                );
            }
        }
    }
    
                                                                          \
                   
                                                      
                                                      
                                
                                                                           
    function set ($url, $rss) {
        $this->ERROR = "";
        $cache_file = $this->file_name( $url );
        $fp = @fopen( $cache_file, 'w' );
        
        if ( ! $fp ) {
            $this->error(
                "Cache unable to open file for writing: $cache_file"
            );
            return 0;
        }
        
        
        $data = $this->serialize( $rss );
        fwrite( $fp, $data );
        fclose( $fp );
        
        return $cache_file;
    }
    
                                                                          \
                   
                                            
                                                      
                                                    
                                                                            
    function get ($url) {
        $this->ERROR = "";
        $cache_file = $this->file_name( $url );
        
        if ( ! file_exists( $cache_file ) ) {
            $this->debug( 
                "Cache doesn't contain: $url (cache file: $cache_file)"
            );
            return 0;
        }
        
        $fp = @fopen($cache_file, 'r');
        if ( ! $fp ) {
            $this->error(
                "Failed to open cache file for reading: $cache_file"
            );
            return 0;
        }
        
        $data = fread( $fp, filesize($cache_file) );
        $rss = $this->unserialize( $data );
        
        return $rss;
    }

                                                                          \
                           
                                                       
                                                                        
                                                      
                                                    
                                                                                
    function check_cache ( $url ) {
        $this->ERROR = "";
        $filename = $this->file_name( $url );
        
        if ( file_exists( $filename ) ) {


            $mtime = filemtime( $filename );
            $age = time() - $mtime;
            if ( $this->MAX_AGE > $age ) {

                return 'HIT';
            }
            else {

                return 'STALE';
            }
        }
        else {

            return 'MISS';
        }
    }

                                                                          \
                         
                                                                                
    function serialize ( $rss ) {
        return serialize( $rss );
    }

                                                                          \
                           
                                                                                
    function unserialize ( $data ) {
        return unserialize( $data );
    }
    
                                                                          \
                         
                                            
                                                      
                           
                                                                                
    function file_name ($url) {
        $filename = md5( $url );
        return join( DIRECTORY_SEPARATOR, array( $this->BASE_CACHE, $filename ) );
    }

                                                                          \
                     
                              
                                                                                    
    function error ($errormsg, $lvl=E_USER_WARNING) {

        if ( isset($php_errormsg) ) { 
            $errormsg .= " ($php_errormsg)";
        }
        $this->ERROR = $errormsg;
        if ( MAGPIE_DEBUG ) {
            trigger_error( $errormsg, $lvl);
        }
        else {
            error_log( $errormsg, 0);
        }
    }
    
    function debug ($debugmsg, $lvl=E_USER_NOTICE) {
        if ( MAGPIE_DEBUG ) {
            $this->error("MagpieRSS [debug] $debugmsg", $lvl);
        }
    }

}

?>
