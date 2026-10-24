<?php
/*
 * Project:     MagpieRSS: a simple RSS integration tool
 * File:        rss_fetch.inc, a simple functional interface
                to fetching and parsing RSS files, via the
                function fetch_rss()
 * Author:      Kellan Elliott-McCrea <kellan@protest.net>
 * License:     GPL
 *
 * The lastest version of MagpieRSS can be obtained from:
 * http://magpierss.sourceforge.net
 *
 * For questions, help, comments, discussion, etc., please join the
 * Magpie mailing list:
 * magpierss-general@lists.sourceforge.net
 *
 */
 



if (!defined('DIR_SEP')) {
    define('DIR_SEP', DIRECTORY_SEPARATOR);
}

if (!defined('MAGPIE_DIR')) {
    define('MAGPIE_DIR', dirname(__FILE__) . DIR_SEP);
}

require_once( MAGPIE_DIR . 'rss_parse.inc' );
require_once( MAGPIE_DIR . 'rss_cache.inc' );


define('MAGPIE_EXTLIB', MAGPIE_DIR . 'extlib' . DIR_SEP);
require_once( MAGPIE_EXTLIB . 'Snoopy.class.inc');


   
                                                          
                                                                    
  
                                                             
                                                                  
                                                  
  
  
                                                                   
                                                                        
                                                                          
                                                     
  
  
                                                                       
  
  
                                                               
                                     
  
                                            
  
  


                                                                          \
                         
                                                
                                
                             
                                                   

                        
                                                                            
    
                                     
                                                                         
                                                                       
        
    
                             
                                                                      
                                                                                
                                                                           

define('MAGPIE_VERSION', '0.7');

$MAGPIE_ERROR = "";

function fetch_rss ($url) {

    init();
    
    if ( !isset($url) ) {
        error("fetch_rss called without a url");
        return false;
    }
    

    if ( !MAGPIE_CACHE_ON ) {

        $resp = _fetch_remote_file( $url );
        if ( is_success( $resp->status ) ) {
            return _response_to_rss( $resp );
        }
        else {
            error("Failed to fetch $url and cache is off");
            return false;
        }
    } 

    else {





        
        $cache = new RSSCache( MAGPIE_CACHE_DIR, MAGPIE_CACHE_AGE );
        
        if (MAGPIE_DEBUG and $cache->ERROR) {
            debug($cache->ERROR, E_USER_WARNING);
        }
        
        
        $cache_status    = 0;        
        $request_headers = array();  
        $rss             = 0;        
        $errormsg        = 0;        
        


        $cache_key       = $url . MAGPIE_OUTPUT_ENCODING;
        
        if (!$cache->ERROR) {

            $cache_status = $cache->check_cache( $cache_key);
        }
        

        if ( $cache_status == 'HIT' ) {
            $rss = $cache->get( $cache_key );
            if ( isset($rss) and $rss ) {

                $rss->from_cache = 1;
                if ( MAGPIE_DEBUG > 1) {
                debug("MagpieRSS: Cache HIT", E_USER_NOTICE);
            }
                return $rss;
            }
        }
        

        

        if ( $cache_status == 'STALE' ) {
            $rss = $cache->get( $cache_key );
            if ( $rss and $rss->etag and $rss->last_modified ) {
                $request_headers['If-None-Match'] = $rss->etag;
                $request_headers['If-Last-Modified'] = $rss->last_modified;
            }
        }
        
        $resp = _fetch_remote_file( $url, $request_headers );
        
        if (isset($resp) and $resp) {
            if ($resp->status == '304' ) {

                if ( MAGPIE_DEBUG > 1) {
                    debug("Got 304 for $url");
                }

                $cache->set($cache_key, $rss);
                return $rss;
            }
            elseif ( is_success( $resp->status ) ) {
                $rss = _response_to_rss( $resp );
                if ( $rss ) {
                    if (MAGPIE_DEBUG > 1) {
                        debug("Fetch successful");
                    }

                    $cache->set( $cache_key, $rss );
                    return $rss;
                }
            }
            else {
                $errormsg = "Failed to fetch $url ";
                if ( $resp->status == '-100' ) {
                    $errormsg .= "(Request timed out after " . MAGPIE_FETCH_TIME_OUT . " seconds)";
                }
                elseif ( $resp->error ) {


                    $http_error = substr($resp->error, 0, -2); 
                    $errormsg .= "(HTTP Error: $http_error)";
                }
                else {
                    $errormsg .=  "(HTTP Response: " . $resp->response_code .')';
                }
            }
        }
        else {
            $errormsg = "Unable to retrieve RSS file for unknown reasons.";
        }
        

        

        if ($rss) {
            if ( MAGPIE_DEBUG ) {
                debug("Returning STALE object for $url");
            }
            return $rss;
        }
        

        error( $errormsg ); 
        
        return false;
        
    }  
}  

                                                                          \
                     
                                                   
                                                                           

function error ($errormsg, $lvl=E_USER_WARNING) {
        global $MAGPIE_ERROR;
        

        if ( isset($php_errormsg) ) { 
            $errormsg .= " ($php_errormsg)";
        }
        if ( $errormsg ) {
            $errormsg = "MagpieRSS: $errormsg";
            $MAGPIE_ERROR = $errormsg;
            trigger_error( $errormsg, $lvl);                
        }
}

function debug ($debugmsg, $lvl=E_USER_NOTICE) {
    trigger_error("MagpieRSS [debug] $debugmsg", $lvl);
}
            
                                                                          \
                            
                                                      
                                                                           
function magpie_error ($errormsg="") {
    global $MAGPIE_ERROR;
    
    if ( isset($errormsg) and $errormsg ) { 
        $MAGPIE_ERROR = $errormsg;
    }
    
    return $MAGPIE_ERROR;   
}

                                                                          \
                                  
                                                 
                                      
                                                                 
                                                                
                                                                           
function _fetch_remote_file ($url, $headers = "" ) {

    $client = new Snoopy();
    $client->agent = MAGPIE_USER_AGENT;
    $client->read_timeout = MAGPIE_FETCH_TIME_OUT;
    $client->use_gzip = MAGPIE_USE_GZIP;
    if (is_array($headers) ) {
        $client->rawheaders = $headers;
    }
    
    @$client->fetch($url);
    return $client;

}

                                                                          \
                                
                                                                
                                                    
                                                 
                                                                           
function _response_to_rss ($resp) {
    $rss = new MagpieRSS( $resp->results, MAGPIE_OUTPUT_ENCODING, MAGPIE_INPUT_ENCODING, MAGPIE_DETECT_ENCODING );
    

    if ( $rss and !$rss->ERROR) {
        

        foreach($resp->headers as $h) {

            if (strpos($h, ": ")) {
                list($field, $val) = explode(": ", $h, 2);
            }
            else {
                $field = $h;
                $val = "";
            }
            
            if ( $field == 'ETag' ) {
                $rss->etag = $val;
            }
            
            if ( $field == 'Last-Modified' ) {
                $rss->last_modified = $val;
            }
        }
        
        return $rss;    
    }  
    else {
        $errormsg = "Failed to parse RSS file.";
        
        if ($rss) {
            $errormsg .= " (" . $rss->ERROR . ")";
        }
        error($errormsg);
        
        return false;
    }  
}

                                                                          \
                    
                                                   
                                        
                                                                           
function init () {
    if ( defined('MAGPIE_INITALIZED') ) {
        return;
    }
    else {
        define('MAGPIE_INITALIZED', true);
    }
    
    if ( !defined('MAGPIE_CACHE_ON') ) {
        define('MAGPIE_CACHE_ON', true);
    }

    if ( !defined('MAGPIE_CACHE_DIR') ) {
        define('MAGPIE_CACHE_DIR', './cache');
    }

    if ( !defined('MAGPIE_CACHE_AGE') ) {
        define('MAGPIE_CACHE_AGE', 60*60);  
    }

    if ( !defined('MAGPIE_CACHE_FRESH_ONLY') ) {
        define('MAGPIE_CACHE_FRESH_ONLY', false);
    }

    if ( !defined('MAGPIE_OUTPUT_ENCODING') ) {
        define('MAGPIE_OUTPUT_ENCODING', 'ISO-8859-1');
    }
    
    if ( !defined('MAGPIE_INPUT_ENCODING') ) {
        define('MAGPIE_INPUT_ENCODING', null);
    }
    
    if ( !defined('MAGPIE_DETECT_ENCODING') ) {
        define('MAGPIE_DETECT_ENCODING', true);
    }
    
    if ( !defined('MAGPIE_DEBUG') ) {
        define('MAGPIE_DEBUG', 0);
    }
    
    if ( !defined('MAGPIE_USER_AGENT') ) {
        $ua = 'MagpieRSS/'. MAGPIE_VERSION . ' (+http://magpierss.sf.net';
        
        if ( MAGPIE_CACHE_ON ) {
            $ua = $ua . ')';
        }
        else {
            $ua = $ua . '; No cache)';
        }
        
        define('MAGPIE_USER_AGENT', $ua);
    }
    
    if ( !defined('MAGPIE_FETCH_TIME_OUT') ) {
        define('MAGPIE_FETCH_TIME_OUT', 5);  
    }
    

    if ( !defined('MAGPIE_USE_GZIP') ) {
        define('MAGPIE_USE_GZIP', true);    
    }
}




                                                                          \
                               
                                                           
                                   
    
                                                                           

                                                     
                                                                           


                                                                          \
                       
                                                        
                                                                           
function is_info ($sc) { 
    return $sc >= 100 && $sc < 200; 
}

                                                                          \
                          
                                                     
                                                                           
function is_success ($sc) { 
    return $sc >= 200 && $sc < 300; 
}

                                                                          \
                           
                                                      
                                                                           
function is_redirect ($sc) { 
    return $sc >= 300 && $sc < 400; 
}

                                                                          \
                        
                                                
                                                                           
function is_error ($sc) { 
    return $sc >= 400 && $sc < 600; 
}

                                                                          \
                               
                                                                        
                                                                           
function is_client_error ($sc) { 
    return $sc >= 400 && $sc < 500; 
}

                                                                          \
                               
                                                                        
                                                                           
function is_server_error ($sc) { 
    return $sc >= 500 && $sc < 600; 
}

?>
