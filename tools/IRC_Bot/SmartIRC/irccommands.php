<?php
/**
 * $Id: irccommands.php,v 1.1.2.1 2003/10/10 20:20:58 meebey Exp $
 * $Revision: 1.1.2.1 $
 * $Author: meebey $
 * $Date: 2003/10/10 20:20:58 $
 *
 * Copyright (c) 2002-2003 Mirco "MEEBEY" Bauer <mail@meebey.net> <http://www.meebey.net>
 * 
 * Full LGPL License: <http://www.meebey.net/lgpl.txt>
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
 */

class Net_SmartIRC_irccommands extends Net_SmartIRC_base
{
       
                          
      
                                            
      
                         
                                                                                             
                                                          
                                         
                      
                     
       
    function message($type, $destination, $message, $priority = SMARTIRC_MEDIUM)
    {
        switch ($type) {
            case SMARTIRC_TYPE_CHANNEL:
            case SMARTIRC_TYPE_QUERY:
                $this->_send('PRIVMSG '.$destination.' :'.$message, $priority);
            break;
            case SMARTIRC_TYPE_ACTION:
                $this->_send('PRIVMSG '.$destination.' :'.chr(1).'ACTION '.$message.chr(1), $priority);
            break;
            case SMARTIRC_TYPE_NOTICE:
                $this->_send('NOTICE '.$destination.' :'.$message, $priority);
            break;
            case SMARTIRC_TYPE_CTCP:  
            case SMARTIRC_TYPE_CTCP_REPLY:
                $this->_send('NOTICE '.$destination.' :'.chr(1).$message.chr(1), $priority);
            break;
            case SMARTIRC_TYPE_CTCP_REQUEST:
                $this->_send('PRIVMSG '.$destination.' :'.chr(1).$message.chr(1), $priority);
            break;
            default:
                return false;
        }
            
        return true;
    }
    
       
                                                           
      
                                                                                
      
                                 
                                                     
                     
       
    function &channel($channelname)
    {
        if (isset($this->_channels[strtolower($channelname)])) {
            return $this->_channels[strtolower($channelname)];
        } else {
            return false;
        }
    }
    

       
                                                           
      
                                 
                          
                                                                            
                   
                     
       
    function join($channelarray, $key = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($channelarray)) {
            $channelarray = array($channelarray);
        }
        
        $channellist = implode(',', $channelarray);
        
        if ($key !== null) {
            $this->_send('JOIN '.$channellist.' '.$key, $priority);
        } else {
            $this->_send('JOIN '.$channellist, $priority);
        }
    }
    
       
                                                                  
      
                                  
                            
                                                                            
                   
                     
       
    function part($channelarray, $reason = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($channelarray)) {
            $channelarray = array($channelarray);
        }
        
        $channellist = implode(',', $channelarray);
        
        if ($reason !== null) {
            $this->_send('PART '.$channellist.' :'.$reason, $priority);
        } else {
            $this->_send('PART '.$channellist, $priority);
        }
    }
    
       
                                                                          
      
                             
                                  
                            
                                                                            
                   
                     
       
    function kick($channel, $nicknamearray, $reason = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($nicknamearray)) {
            $nicknamearray = array($nicknamearray);
        }
        
        $nicknamelist = implode(',', $nicknamearray);
        
        if ($reason !== null) {
            $this->_send('KICK '.$channel.' '.$nicknamelist.' :'.$reason, $priority);
        } else {
            $this->_send('KICK '.$channel.' '.$nicknamelist, $priority);
        }
    }
    
       
                                           
      
                                                                 
                                                     
      
                                 
                                                                            
                   
                     
       
    function getList($channelarray = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($channelarray !== null) {
            if (!is_array($channelarray)) {
                $channelarray = array($channelarray);
            }
            
            $channellist = implode(',', $channelarray);
            $this->_send('LIST '.$channellist, $priority);
        } else {
            $this->_send('LIST', $priority);
        }
    }

       
                                                     
      
                                                                   
      
                                 
                                                                            
                   
                     
       
    function names($channelarray = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($channelarray !== null) {
            if (!is_array($channelarray)) {
                $channelarray = array($channelarray);
            }
            
            $channellist = implode(',', $channelarray);
            $this->_send('NAMES '.$channellist, $priority);
        } else {
            $this->_send('NAMES', $priority);
        }
    }
    
       
                                    
      
                             
                              
                                                                            
                   
                     
       
    function setTopic($channel, $newtopic, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('TOPIC '.$channel.' :'.$newtopic, $priority);
    }
    
       
                                  
      
                             
                                                                            
                   
                     
       
    function getTopic($channel, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('TOPIC '.$channel, $priority);
    }
    
       
                                                  
      
                                                     
      
                                                                                    
                                                   
                                                                            
                   
                     
       
    function mode($target, $newmode = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($newmode !== null) {
            $this->_send('MODE '.$target.' '.$newmode, $priority);
        } else {
            $this->_send('MODE '.$target, $priority);
        }
    }
    
       
                                       
      
                             
                              
                                                                            
                   
                     
       
    function op($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '+o '.$nickname, $priority);
    }
    
       
                                         
      
                             
                              
                                                                            
                   
                     
       
    function deop($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-o '.$nickname, $priority);
    }
    
       
                                        
      
                             
                              
                                                                            
                   
                     
       
    function voice($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '+v '.$nickname, $priority);
    }
    
       
                                          
      
                             
                              
                                                                            
                   
                     
       
    function devoice($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-v '.$nickname, $priority);
    }
    
       
                                                                            
      
                                                                
      
                             
                              
                                                                            
                   
                     
       
    function ban($channel, $hostmask = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($hostmask !== null) {
            $this->mode($channel, '+b '.$hostmask, $priority);
        } else {
            $this->mode($channel, 'b', $priority);
        }
    }
    
       
                                             
      
                             
                              
                                                                            
                   
                     
       
    function unban($channel, $hostmask, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-b '.$hostmask, $priority);
    }
    
       
                                              
      
                              
                             
                                                                            
                   
                     
       
    function invite($nickname, $channel, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('INVITE '.$nickname.' '.$channel, $priority);
    }
    
       
                               
      
                                                              
      
                             
                                                                            
                   
                     
       
    function changeNick($newnick, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('NICK '.$newnick, $priority);
        $this->_nick = $newnick;
    }
    
       
                                                 
      
                            
                                                                            
                   
                     
       
    function who($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHO '.$target, $priority);
    }
    
       
                                                   
      
                            
                                                                            
                   
                     
       
    function whois($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHOIS '.$target, $priority);
    }
    
       
                                                    
                                   
      
                            
                                                                            
                   
                     
       
    function whowas($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHOWAS '.$target, $priority);
    }
    
       
                                               
      
                                                      
                                                                            
                   
                     
       
    function quit($quitmessage = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($quitmessage !== null) {
            $this->_send('QUIT :'.$quitmessage, $priority);
        } else {
            $this->_send('QUIT', $priority);
        }
    }
}
?>