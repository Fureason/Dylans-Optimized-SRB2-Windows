<?php
/**
 * $Id: SmartIRC.php,v 1.54.2.14 2005/05/27 23:40:09 meebey Exp $
 * $Revision: 1.54.2.14 $
 * $Author: meebey $
 * $Date: 2005/05/27 23:40:09 $
 *
 * Net_SmartIRC
 * This is a PHP class for communication with IRC networks,
 * which conforms to the RFC 2812 (IRC protocol).
 * It's an API that handles all IRC protocol messages.
 * This class is designed for creating IRC bots, chats and show irc related info on webpages.
 *
 * Documenation, a HOWTO and examples are in SmartIRC included.
 *
 * Here you will find a service bot which I am also developing
 * <http://cvs.meebey.net/atbs> and <http://cvs.meebey.net/phpbitch>
 * Latest versions of Net_SmartIRC you will find on the project homepage
 * or get it through PEAR since SmartIRC is an official PEAR package.
 * See <http://pear.php.net/Net_SmartIRC>.
 *
 * Official Projet Homepage: <http://sf.net/projects/phpsmartirc>
 *
 * Net_SmartIRC conforms to RFC 2812 (Internet Relay Chat: Client Protocol)
 * 
 * Copyright (c) 2002-2003 Mirco 'meebey' Bauer <meebey@meebey.net> <http://www.meebey.net>
 * 
 * Full LGPL License: <http://www.gnu.org/licenses/lgpl.txt>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

include_once('SmartIRC/defines.php');
include_once('SmartIRC/irccommands.php');
include_once('SmartIRC/messagehandler.php');
define('SMARTIRC_VERSION', '1.0.0 ($Revision: 1.54.2.14 $)');
define('SMARTIRC_VERSIONSTRING', 'Net_SmartIRC '.SMARTIRC_VERSION);

   
                      
  
                        
                 
                                                 
                 
   
class Net_SmartIRC_base
{
       
                    
                      
       
    var $_socket;
    
       
                  
                      
       
    var $_address;
    
       
                   
                      
       
    var $_port;
    
       
                  
                      
       
    var $_nick;
    
       
                  
                      
       
    var $_username;
    
       
                  
                      
       
    var $_realname;
    
       
                  
                      
       
    var $_usermode;
    
       
                  
                      
       
    var $_password;
    
       
                   
                      
       
    var $_state = false;
    
       
                 
                      
       
    var $_actionhandler = array();
    
       
                 
                      
       
    var $_timehandler = array();
    
       
                   
                      
       
    var $_debug = SMARTIRC_DEBUG_NOTICE;
    
       
                 
                      
       
    var $_messagebuffer = array();
    
       
                   
                      
       
    var $_messagebuffersize;
    
       
                   
                      
       
    var $_usesockets = false;
    
       
                   
                      
       
    var $_receivedelay = 100;
    
       
                   
                      
       
    var $_senddelay = 250;
    
       
                   
                      
       
    var $_logdestination = SMARTIRC_STDOUT;
    
       
                    
                      
       
    var $_logfilefp = 0;
    
       
                  
                      
       
    var $_logfile = 'Net_SmartIRC.log';
    
       
                   
                      
       
    var $_disconnecttime = 1000;
    
       
                   
                      
       
    var $_loggedin = false;
    
       
                   
                      
       
    var $_benchmark = false;
    
       
                   
                      
       
    var $_benchmark_starttime;
    
       
                   
                      
       
    var $_benchmark_stoptime;
    
       
                   
                      
       
    var $_actionhandlerid = 0;
    
       
                   
                      
       
    var $_timehandlerid = 0;
    
       
                 
                      
       
    var $_motd = array();
    
       
                 
                      
       
    var $_channels = array();
    
       
                   
                      
       
    var $_channelsyncing = false;
    
       
                  
                      
       
    var $_ctcpversion;
    
       
                 
                      
       
    var $_mintimer = false;
    
       
                   
                      
       
    var $_maxtimer = 300000;
    
       
                   
                      
       
    var $_txtimeout = 300;
    
       
                   
                      
       
    var $_rxtimeout = 300;
    
       
                   
                      
       
    var $_selecttimeout;
    
       
                   
                      
       
    var $_lastrx;
    
       
                   
                      
       
    var $_lasttx;
    
       
                   
                      
       
    var $_autoreconnect = false;
    
       
                   
                      
       
    var $_autoretry = false;
    
       
                                                           
      
                                
                 
                     
       
    var $replycodes;
    
       
                                                                      
      
                                 
                 
                     
       
    var $nreplycodes;
    
       
                                                                                                        
                                                                                                          
                                                     
      
                               
                                
                                    
                 
                     
       
    var $channel;
    
       
                                                                               
                                                                             
      
                     
                   
       
    function Net_SmartIRC_base()
    {

        $this->_checkPHPVersion();
        
        ob_implicit_flush(true);
        @set_time_limit(0);
        ignore_user_abort(true);
        $this->_messagebuffer[SMARTIRC_CRITICAL] = array();
        $this->_messagebuffer[SMARTIRC_HIGH] = array();
        $this->_messagebuffer[SMARTIRC_MEDIUM] = array();
        $this->_messagebuffer[SMARTIRC_LOW] = array();
        $this->replycodes = &$GLOBALS['SMARTIRC_replycodes'];
        $this->nreplycodes = &$GLOBALS['SMARTIRC_nreplycodes'];
        

        $this->channel = &$this->_channels;

        $this->user = &$this->_users;
        
        if (isset($_SERVER['REQUEST_METHOD'])) {


            $this->setLogdestination(SMARTIRC_BROWSEROUT);
        }
    }
    
       
                                                  
      
                                                                   
                                                                         
                     
      
                           
                   
                     
       
    function setUseSockets($boolean)
    {
        if ($boolean === true) {
            if (@extension_loaded('sockets')) {
                $this->_usesockets = true;
            } else {
                $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: socket extension not loaded, trying to load it...', __FILE__, __LINE__);
                
                if (strtoupper(substr(PHP_OS, 0,3) == 'WIN')) {
                    $load_status = @dl('php_sockets.dll');
                } else {
                    $load_status = @dl('sockets.so');
                }
 
                if ($load_status) {
                    $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: socket extension succesfully loaded', __FILE__, __LINE__);
                    $this->_usesockets = true;
                } else {
                    $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: couldn\'t load the socket extension', __FILE__, __LINE__);
                    $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: your PHP build doesn\'t support real sockets, will use fsocks instead', __FILE__, __LINE__);
                    $this->_usesockets = false;
                }
            }
        } else {
            $this->_usesockets = false;
        }
    }
    
       
                                        
      
                                                                               
                                                  
                          
                            
                                
                            
                                 
                                  
                                   
                                 
                                    
                                    
                             
                                 
                         
      
                                     
      
                         
                                 
                            
                   
                     
       
    function setDebug($level)
    {
        $this->_debug = $level;
    }
    
       
                                             
       
                              
                   
                     
       
    function setBenchmark($boolean)
    {
        if (is_bool($boolean)) {
            $this->_benchmark = $boolean;
        } else {
            $this->_benchmark = false;
        }
    }
    
       
                                                   
      
                  
                              
                   
                     
       
    function setChannelSynching($boolean)
    {
        $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: you are using setChannelSynching() which is a deprecated method, use setChannelSyncing() instead!', __FILE__, __LINE__);
        $this->setChannelSyncing($boolean);
    }
    
       
                                        
      
                                                                                       
                                                             
       
                              
                   
                     
       
    function setChannelSyncing($boolean)
    {
        if (is_bool($boolean)) {
            $this->_channelsyncing = $boolean;
        } else {
            $this->_channelsyncing = false;
        }
        
        if ($this->_channelsyncing == true) {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: Channel syncing enabled', __FILE__, __LINE__);
        } else {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: Channel syncing disabled', __FILE__, __LINE__);
        }
    }
    
       
                                          
       
                                   
                   
                     
       
    function setCtcpVersion($versionstring)
    {
        $this->_ctcpversion = $versionstring;
    }
    
       
                                                
      
                                            
                    
                                                   
                                                    
                                                        
                               
      
                           
                                                       
                   
                     
       
    function setLogdestination($type)
    {
        switch ($type) {
            case (SMARTIRC_FILE ||
                  SMARTIRC_STDOUT ||
                  SMARTIRC_SYSLOG ||
                  SMARTIRC_BROWSEROUT ||
                  SMARTIRC_NONE):
                $this->_logdestination = $type;
            break;
            default:
                $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: unknown logdestination type ('.$type.'), will use STDOUT instead', __FILE__, __LINE__);
                $this->_logdestination = SMARTIRC_STDOUT;
        }
    }
    
       
                                                                   
      
                                                                                             
                                               
      
                           
                   
                     
       
    function setLogfile($file)
    {
        $this->_logfile = $file;
    }
    
       
                                                                    
      
                                   
                   
                     
       
    function setDisconnecttime($milliseconds)
    {
        if (is_integer($milliseconds) && $milliseconds >= 100) {
            $this->_disconnecttime = $milliseconds;
        } else {
            $this->_disconnecttime = 100;
        }
    }
    
       
                                                             
      
                                                                                         
                                          
                   
      
                                   
                   
                     
       
    function setReceivedelay($milliseconds)
    {
        if (is_integer($milliseconds) && $milliseconds >= 100) {
            $this->_receivedelay = $milliseconds;
        } else {
            $this->_receivedelay = 100;
        }
    }
    
       
                                                         
      
                                                                                                  
                                                                        
                   
      
                                   
                   
                     
       
    function setSenddelay($milliseconds)
    {
        if (is_integer($milliseconds)) {
            $this->_senddelay = $milliseconds;
        } else {
            $this->_senddelay = 250;
        }
    }
    
       
                                         
       
                              
                   
                     
       
    function setAutoReconnect($boolean)
    {
        if (is_bool($boolean)) {
            $this->_autoreconnect = $boolean;
        } else {
            $this->_autoreconnect = false;
        }
    }
    
       
                                                             
       
                              
                   
                     
       
    function setAutoRetry($boolean)
    {
        if (is_bool($boolean)) {
            $this->_autoretry = $boolean;
        } else {
            $this->_autoretry = false;
        }
    }
    
       
                                
      
                                                                  
                           
      
                              
                   
                     
       
    function setReceiveTimeout($seconds)
    {
        if (is_integer($seconds)) {
            $this->_rxtimeout = $seconds;
        } else {
            $this->_rxtimeout = 300;
        }
    }
    
       
                                 
      
                                                                  
                           
      
                              
                   
                     
       
    function setTransmitTimeout($seconds)
    {
        if (is_integer($seconds)) {
            $this->_txtimeout = $seconds;
        } else {
            $this->_txtimeout = 300;
        }
    }
    
       
                                                
      
                   
                     
       
    function startBenchmark()
    {
        $this->_benchmark_starttime = $this->_microint();
        $this->log(SMARTIRC_DEBUG_NOTICE, 'benchmark started', __FILE__, __LINE__);
    }
    
       
                                                   
      
                   
                     
       
    function stopBenchmark()
    {
        $this->_benchmark_stoptime = $this->_microint();
        $this->log(SMARTIRC_DEBUG_NOTICE, 'benchmark stopped', __FILE__, __LINE__);
        
        if ($this->_benchmark) {
            $this->showBenchmark();
        }
    }
    
       
                                  
      
                   
                     
       
    function showBenchmark()
    {
        $this->log(SMARTIRC_DEBUG_NOTICE, 'benchmark time: '.((float)$this->_benchmark_stoptime-(float)$this->_benchmark_starttime), __FILE__, __LINE__);
    }
    
       
                                
      
                                                            
                                                                 
                          
                            
                                
                            
                                 
                                  
                                   
                                 
                                    
                                    
                             
                                 
                         
      
                                 
                                                             
                                             
                   
                     
       
    function log($level, $entry, $file = null, $line = null)
    {

        if (!(is_integer($level)) ||
            !($level & SMARTIRC_DEBUG_ALL)) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: invalid log level passed to log() ('.$level.')', __FILE__, __LINE__);
            return;
        }
        
        if (!($level & $this->_debug) ||
             ($this->_logdestination == SMARTIRC_NONE)) {
            return;
        }
        
        if (substr($entry, -1) != "\n") {
            $entry .= "\n";
        }
        
        if ($file !== null &&
            $line !== null) {
            $file = basename($file);
            $entry = $file.'('.$line.') '.$entry;
        } else {
            $entry = 'unknown(0) '.$entry;
        }
        
        $formatedentry = date('M d H:i:s ').$entry;
        switch ($this->_logdestination) {
            case SMARTIRC_STDOUT:
                echo $formatedentry;
                flush();
            break;
            case SMARTIRC_BROWSEROUT:
                echo '<pre>'.htmlentities($formatedentry).'</pre>';
            break;
            case SMARTIRC_FILE:
                if (!is_resource($this->_logfilefp)) {
                    if ($this->_logfilefp === null) {

                        $this->_logfilefp = @fopen($this->_logfile,'a');
                    } else {
                        $this->_logfilefp = @fopen($this->_logfile,'w');
                    }
                }
                @fwrite($this->_logfilefp, $formatedentry);
                fflush($this->_logfilefp);
            break;
            case SMARTIRC_SYSLOG:
                define_syslog_variables();
                if (!is_int($this->_logfilefp)) {
                    $this->_logfilefp = openlog('Net_SmartIRC', LOG_NDELAY, LOG_DAEMON);
                }
                syslog(LOG_INFO, $entry);
            break;
        }
    }
    
       
                             
      
                    
                     
       
    function getMotd()
    {
        return $this->_motd;
    }
    
       
                            
      
                     
                     
       
    function getUsermode()
    {
        return $this->_usermode;
    }
    
       
                                                                            
      
                              
                           
                   
                     
       
    function connect($address, $port)
    {
        $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: connecting', __FILE__, __LINE__);
        $this->_address = $address;
        $this->_port = $port;
        
        if ($this->_usesockets == true) {
            $this->log(SMARTIRC_DEBUG_SOCKET, 'DEBUG_SOCKET: using real sockets', __FILE__, __LINE__);
            $this->_socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
            $result = @socket_connect($this->_socket, $this->_address, $this->_port);
        } else {
            $this->log(SMARTIRC_DEBUG_SOCKET, 'DEBUG_SOCKET: using fsockets', __FILE__, __LINE__);
            $result = @fsockopen($this->_address, $this->_port, $errno, $errstr);
        }
        
        if ($result === false) {
            if ($this->_usesockets == true) {
                $error = socket_strerror(socket_last_error($this->_socket));
            } else {
                $error = $errstr.' ('.$errno.')';
            }
            
            $error_msg = 'couldn\'t connect to "'.$address.'" reason: "'.$error.'"';
            $this->log(SMARTIRC_DEBUG_NOTICE, 'DEBUG_NOTICE: '.$error_msg, __FILE__, __LINE__);

            $this->throwError($error_msg);
            

            static $tries = 0;
            if ($this->_autoretry == true && $tries < 5) {
                $this->reconnect();
                $tries++;
            } else {
                die();
            }
        } else {
            $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: connected', __FILE__, __LINE__);
            
            if ($this->_usesockets != true) {
                $this->_socket = $result;
                $this->log(SMARTIRC_DEBUG_SOCKET, 'DEBUG_SOCKET: activating nonblocking fsocket mode', __FILE__, __LINE__);
                socket_set_blocking($this->_socket, false);
            }
        }
        
        $this->_lastrx = time();
        $this->_lasttx = $this->_lastrx;
        $this->_updatestate();
        
        if ($result !== false) {
            return true;
        } else {
            return false;
        }
    }
    
       
                                                                                      
      
                                                                   
                        
                                  
                                                                                                  
      
                                                     
                      
                     
       
    function disconnect($quickdisconnect = false)
    {
        if ($this->_state() == SMARTIRC_STATE_CONNECTED) {
            if ($quickdisconnect == false) {
                $this->_send('QUIT', SMARTIRC_CRITICAL);
                usleep($this->_disconnecttime*1000);
            }
            
            if ($this->_usesockets == true) {
                @socket_shutdown($this->_socket);
                @socket_close($this->_socket);
            } else {
                fclose($this->_socket);
            }
            
            $this->_updatestate();
            $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: disconnected', __FILE__, __LINE__);
        } else {
            return false;
        }
        
        if ($this->_channelsyncing == true) {

            $this->_channels = array();
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: cleaned channel array', __FILE__, __LINE__);
        }
        
        if ($this->_logdestination == SMARTIRC_FILE) {
            fclose($this->_logfilefp);
            $this->_logfilefp = null;
        } else if ($this->_logdestination == SMARTIRC_SYSLOG) {
            closelog();
        }
        
        return true;
    }
    
       
                                                             
                                   
      
                   
                     
       
    function reconnect()
    {
        $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: reconnecting...', __FILE__, __LINE__);
        

        $channels = array();
        foreach ($this->_channels as $value) {
            if (empty($value->key)) {
                $channels[] = array('name' => $value->name);
            } else {
                $channels[] = array('name' => $value->name, 'key' => $value->key);
            }
        }
        
        $this->disconnect(true);
        $this->connect($this->_address, $this->_port);
        $this->login($this->_nick, $this->_realname, $this->_usermode, $this->_username, $this->_password);
        

        foreach ($channels as $value) {
            if (isset($value['key'])) {
                $this->join($value['name'], $value['key']);
            } else {
                $this->join($value['name']);
            }
        }
    }
    
       
                                                     
      
                                                                      
      
                          
                              
                               
                              
                              
                   
                     
       
    function login($nick, $realname, $usermode = 0, $username = null, $password = null)
    {
        $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: logging in', __FILE__, __LINE__);
        
        $this->_nick = str_replace(' ', '', $nick);
        $this->_realname = $realname;
        
        if ($username !== null) {
            $this->_username = str_replace(' ', '', $username);
        } else {
            $this->_username = str_replace(' ', '', exec('whoami'));
        }
        
        if ($password !== null) {
            $this->_password = $password;
            $this->_send('PASS '.$this->_password, SMARTIRC_CRITICAL);
        }
        
        if (!is_numeric($usermode)) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'DEBUG_NOTICE: login() usermode ('.$usermode.') is not valid, will use 0 instead', __FILE__, __LINE__);
            $usermode = 0;
        }
        
        $this->_send('NICK '.$this->_nick, SMARTIRC_CRITICAL);
        $this->_send('USER '.$this->_username.' '.$usermode.' '.SMARTIRC_UNUSED.' :'.$this->_realname, SMARTIRC_CRITICAL);
    }
    

    
       
                                                                                               
                                           
      
                             
                             
                              
                      
                     
       
    function isJoined($channel, $nickname = null)
    {
        if ($this->_channelsyncing != true) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: isJoined() is called and the required Channel Syncing is not activated!', __FILE__, __LINE__);
            return false;
        }
        
        if ($nickname === null) {
            $nickname = $this->_nick;
        }
        
        if (isset($this->_channels[strtolower($channel)]->users[strtolower($nickname)])) {
            return true;
        }
        
        return false;
    }
    
       
                                                                                               
                                           
      
                             
                             
                              
                      
                     
       
    function isOpped($channel, $nickname = null)
    {
        if ($this->_channelsyncing != true) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: isOpped() is called and the required Channel Syncing is not activated!', __FILE__, __LINE__);
            return false;
        }
        
        if ($nickname === null) {
            $nickname = $this->_nick;
        }
        
        if ($this->isJoined($channel, $nickname)) {
            if ($this->_channels[strtolower($channel)]->users[strtolower($nickname)]->op) {
                return true;
            }
        }
        
        return false;
    }
    
       
                                                                                                
                                           
      
                             
                             
                              
                      
                     
       
    function isVoiced($channel, $nickname = null)
    {
        if ($this->_channelsyncing != true) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: isVoiced() is called and the required Channel Syncing is not activated!', __FILE__, __LINE__);
            return false;
        }
        
        if ($nickname === null) {
            $nickname = $this->_nick;
        }
        
        if ($this->isJoined($channel, $nickname)) {
            if ($this->_channels[strtolower($channel)]->users[strtolower($nickname)]->voice) {
                return true;
            }
        }
        
        return false;
    }
    
       
                                                                                        
                                           
      
                             
                             
                              
                      
                     
       
    function isBanned($channel, $hostmask)
    {
        if ($this->_channelsyncing != true) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: isBanned() is called and the required Channel Syncing is not activated!', __FILE__, __LINE__);
            return false;
        }
        
        if ($this->isJoined($channel)) {
            $result = array_search($hostmask, $this->_channels[strtolower($channel)]->bans);
            
            if ($result !== false) {
                return true;
            }
        }
        
        return false;
    }

       
                             
      
                                                                                      
                                                                                            
      
                      
                     
       
    function listen()
    {
        if ($this->_state() == SMARTIRC_STATE_CONNECTED) {
            $this->_rawreceive();
            return true;
        } else {
            return false;
        }
    }
    
       
                                                                      
      
                                                                                  
                                                                                                      
                                                                                          
      
                                                                           
                                                                     
                     
       
    function listenFor($messagetype)
    {
        $listenfor = &new Net_SmartIRC_listenfor();
        $this->registerActionhandler($messagetype, '.*', $listenfor, 'handler');
        $this->listen();
        $result = $listenfor->result;
        
        if (isset($listenfor)) {
            unset($listenfor);
        }
        
        return $result;
    }
    
       
                                                                      
      
                                                                                  
                                                                                                      
                                                                                          
                                                                                                 
      
                                                                           
                                                                     
                     
       
    function objListenFor($messagetype)
    {
        $objlistenfor = &new Net_SmartIRC_objListenFor();
        $this->registerActionhandler($messagetype, '.*', $objlistenfor, 'handler');
        $this->listen();
        $result = $objlistenfor->result;
        
        if (isset($objlistenfor)) {
            unset($objlistenfor);
        }
        
        return $result;
    }    
    
       
                                                                
      
                                                                       
                                                                          
      
                       
                                                                                          
                                                                                                
                                                                     
                                                                                            
                                                
                     
       
    function registerActionhandler($handlertype, $regexhandler, &$object, $methodname)
    {

        if (!$this->_isValidType($handlertype)) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: passed invalid handlertype to registerActionhandler()', __FILE__, __LINE__);
            return false;
        }
        
        $id = $this->_actionhandlerid++;
        $newactionhandler = &new Net_SmartIRC_actionhandler();
        
        $newactionhandler->id = $id;
        $newactionhandler->type = $handlertype;
        $newactionhandler->message = $regexhandler;
        $newactionhandler->object = &$object;
        $newactionhandler->method = $methodname;
        
        $this->_actionhandler[] = &$newactionhandler;
        $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: actionhandler('.$id.') registered', __FILE__, __LINE__);
        return $id;
    }
    
       
                                            
      
                                  
                                  
                            
                                
                      
                     
       
    function unregisterActionhandler($handlertype, $regexhandler, &$object, $methodname)
    {

        if (!$this->_isValidType($handlertype)) {
            $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: passed invalid handlertype to unregisterActionhandler()', __FILE__, __LINE__);
            return false;
        }
        
        $handler = &$this->_actionhandler;
        $handlercount = count($handler);
        
        for ($i = 0; $i < $handlercount; $i++) {
            $handlerobject = &$handler[$i];
                        
            if ($handlerobject->type == $handlertype &&
                $handlerobject->message == $regexhandler &&
                $handlerobject->method == $methodname) {
                
                $id = $handlerobject->id;
                
                if (isset($this->_actionhandler[$i])) {
                    unset($this->_actionhandler[$i]);
                }
                
                $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: actionhandler('.$id.') unregistered', __FILE__, __LINE__);
                $this->_reorderactionhandler();
                return true;
            }
        }
        
        $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: could not find actionhandler type: "'.$handlertype.'" message: "'.$regexhandler.'" method: "'.$methodname.'" from object "'.get_class($object).'" _not_ unregistered', __FILE__, __LINE__);
        return false;
    }
    
       
                                                       
      
                         
                      
                     
       
    function unregisterActionid($id)
    {
        $handler = &$this->_actionhandler;
        $handlercount = count($handler);
        for ($i = 0; $i < $handlercount; $i++) {
            $handlerobject = &$handler[$i];
                        
            if ($handlerobject->id == $id) {
                if (isset($this->_actionhandler[$i])) {
                    unset($this->_actionhandler[$i]);
                }
                
                $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: actionhandler('.$id.') unregistered', __FILE__, __LINE__);
                $this->_reorderactionhandler();
                return true;
            }
        }
        
        $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: could not find actionhandler id: '.$id.' _not_ unregistered', __FILE__, __LINE__);
        return false;
    }
    
       
                                                          
      
                                                                                               
                                                                      
      
                        
                                                             
                                                                     
                                                                                            
                                              
                     
       
    function registerTimehandler($interval, &$object, $methodname)
    {
        $id = $this->_timehandlerid++;
        $newtimehandler = &new Net_SmartIRC_timehandler();
        
        $newtimehandler->id = $id;
        $newtimehandler->interval = $interval;
        $newtimehandler->object = &$object;
        $newtimehandler->method = $methodname;
        $newtimehandler->lastmicrotimestamp = $this->_microint();
        
        $this->_timehandler[] = &$newtimehandler;
        $this->log(SMARTIRC_DEBUG_TIMEHANDLER, 'DEBUG_TIMEHANDLER: timehandler('.$id.') registered', __FILE__, __LINE__);
        
        if (($interval < $this->_mintimer) || ($this->_mintimer == false)) {
            $this->_mintimer = $interval;
        }
            
        return $id;
    }
    
       
                                                     
      
                        
                         
                      
                     
       
    function unregisterTimeid($id)
    {
        $handler = &$this->_timehandler;
        $handlercount = count($handler);
        for ($i = 0; $i < $handlercount; $i++) {
            $handlerobject = &$handler[$i];
            
            if ($handlerobject->id == $id) {
                if (isset($this->_timehandler[$i])) {
                    unset($this->_timehandler[$i]);
                }
                
                $this->log(SMARTIRC_DEBUG_TIMEHANDLER, 'DEBUG_TIMEHANDLER: timehandler('.$id.') unregistered', __FILE__, __LINE__);
                $this->_reordertimehandler();
                $this->_updatemintimer();
                return true;
            }
        }
        
        $this->log(SMARTIRC_DEBUG_TIMEHANDLER, 'DEBUG_TIMEHANDLER: could not find timehandler id: '.$id.' _not_ unregistered', __FILE__, __LINE__);
        return false;
    }
    

       
                                                                             
      
                   
                      
       
    function _nicknameinuse()
    {
        $newnickname = substr($this->_nick, 0, 5).rand(0, 999);
        $this->changeNick($newnickname, SMARTIRC_CRITICAL);
    }
    
       
                           
      
                                                                      
                 
                        
                    
                      
                   
      
                          
                                                                     
                      
                      
       
    function _send($data, $priority = SMARTIRC_MEDIUM)
    {
        switch ($priority) {
            case SMARTIRC_CRITICAL:
                $this->_rawsend($data);
                return true;
            break;
            case (SMARTIRC_HIGH||
                  SMARTIRC_MEDIUM||
                  SMARTIRC_LOW):
                $this->_messagebuffer[$priority][] = $data;
                return true;
            break;
            default:
                $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: message ('.$data.') with an invalid priority passed ('.$priority.'), message is ignored!', __FILE__, __LINE__);
                return false;
        }
    }
    
       
                                                      
      
                   
                      
       
    function _checkbuffer()
    {
        if (!$this->_loggedin) {
            return;
        }
        
        static $highsent = 0;
        static $lastmicrotimestamp = 0;
        
        if ($lastmicrotimestamp == 0) {
            $lastmicrotimestamp = $this->_microint();
        }
        
        $highcount = count($this->_messagebuffer[SMARTIRC_HIGH]);
        $mediumcount = count($this->_messagebuffer[SMARTIRC_MEDIUM]);
        $lowcount = count($this->_messagebuffer[SMARTIRC_LOW]);
        $this->_messagebuffersize = $highcount+$mediumcount+$lowcount;
        

        if ($this->_microint() >= ($lastmicrotimestamp+($this->_senddelay/1000))) {
            if ($highcount > 0 && $highsent <= 2) {
                $this->_rawsend(array_shift($this->_messagebuffer[SMARTIRC_HIGH]));
                $lastmicrotimestamp = $this->_microint();
                $highsent++;
            } else if ($mediumcount > 0) {
                $this->_rawsend(array_shift($this->_messagebuffer[SMARTIRC_MEDIUM]));
                $lastmicrotimestamp = $this->_microint();
                $highsent = 0;
            } else if ($lowcount > 0) {
                $this->_rawsend(array_shift($this->_messagebuffer[SMARTIRC_LOW]));
                $lastmicrotimestamp = $this->_microint();
            }
        }
    }
    
       
                                                                      
                                    
      
                   
                      
       
    function _checktimer()
    {
        if (!$this->_loggedin) {
            return;
        }
        

        for ($i = 0; $i < count($this->_timehandler); $i++) {
            $handlerobject = &$this->_timehandler[$i];
            $microtimestamp = $this->_microint();
            if ($microtimestamp >= ($handlerobject->lastmicrotimestamp+($handlerobject->interval/1000))) {
                $methodobject = &$handlerobject->object;
                $method = $handlerobject->method;
                $handlerobject->lastmicrotimestamp = $microtimestamp;
                
                if (@method_exists($methodobject, $method)) {
                    $this->log(SMARTIRC_DEBUG_TIMEHANDLER, 'DEBUG_TIMEHANDLER: calling method "'.get_class($methodobject).'->'.$method.'"', __FILE__, __LINE__);
                    $methodobject->$method($this);
                }
            }
        }
    }
    
       
                                                                                   
      
                   
                      
       
    function _checktimeout()
    {
        if ($this->_autoreconnect == true) {
            $timestamp = time();
            if ($this->_lastrx < ($timestamp - $this->_rxtimeout)) {
                $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: receive timeout detected, doing reconnect...', __FILE__, __LINE__);
                $this->reconnect();
            } else if ($this->_lasttx < ($timestamp - $this->_txtimeout)) {
                $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: transmit timeout detected, doing reconnect...', __FILE__, __LINE__);
                $this->reconnect();
            }
        }
    }
    
       
                                                               
      
                                        
      
                          
                      
                      
       
    function _rawsend($data)
    {
        if ($this->_state() == SMARTIRC_STATE_CONNECTED) {
            $this->log(SMARTIRC_DEBUG_IRCMESSAGES, 'DEBUG_IRCMESSAGES: sent: "'.$data.'"', __FILE__, __LINE__);
            
            if ($this->_usesockets == true) {
                $result = @socket_write($this->_socket, $data.SMARTIRC_CRLF);
            } else {
                $result = @fwrite($this->_socket, $data.SMARTIRC_CRLF);
            }
            
            
            if ($result === false) {
                return false;
            } else {
                $this->_lasttx = time();
                return true;
            }
        } else {
            return false;
        }
    }
    
       
                                                                        
      
                   
                      
       
    function _rawreceive()
    {
        $lastpart = '';
        $rawdataar = array();
        
        while ($this->_state() == SMARTIRC_STATE_CONNECTED) {
            $this->_checkbuffer();
            
            $timeout = $this->_selecttimeout();
            if ($this->_usesockets == true) {
                $sread = array($this->_socket);
                $result = @socket_select($sread, $w = null, $e = null, 0, $timeout*1000);
                
                if ($result == 1) {

                    $rawdata = @socket_read($this->_socket, 10240);
                } else if ($result === false) {

                    $this->log(SMARTIRC_DEBUG_NOTICE, 'WARNING: socket_select() returned false, something went wrong! Reason: '.socket_strerror(socket_last_error()), __FILE__, __LINE__);
                    exit;
                } else {

                    $rawdata = null;
                }
            } else {
                usleep($this->_receivedelay*1000);
                $rawdata = @fread($this->_socket, 10240);
            }
            
            $this->_checktimer();
            $this->_checktimeout();
            
            if ($rawdata !== null && !empty($rawdata)) {
                $this->_lastrx = time();
                $rawdata = str_replace("\r", '', $rawdata);
                $rawdata = $lastpart.$rawdata;
                
                $lastpart = substr($rawdata, strrpos($rawdata ,"\n")+1);
                $rawdata = substr($rawdata, 0, strrpos($rawdata ,"\n"));
                $rawdataar = explode("\n", $rawdata);
            }
            

            while (count($rawdataar) > 0) {
                $rawline = array_shift($rawdataar);
                $validmessage = false;
                
                $this->log(SMARTIRC_DEBUG_IRCMESSAGES, 'DEBUG_IRCMESSAGES: received: "'.$rawline.'"', __FILE__, __LINE__);
                

                $ircdata = &new Net_SmartIRC_data();
                $ircdata->rawmessage = $rawline;
                $lineex = explode(' ', $rawline);
                $ircdata->rawmessageex = $lineex;
                $messagecode = $lineex[0];
                
                if (substr($rawline, 0, 1) == ':') {
                    $validmessage = true;
                    $line = substr($rawline, 1);
                    $lineex = explode(' ', $line);
                    

                    $from = $lineex[0];
                    $messagecode = $lineex[1];
                    $exclamationpos = strpos($from, '!');
                    $atpos = strpos($from, '@');
                    $colonpos = strpos($line, ' :');
                    if ($colonpos !== false) {

                        $colonpos += 1;
                    }
                    $ircdata->nick = substr($from, 0, $exclamationpos);
                    $ircdata->ident = substr($from, $exclamationpos+1, ($atpos-$exclamationpos)-1);
                    $ircdata->host = substr($from, $atpos+1);
                    $ircdata->type = $this->_gettype($rawline);
                    $ircdata->from = $from;
                    if ($colonpos !== false) {
                        $ircdata->message = substr($line, $colonpos+1);
                        $ircdata->messageex = explode(' ', $ircdata->message);
                    }
                    
                    if ($ircdata->type & (SMARTIRC_TYPE_CHANNEL|
                                 SMARTIRC_TYPE_ACTION|
                                 SMARTIRC_TYPE_MODECHANGE|
                                 SMARTIRC_TYPE_KICK|
                                 SMARTIRC_TYPE_PART|
                                 SMARTIRC_TYPE_JOIN)) {
                        $ircdata->channel = $lineex[2];
                    } else if ($ircdata->type & (SMARTIRC_TYPE_WHO|
                                        SMARTIRC_TYPE_BANLIST|
                                        SMARTIRC_TYPE_TOPIC|
                                        SMARTIRC_TYPE_CHANNELMODE)) {
                        $ircdata->channel = $lineex[3];
                    } else if ($ircdata->type & SMARTIRC_TYPE_NAME) {
                        $ircdata->channel = $lineex[4];
                    }
                    
                    if ($ircdata->channel !== null) {
                        if (substr($ircdata->channel, 0, 1) == ':') {
                            $ircdata->channel = substr($ircdata->channel, 1);
                        }
                    }
                    
                    $this->log(SMARTIRC_DEBUG_MESSAGEPARSER, 'DEBUG_MESSAGEPARSER: ircdata nick: "'.$ircdata->nick.
                                                                '" ident: "'.$ircdata->ident.
                                                                '" host: "'.$ircdata->host.
                                                                '" type: "'.$ircdata->type.
                                                                '" from: "'.$ircdata->from.
                                                                '" channel: "'.$ircdata->channel.
                                                                '" message: "'.$ircdata->message.
                                                                '"', __FILE__, __LINE__);
                }
                

                $this->_handlemessage($messagecode, $ircdata);
                    
                if ($validmessage == true) {

                    $this->_handleactionhandler($ircdata);
                }
                
                if (isset($ircdata)) {
                    unset($ircdata);
                }
            }
        }
    }
    
       
                                       
      
                                                                      
      
                          
                   
                      
       
    function _pong($data)
    {
        $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: Ping? Pong!', __FILE__, __LINE__);
        $this->_send('PONG '.$data, SMARTIRC_CRITICAL);
    }
    
       
                                                 
      
                                                    
                      
       
    function _selecttimeout() {
        if ($this->_messagebuffersize == 0) {
            $this->_selecttimeout = null;
            
            if ($this->_mintimer != false) {
                $this->_calculateselecttimeout($this->_mintimer);
            }
            
            if ($this->_autoreconnect == true) {
                $this->_calculateselecttimeout($this->_rxtimeout*1000);
            }
            
            $this->_calculateselecttimeout($this->_maxtimer);
            return $this->_selecttimeout;
        } else {
            return $this->_senddelay;
        }
    }
    
       
                                         
      
                   
                      
       
    function _calculateselecttimeout($microseconds)
    {
        if (($this->_selecttimeout > $microseconds) || $this->_selecttimeout === null) {
            $this->_selecttimeout = $microseconds;
        }
    }
    
       
                                                       
      
                   
                      
       
    function _updatemintimer()
    {
        $timerarray = array();
        foreach ($this->_timehandler as $values) {
            $timerarray[] = $values->interval;
        }
        
        $result = array_multisort($timerarray, SORT_NUMERIC, SORT_ASC);
        if ($result == true && isset($timerarray[0])) {
            $this->_mintimer = $timerarray[0];
        } else {
            $this->_mintimer = false;
        }
    }
    
       
                                                                  
      
                   
                      
       
    function _reorderactionhandler()
    {
        $orderedactionhandler = array();
        foreach ($this->_actionhandler as $value) {
            $orderedactionhandler[] = $value;
        }
        $this->_actionhandler = &$orderedactionhandler;
    }
    
       
                                                                
      
                   
                      
       
    function _reordertimehandler()
    {
        $orderedtimehandler = array();
        foreach ($this->_timehandler as $value) {
            $orderedtimehandler[] = $value;
        }
        $this->_timehandler = &$orderedtimehandler;
    }
    
       
                                                            
      
                   
                      
       
    function _reordermodules()
    {
        $orderedmodules = array();
        foreach ($this->_modules as $value) {
            $orderedmodules[] = $value;
        }
        $this->_modules = &$orderedmodules;
    }

       
                                          
      
                                                                
      
                          
                                               
                      
       
    function _gettype($line)
    {
        if (preg_match('/^:[^ ]+? [0-9]{3} .+$/', $line) == 1) {
            $lineex = explode(' ', $line);
            $code = $lineex[1];
                
            switch ($code) {
                case SMARTIRC_RPL_WELCOME:
                case SMARTIRC_RPL_YOURHOST:
                case SMARTIRC_RPL_CREATED:
                case SMARTIRC_RPL_MYINFO:
                case SMARTIRC_RPL_BOUNCE:
                    return SMARTIRC_TYPE_LOGIN;
                case SMARTIRC_RPL_LUSERCLIENT:
                case SMARTIRC_RPL_LUSEROP:
                case SMARTIRC_RPL_LUSERUNKNOWN:
                case SMARTIRC_RPL_LUSERME:
                case SMARTIRC_RPL_LUSERCHANNELS:
                    return SMARTIRC_TYPE_INFO;
                case SMARTIRC_RPL_MOTDSTART:
                case SMARTIRC_RPL_MOTD:
                case SMARTIRC_RPL_ENDOFMOTD:
                    return SMARTIRC_TYPE_MOTD;
                case SMARTIRC_RPL_NAMREPLY:
                case SMARTIRC_RPL_ENDOFNAMES:
                    return SMARTIRC_TYPE_NAME;
                case SMARTIRC_RPL_WHOREPLY:
                case SMARTIRC_RPL_ENDOFWHO:
                    return SMARTIRC_TYPE_WHO;
                case SMARTIRC_RPL_LISTSTART:
                    return SMARTIRC_TYPE_NONRELEVANT;
                case SMARTIRC_RPL_LIST:
                case SMARTIRC_RPL_LISTEND:
                    return SMARTIRC_TYPE_LIST;
                case SMARTIRC_RPL_BANLIST:
                case SMARTIRC_RPL_ENDOFBANLIST:
                    return SMARTIRC_TYPE_BANLIST;
                case SMARTIRC_RPL_TOPIC:
                    return SMARTIRC_TYPE_TOPIC;
                case SMARTIRC_RPL_WHOISUSER:
                case SMARTIRC_RPL_WHOISSERVER:
                case SMARTIRC_RPL_WHOISOPERATOR:
                case SMARTIRC_RPL_WHOISIDLE:
                case SMARTIRC_RPL_ENDOFWHOIS:
                case SMARTIRC_RPL_WHOISCHANNELS:
                    return SMARTIRC_TYPE_WHOIS;
                case SMARTIRC_RPL_WHOWASUSER:
                case SMARTIRC_RPL_ENDOFWHOWAS:
                    return SMARTIRC_TYPE_WHOWAS;
                case SMARTIRC_RPL_UMODEIS:
                    return SMARTIRC_TYPE_USERMODE;
                case SMARTIRC_RPL_CHANNELMODEIS:
                    return SMARTIRC_TYPE_CHANNELMODE;
                case SMARTIRC_ERR_NICKNAMEINUSE:
                case SMARTIRC_ERR_NOTREGISTERED:
                    return SMARTIRC_TYPE_ERROR;
                default:
                    $this->log(SMARTIRC_DEBUG_IRCMESSAGES, 'DEBUG_IRCMESSAGES: replycode UNKNOWN ('.$code.'): "'.$line.'"', __FILE__, __LINE__);
            }
        }
        
        if (preg_match('/^:.*? PRIVMSG .* :'.chr(1).'ACTION .*'.chr(1).'$/', $line) == 1) {
            return SMARTIRC_TYPE_ACTION;
        } else if (preg_match('/^:.*? PRIVMSG .* :'.chr(1).'.*'.chr(1).'$/', $line) == 1) {
            return SMARTIRC_TYPE_CTCP;
        } else if (preg_match('/^:.*? PRIVMSG (\&|\#|\+|\!).* :.*$/', $line) == 1) {
            return SMARTIRC_TYPE_CHANNEL;
        } else if (preg_match('/^:.*? PRIVMSG .*:.*$/', $line) == 1) {
            return SMARTIRC_TYPE_QUERY;
        } else if (preg_match('/^:.*? NOTICE .* :.*$/', $line) == 1) {
            return SMARTIRC_TYPE_NOTICE;
        } else if (preg_match('/^:.*? INVITE .* .*$/', $line) == 1) {
            return SMARTIRC_TYPE_INVITE;
        } else if (preg_match('/^:.*? JOIN .*$/', $line) == 1) {
            return SMARTIRC_TYPE_JOIN;
        } else if (preg_match('/^:.*? TOPIC .* :.*$/', $line) == 1) {
            return SMARTIRC_TYPE_TOPICCHANGE;
        } else if (preg_match('/^:.*? NICK .*$/', $line) == 1) {
            return SMARTIRC_TYPE_NICKCHANGE;
        } else if (preg_match('/^:.*? KICK .* .*$/', $line) == 1) {
            return SMARTIRC_TYPE_KICK;
        } else if (preg_match('/^:.*? PART .*$/', $line) == 1) {
            return SMARTIRC_TYPE_PART;
        } else if (preg_match('/^:.*? MODE .* .*$/', $line) == 1) {
            return SMARTIRC_TYPE_MODECHANGE;
        } else if (preg_match('/^:.*? QUIT :.*$/', $line) == 1) {
            return SMARTIRC_TYPE_QUIT;
        } else {
            $this->log(SMARTIRC_DEBUG_MESSAGETYPES, 'DEBUG_MESSAGETYPES: SMARTIRC_TYPE_UNKNOWN!: "'.$line.'"', __FILE__, __LINE__);
            return SMARTIRC_TYPE_UNKNOWN;
        }
    }
    
       
                                           
      
                      
                      
       
    function _updatestate()
    {
        $rtype = get_resource_type($this->_socket);
        if ((is_resource($this->_socket)) &&
            ($this->_socket !== false) &&
            ($rtype == 'socket' || $rtype == 'Socket' || $rtype == 'stream')) {
            
            $this->_state = true;
            return true;
        } else {
            $this->_state = false;
            $this->_loggedin = false;
            return false;
        }
    }
    
       
                                           
      
                                                                              
                      
       
    function _state()
    {
        $result = $this->_updatestate();
        
        if ($result == true) {
            return SMARTIRC_STATE_CONNECTED;
        } else {
            return SMARTIRC_STATE_DISCONNECTED;
        }
    }
    
       
                                                                                      
      
                                 
                             
                   
                      
       
    function _handlemessage($messagecode, &$ircdata)
    {
        $found = false;
        
        if (is_numeric($messagecode)) {
            if (!array_key_exists($messagecode, $this->nreplycodes)) {
                $this->log(SMARTIRC_DEBUG_MESSAGEHANDLER, 'DEBUG_MESSAGEHANDLER: ignoring unreconzied messagecode! "'.$messagecode.'"', __FILE__, __LINE__);
                $this->log(SMARTIRC_DEBUG_MESSAGEHANDLER, 'DEBUG_MESSAGEHANDLER: this IRC server ('.$this->_address.') doesn\'t conform to the RFC 2812!', __FILE__, __LINE__);
                return;
            }
            
            $methodname = 'event_'.strtolower($this->nreplycodes[$messagecode]);
            $_methodname = '_'.$methodname;
            $_codetype = 'by numeric';
        } else if (is_string($messagecode)) {  
            $methodname = 'event_'.strtolower($messagecode);
            $_methodname = '_'.$methodname;
            $_codetype = 'by string';
        }
        

        if (@method_exists($this, $_methodname)) {
           $this->log(SMARTIRC_DEBUG_MESSAGEHANDLER, 'DEBUG_MESSAGEHANDLER: calling internal method "'.get_class($this).'->'.$_methodname.'" ('.$_codetype.')', __FILE__, __LINE__);
           $this->$_methodname($ircdata);
           $found = true;
        }
        

        if (@method_exists($this, $methodname)) {
           $this->log(SMARTIRC_DEBUG_MESSAGEHANDLER, 'DEBUG_MESSAGEHANDLER: calling user defined method "'.get_class($this).'->'.$methodname.'" ('.$_codetype.')', __FILE__, __LINE__);
           $this->$methodname($ircdata);
           $found = true;
        }
        
        if ($found == false) {
            $this->log(SMARTIRC_DEBUG_MESSAGEHANDLER, 'DEBUG_MESSAGEHANDLER: no method found for "'.$messagecode.'" ('.$methodname.')', __FILE__, __LINE__);
        }
    }
    
       
                                                                                     
      
                             
                   
                      
       
    function _handleactionhandler(&$ircdata)
    {
        $handler = &$this->_actionhandler;
        $handlercount = count($handler);
        for ($i = 0; $i < $handlercount; $i++) {
            $handlerobject = &$handler[$i];
            
            if (substr($handlerobject->message, 0, 1) == '/') {
                $regex = $handlerobject->message;
            } else {
                $regex = '/'.$handlerobject->message.'/';
            }
            
            if (($handlerobject->type & $ircdata->type) &&
                (preg_match($regex, $ircdata->message) == 1)) {
                
                $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: actionhandler match found for id: '.$i.' type: '.$ircdata->type.' message: "'.$ircdata->message.'" regex: "'.$regex.'"', __FILE__, __LINE__);
                
                $methodobject = &$handlerobject->object;
                $method = $handlerobject->method;
                
                if (@method_exists($methodobject, $method)) {
                    $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: calling method "'.get_class($methodobject).'->'.$method.'"', __FILE__, __LINE__);
                    $methodobject->$method($this, $ircdata);
                } else {
                    $this->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: method doesn\'t exist! "'.get_class($methodobject).'->'.$method.'"', __FILE__, __LINE__);
                }
                
                break;
            }
        }
    }
    
       
                                                       
      
                    
                      
       
    function _microint()
    {
        $tmp = microtime();
        $parts = explode(' ', $tmp);
        $floattime = (float)$parts[0] + (float)$parts[1];
        return $floattime;
    }
    
       
                                                            
      
                             
                             
                   
                      
       
    function _adduser(&$channel, &$newuser)
    {
        $lowerednick = strtolower($newuser->nick);
        if (isset($channel->users[$lowerednick])) {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: updating user: '.$newuser->nick.' on channel: '.$channel->name, __FILE__, __LINE__);
            

            $currentuser = &$channel->users[$lowerednick];
            
            if ($newuser->ident !== null) {
                $currentuser->ident = $newuser->ident;
            }
            if ($newuser->host !== null) {
                $currentuser->host = $newuser->host;
            }
            if ($newuser->realname !== null) {
                $currentuser->realname = $newuser->realname;
            }
            if ($newuser->op !== null) {
                $currentuser->op = $newuser->op;
            }
            if ($newuser->voice !== null) {
                $currentuser->voice = $newuser->voice;
            }
            if ($newuser->ircop !== null) {
                $currentuser->ircop = $newuser->ircop;
            }
            if ($newuser->away !== null) {
                $currentuser->away = $newuser->away;
            }
            if ($newuser->server !== null) {
                $currentuser->server = $newuser->server;
            }
            if ($newuser->hopcount !== null) {
                $currentuser->hopcount = $newuser->hopcount;
            }
        } else {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding user: '.$newuser->nick.' to channel: '.$channel->name, __FILE__, __LINE__);
            

            $channel->users[$lowerednick] = &$newuser;
        }
        
        $user = &$channel->users[$lowerednick];
        if ($user->op) {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding op: '.$user->nick.' to channel: '.$channel->name, __FILE__, __LINE__);
            $channel->ops[$user->nick] = true;
        }
        if ($user->voice) {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding voice: '.$user->nick.' to channel: '.$channel->name, __FILE__, __LINE__);
            $channel->voices[$user->nick] = true;
        }
    }
    
       
                                                          
      
                             
                   
                      
       
    function _removeuser(&$ircdata)
    {
        if ($ircdata->type & (SMARTIRC_TYPE_PART|SMARTIRC_TYPE_QUIT)) {
            $nick = $ircdata->nick;
        } else if ($ircdata->type & SMARTIRC_TYPE_KICK) {
            $nick = $ircdata->rawmessageex[3];
        } else {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: unknown TYPE ('.$ircdata->type.') in _removeuser(), trying default', __FILE__, __LINE__);
            $nick = $ircdata->nick;
        }
        
        $lowerednick = strtolower($nick);
        
        if ($this->_nick == $nick) {
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: we left channel: '.$ircdata->channel.' destroying...', __FILE__, __LINE__);
            unset($this->_channels[strtolower($ircdata->channel)]);
        } else {
            if ($ircdata->type & SMARTIRC_TYPE_QUIT) {
                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: user '.$nick.' quit, removing him from all channels', __FILE__, __LINE__);

                foreach ($this->_channels as $channelkey => $channelvalue) {

                    $channel = &$this->_channels[$channelkey];
                    foreach ($channel->users as $userkey => $uservalue) {

                        
                        if ($nick == $uservalue->nick) {


                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: found him on channel: '.$channel->name.' destroying...', __FILE__, __LINE__);
                            unset($channel->users[$lowerednick]);
                            
                            if (isset($channel->ops[$nick])) {

                                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing him from op list', __FILE__, __LINE__);
                                unset($channel->ops[$nick]);
                            }
                            
                            if (isset($channel->voices[$nick])) {

                                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing him from voice list', __FILE__, __LINE__);
                                unset($channel->voices[$nick]);
                            }
                            

                        }
                    }
                }
            } else {
                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing user: '.$nick.' from channel: '.$ircdata->channel, __FILE__, __LINE__);
                $channel = &$this->_channels[strtolower($ircdata->channel)];
                unset($channel->users[$lowerednick]);
                
                if (isset($channel->ops[$nick])) {
                    $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing him from op list', __FILE__, __LINE__);
                    unset($channel->ops[$nick]);
                }
                
                if (isset($channel->voices[$nick])) {
                    $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing him from voice list', __FILE__, __LINE__);
                    unset($channel->voices[$nick]);
                }
            }
        }
    }
    
       
                   
                      
       
    function _checkPHPVersion()
    {

    }
    
       
                                                
      
                                  
                      
                      
       
    function _isValidType($handlertype) {
        if ($handlertype & SMARTIRC_TYPE_ALL ) {
            return true;
        } else {
            return false;
        }
    }
    

    
    function isError($object) {
        return (bool)(is_object($object) && (strtolower(get_class($object)) == 'net_smartirc_error'));
    }
    
    function &throwError($message) {
        return new Net_SmartIRC_Error($message);
    }
}

class Net_SmartIRC extends Net_SmartIRC_messagehandler
{

}

   
                 
   
class Net_SmartIRC_data
{
       
                  
                     
       
    var $from;
    
       
                  
                     
       
    var $nick;
    
       
                  
                     
       
    var $ident;
    
       
                  
                     
       
    var $host;
    
       
                  
                     
       
    var $channel;
    
       
                  
                     
       
    var $message;
    
       
                 
                     
       
    var $messageex = array();
    
       
                   
                     
       
    var $type;
    
       
                  
                     
       
    var $rawmessage;
    
       
                 
                     
       
    var $rawmessageex = array();
}

   
                 
   
class Net_SmartIRC_actionhandler
{
       
                   
                     
       
    var $id;
    
       
                   
                     
       
    var $type;
    
       
                  
                     
       
    var $message;
    
       
                  
                     
       
    var $object;
    
       
                  
                     
       
    var $method;
}

   
                 
   
class Net_SmartIRC_timehandler
{
       
                   
                     
       
    var $id;
    
       
                   
                     
       
    var $interval;
    
       
                   
                     
       
    var $lastmicrotimestamp;
    
       
                  
                     
       
    var $object;
    
       
                  
                     
       
    var $method;
}

   
                 
   
class Net_SmartIRC_channel
{
       
                  
                     
       
    var $name;
    
       
                  
                     
       
    var $key;
    
       
                 
                     
       
    var $users = array();
    
       
                 
                     
       
    var $ops = array();
    
       
                 
                     
       
    var $voices = array();
    
       
                 
                     
       
    var $bans = array();
    
       
                  
                     
       
    var $topic;
    
       
                  
                     
       
    var $mode;
}

   
                 
   
class Net_SmartIRC_user
{
       
                  
                     
       
    var $nick;
    
       
                  
                     
       
    var $ident;
    
       
                  
                     
       
    var $host;
    
       
                  
                     
       
    var $realname;
    
       
                   
                     
       
    var $ircop;
    
       
                   
                     
       
    var $away;
    
       
                  
                     
       
    var $server;
    
       
                   
                     
       
    var $hopcount;
}

   
                 
   
class Net_SmartIRC_channeluser extends Net_SmartIRC_user
{
       
                   
                     
       
    var $op;
    
       
                   
                     
       
    var $voice;
}

   
                 
   
class Net_SmartIRC_ircuser extends Net_SmartIRC_user
{
       
                 
                     
       
    var $joinedchannels = array();
}

   
                 
   
class Net_SmartIRC_listenfor
{
       
                 
                     
       
    var $result = array();
    
       
                                                       
      
                         
                             
                   
       
    function handler(&$irc, &$ircdata)
    {
        $irc->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: listenfor handler called', __FILE__, __LINE__);
        $this->result[] = $ircdata->message;
        $irc->disconnect(true);
    }
}


   
                 
   
class Net_SmartIRC_objListenFor
{
       
                 
                     
       
    var $result = array();
    
       
                                                       
      
                         
                             
                   
       
    function handler(&$irc, &$ircdata)
    {
        $irc->log(SMARTIRC_DEBUG_ACTIONHANDLER, 'DEBUG_ACTIONHANDLER: objListenFor handler called', __FILE__, __LINE__);
        $this->result[] = $ircdata;
        $irc->disconnect(true);
    }
}

class Net_SmartIRC_Error
{
    var $error_msg;
    
    function Net_SmartIRC_Error($message)
    {
        $this->error_msg = $message;
    }
    
    function getMessage()
    {
        return $this->error_msg;
    }
}
?>