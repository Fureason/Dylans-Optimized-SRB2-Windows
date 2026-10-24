<?php

/*************************************************

Snoopy - the PHP net client
Author: Monte Ohrt <monte@ispi.net>
Copyright (c): 1999-2000 ispi, all rights reserved
Version: 1.0

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

You may contact the author of Snoopy by e-mail at:
monte@ispi.net

Or, write to:
Monte Ohrt
CTO, ispi
237 S. 70th suite 220
Lincoln, NE 68510

The latest version of Snoopy can be obtained from:
http://snoopy.sourceforge.com

*************************************************/

class Snoopy
{
	 
	
	 

	var $host			=	"www.php.net";		 
	var $port			=	80;					 
	var $proxy_host		=	"";					 
	var $proxy_port		=	"";					 
	var $agent			=	"Snoopy v1.0";		 
	var	$referer		=	"";					 
	var $cookies		=	array();			 

	var	$rawheaders		=	array();			 


	var $maxredirs		=	5;					 
	var $lastredirectaddr	=	"";				 
	var	$offsiteok		=	true;				 
	var $maxframes		=	0;					 
	var $expandlinks	=	true;				 


	var $passcookies	=	true;				 


	
	var	$user			=	"";					 
	var	$pass			=	"";					 
	

	var $accept			=	"image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*";
	
	var $results		=	"";					 
		
	var $error			=	"";					 
	var	$response_code	=	"";					 
	var	$headers		=	array();			 
	var	$maxlength		=	500000;				 
	var $read_timeout	=	0;					 


	var $timed_out		=	false;				 
	var	$status			=	0;					 
	
	var	$curl_path		=	"/usr/bin/curl";










	

	var $use_gzip		= true;	
	
	 	
	
	var	$_maxlinelen	=	4096;				 
	
	var $_httpmethod	=	"GET";				 
	var $_httpversion	=	"HTTP/1.0";			 
	var $_submit_method	=	"POST";				 
	var $_submit_type	=	"application/x-www-form-urlencoded";	 
	var $_mime_boundary	=   "";					 
	var $_redirectaddr	=	false;				 
	var $_redirectdepth	=	0;					 
	var $_frameurls		= 	array();			 
	var $_framedepth	=	0;					 
	
	var $_isproxy		=	false;				 
	var $_fp_timeout	=	30;					 

                                                                         \
                
                                          
                                        
                                        
                                               
                                                       
                                                                          

	function fetch($URI)
	{
	

		$URI_PARTS = parse_url($URI);
		if (!empty($URI_PARTS["user"]))
			$this->user = $URI_PARTS["user"];
		if (!empty($URI_PARTS["pass"]))
			$this->pass = $URI_PARTS["pass"];
				
		switch($URI_PARTS["scheme"])
		{
			case "http":
				$this->host = $URI_PARTS["host"];
				if(!empty($URI_PARTS["port"]))
					$this->port = $URI_PARTS["port"];
				if($this->_connect($fp))
				{
					if($this->_isproxy)
					{

						$this->_httprequest($URI,$fp,$URI,$this->_httpmethod);
					}
					else
					{
						$path = $URI_PARTS["path"].(isset($URI_PARTS["query"]) ? "?".$URI_PARTS["query"] : "");

						$this->_httprequest($path, $fp, $URI, $this->_httpmethod);
					}
					
					$this->_disconnect($fp);

					if($this->_redirectaddr)
					{
						 
						if($this->maxredirs > $this->_redirectdepth)
						{

							if(preg_match("|^http://".preg_quote($this->host)."|i",$this->_redirectaddr) || $this->offsiteok)
							{
								 
								$this->_redirectdepth++;
								$this->lastredirectaddr=$this->_redirectaddr;
								$this->fetch($this->_redirectaddr);
							}
						}
					}

					if($this->_framedepth < $this->maxframes && count($this->_frameurls) > 0)
					{
						$frameurls = $this->_frameurls;
						$this->_frameurls = array();
						
						while(list(,$frameurl) = each($frameurls))
						{
							if($this->_framedepth < $this->maxframes)
							{
								$this->fetch($frameurl);
								$this->_framedepth++;
							}
							else
								break;
						}
					}					
				}
				else
				{
					return false;
				}
				return true;					
				break;
			case "https":
				if(!$this->curl_path || (!is_executable($this->curl_path))) {
					$this->error = "Bad curl ($this->curl_path), can't fetch HTTPS \n";
					return false;
				}
				$this->host = $URI_PARTS["host"];
				if(!empty($URI_PARTS["port"]))
					$this->port = $URI_PARTS["port"];
				if($this->_isproxy)
				{

					$this->_httpsrequest($URI,$URI,$this->_httpmethod);
				}
				else
				{
					$path = $URI_PARTS["path"].($URI_PARTS["query"] ? "?".$URI_PARTS["query"] : "");

					$this->_httpsrequest($path, $URI, $this->_httpmethod);
				}

				if($this->_redirectaddr)
				{
					 
					if($this->maxredirs > $this->_redirectdepth)
					{

						if(preg_match("|^http://".preg_quote($this->host)."|i",$this->_redirectaddr) || $this->offsiteok)
						{
							 
							$this->_redirectdepth++;
							$this->lastredirectaddr=$this->_redirectaddr;
							$this->fetch($this->_redirectaddr);
						}
					}
				}

				if($this->_framedepth < $this->maxframes && count($this->_frameurls) > 0)
				{
					$frameurls = $this->_frameurls;
					$this->_frameurls = array();

					while(list(,$frameurl) = each($frameurls))
					{
						if($this->_framedepth < $this->maxframes)
						{
							$this->fetch($frameurl);
							$this->_framedepth++;
						}
						else
							break;
					}
				}					
				return true;					
				break;
			default:

				$this->error	=	'Invalid protocol "'.$URI_PARTS["scheme"].'"\n';
				return false;
				break;
		}		
		return true;
	}



                                                                         \
                  
                                                                          
	
	
                                                                         \
                      
                                                    
                                     
                                       
                                                                          

	function _striplinks($document)
	{	
		preg_match_all("'<\s*a\s+.*href\s*=\s*			# find <a href=
						([\"\'])?					# find single or double quote
						(?(1) (.*?)\\1 | ([^\s\>]+))		# if quote found, match up to next matching
						'isx",$document,$links);
						



		while(list($key,$val) = each($links[2]))
		{
			if(!empty($val))
				$match[] = $val;
		}				
		
		while(list($key,$val) = each($links[3]))
		{
			if(!empty($val))
				$match[] = $val;
		}		
		

		return $match;
	}

                                                                         \
                     
                                                       
                                     
                                       
                                                                          

	function _stripform($document)
	{	
		preg_match_all("'<\/?(FORM|INPUT|SELECT|TEXTAREA|(OPTION))[^<>]*>(?(2)(.*(?=<\/?(option|select)[^<>]*>[\r\n]*)|(?=[\r\n]*))|(?=[\r\n]*))'Usi",$document,$elements);
		

		$match = implode("\r\n",$elements[0]);
				

		return $match;
	}

	
	
                                                                         \
                     
                                              
                                     
                                   
                                                                          

	function _striptext($document)
	{
		



								
		$search = array("'<script[^>]*?>.*?</script>'si",	 
						"'<[\/\!]*?[^<>]*?>'si",			 
						"'([\r\n])[\s]+'",					 
						"'&(quote|#34);'i",					 
						"'&(amp|#38);'i",
						"'&(lt|#60);'i",
						"'&(gt|#62);'i",
						"'&(nbsp|#160);'i",
						"'&(iexcl|#161);'i",
						"'&(cent|#162);'i",
						"'&(pound|#163);'i",
						"'&(copy|#169);'i"
						);				
		$replace = array(	"",
							"",
							"\\1",
							"\"",
							"&",
							"<",
							">",
							" ",
							chr(161),
							chr(162),
							chr(163),
							chr(169));
					
		$text = preg_replace($search,$replace,$document);
								
		return $text;
	}

                                                                         \
                       
                                                     
                                      
                                            
                                           
                                                                          

	function _expandlinks($links,$URI)
	{
		
		preg_match("/^[^\?]+/",$URI,$match);

		$match = preg_replace("|/[^\/\.]+\.[^\/\.]+$|","",$match[0]);
				
		$search = array( 	"|^http://".preg_quote($this->host)."|i",
							"|^(?!http://)(\/)?(?!mailto:)|i",
							"|/\./|",
							"|/[^\/]+/\.\./|"
						);
						
		$replace = array(	"",
							$match."/",
							"/",
							"/"
						);			
				
		$expandedLinks = preg_replace($search,$replace,$links);

		return $expandedLinks;
	}

                                                                         \
                       
                                              
                               
                                       
                      
                                              
          
                                                                          
	
	function _httprequest($url,$fp,$URI,$http_method,$content_type="",$body="")
	{
		if($this->passcookies && $this->_redirectaddr)
			$this->setcookies();
			
		$URI_PARTS = parse_url($URI);
		if(empty($url))
			$url = "/";
		$headers = $http_method." ".$url." ".$this->_httpversion."\r\n";		
		if(!empty($this->agent))
			$headers .= "User-Agent: ".$this->agent."\r\n";
		if(!empty($this->host) && !isset($this->rawheaders['Host']))
			$headers .= "Host: ".$this->host."\r\n";
		if(!empty($this->accept))
			$headers .= "Accept: ".$this->accept."\r\n";
		
		if($this->use_gzip) {


			if ( function_exists(gzinflate) ) {
			   $headers .= "Accept-encoding: gzip\r\n";
			}
			else {
			   trigger_error(
			   	"use_gzip is on, but PHP was built without zlib support.".
				"  Requesting file(s) without gzip encoding.", 
				E_USER_NOTICE);
			}
		}
		
		if(!empty($this->referer))
			$headers .= "Referer: ".$this->referer."\r\n";
		if(!empty($this->cookies))
		{			
			if(!is_array($this->cookies))
				$this->cookies = (array)$this->cookies;
	
			reset($this->cookies);
			if ( count($this->cookies) > 0 ) {
				$cookie_headers .= 'Cookie: ';
				foreach ( $this->cookies as $cookieKey => $cookieVal ) {
				$cookie_headers .= $cookieKey."=".urlencode($cookieVal)."; ";
				}
				$headers .= substr($cookie_headers,0,-2) . "\r\n";
			} 
		}
		if(!empty($this->rawheaders))
		{
			if(!is_array($this->rawheaders))
				$this->rawheaders = (array)$this->rawheaders;
			while(list($headerKey,$headerVal) = each($this->rawheaders))
				$headers .= $headerKey.": ".$headerVal."\r\n";
		}
		if(!empty($content_type)) {
			$headers .= "Content-type: $content_type";
			if ($content_type == "multipart/form-data")
				$headers .= "; boundary=".$this->_mime_boundary;
			$headers .= "\r\n";
		}
		if(!empty($body))	
			$headers .= "Content-length: ".strlen($body)."\r\n";
		if(!empty($this->user) || !empty($this->pass))	
			$headers .= "Authorization: BASIC ".base64_encode($this->user.":".$this->pass)."\r\n";

		$headers .= "\r\n";
		

		if ($this->read_timeout > 0)
			socket_set_timeout($fp, $this->read_timeout);
		$this->timed_out = false;
		
		fwrite($fp,$headers.$body,strlen($headers.$body));
		
		$this->_redirectaddr = false;
		unset($this->headers);
		

		$is_gzipped = false;
						
		while($currentHeader = fgets($fp,$this->_maxlinelen))
		{
			if ($this->read_timeout > 0 && $this->_check_timeout($fp))
			{
				$this->status=-100;
				return false;
			}
				

			if(preg_match("/^\r?\n$/", $currentHeader) )
			      break;
						

			if(preg_match("/^(Location:|URI:)/i",$currentHeader))
			{

				preg_match("/^(Location:|URI:)\s+(.*)/",chop($currentHeader),$matches);

				if(!preg_match("|\:\/\/|",$matches[2]))
				{

					$this->_redirectaddr = $URI_PARTS["scheme"]."://".$this->host.":".$this->port;

					if(!preg_match("|^/|",$matches[2]))
							$this->_redirectaddr .= "/".$matches[2];
					else
							$this->_redirectaddr .= $matches[2];
				}
				else
					$this->_redirectaddr = $matches[2];
			}
		
			if(preg_match("|^HTTP/|",$currentHeader))
			{
                if(preg_match("|^HTTP/[^\s]*\s(.*?)\s|",$currentHeader, $status))
				{
					$this->status= $status[1];
                }				
				$this->response_code = $currentHeader;
			}
			
			if (preg_match("/Content-Encoding: gzip/", $currentHeader) ) {
				$is_gzipped = true;
			}
			
			$this->headers[] = $currentHeader;
		}


		$results = "";
		while ( $data = fread($fp, $this->maxlength) ) {
		    $results .= $data;
		    if (
		        strlen($results) > $this->maxlength ) {
		        break;
		    }
		}
		

		if ( $is_gzipped ) {

			$results = substr($results, 10);
			$results = gzinflate($results);
		}
		
		if ($this->read_timeout > 0 && $this->_check_timeout($fp))
		{
			$this->status=-100;
			return false;
		}
		

		
		if(preg_match("'<meta[\s]*http-equiv[^>]*?content[\s]*=[\s]*[\"\']?\d+;[\s]+URL[\s]*=[\s]*([^\"\']*?)[\"\']?>'i",$results,$match))
		{
			$this->_redirectaddr = $this->_expandlinks($match[1],$URI);	
		}


		if(($this->_framedepth < $this->maxframes) && preg_match_all("'<frame\s+.*src[\s]*=[\'\"]?([^\'\"\>]+)'i",$results,$match))
		{
			$this->results[] = $results;
			for($x=0; $x<count($match[1]); $x++)
				$this->_frameurls[] = $this->_expandlinks($match[1][$x],$URI_PARTS["scheme"]."://".$this->host);
		}

		elseif(is_array($this->results))
			$this->results[] = $results;

		else
			$this->results = $results;
		
		return true;
	}

                                                                         \
                        
                                                          
                               
                      
                                              
          
                                                                          
	
	function _httpsrequest($url,$URI,$http_method,$content_type="",$body="")
	{
		if($this->passcookies && $this->_redirectaddr)
			$this->setcookies();

		$headers = array();		
					
		$URI_PARTS = parse_url($URI);
		if(empty($url))
			$url = "/";


		if(!empty($this->agent))
			$headers[] = "User-Agent: ".$this->agent;
		if(!empty($this->host))
			$headers[] = "Host: ".$this->host;
		if(!empty($this->accept))
			$headers[] = "Accept: ".$this->accept;
		if(!empty($this->referer))
			$headers[] = "Referer: ".$this->referer;
		if(!empty($this->cookies))
		{			
			if(!is_array($this->cookies))
				$this->cookies = (array)$this->cookies;
	
			reset($this->cookies);
			if ( count($this->cookies) > 0 ) {
				$cookie_str = 'Cookie: ';
				foreach ( $this->cookies as $cookieKey => $cookieVal ) {
				$cookie_str .= $cookieKey."=".urlencode($cookieVal)."; ";
				}
				$headers[] = substr($cookie_str,0,-2);
			}
		}
		if(!empty($this->rawheaders))
		{
			if(!is_array($this->rawheaders))
				$this->rawheaders = (array)$this->rawheaders;
			while(list($headerKey,$headerVal) = each($this->rawheaders))
				$headers[] = $headerKey.": ".$headerVal;
		}
		if(!empty($content_type)) {
			if ($content_type == "multipart/form-data")
				$headers[] = "Content-type: $content_type; boundary=".$this->_mime_boundary;
			else
				$headers[] = "Content-type: $content_type";
		}
		if(!empty($body))	
			$headers[] = "Content-length: ".strlen($body);
		if(!empty($this->user) || !empty($this->pass))	
			$headers[] = "Authorization: BASIC ".base64_encode($this->user.":".$this->pass);
			
		for($curr_header = 0; $curr_header < count($headers); $curr_header++)
			$cmdline_params .= " -H \"".$headers[$curr_header]."\"";
		
		if(!empty($body))
			$cmdline_params .= " -d \"$body\"";
		
		if($this->read_timeout > 0)
			$cmdline_params .= " -m ".$this->read_timeout;
		
		$headerfile = uniqid(time());
		

		$cmdline_params .= " -k";
		exec($this->curl_path." -D \"/tmp/$headerfile\"".$cmdline_params." ".$URI,$results,$return);
		
		if($return)
		{
			$this->error = "Error: cURL could not retrieve the document, error $return.";
			return false;
		}
			
			
		$results = implode("\r\n",$results);
		
		$result_headers = file("/tmp/$headerfile");
						
		$this->_redirectaddr = false;
		unset($this->headers);
						
		for($currentHeader = 0; $currentHeader < count($result_headers); $currentHeader++)
		{
			

			if(preg_match("/^(Location: |URI: )/i",$result_headers[$currentHeader]))
			{

				preg_match("/^(Location: |URI:)(.*)/",chop($result_headers[$currentHeader]),$matches);

				if(!preg_match("|\:\/\/|",$matches[2]))
				{

					$this->_redirectaddr = $URI_PARTS["scheme"]."://".$this->host.":".$this->port;

					if(!preg_match("|^/|",$matches[2]))
							$this->_redirectaddr .= "/".$matches[2];
					else
							$this->_redirectaddr .= $matches[2];
				}
				else
					$this->_redirectaddr = $matches[2];
			}
		
			if(preg_match("|^HTTP/|",$result_headers[$currentHeader]))
			{
			    $this->response_code = $result_headers[$currentHeader];
			    if(preg_match("|^HTTP/[^\s]*\s(.*?)\s|",$this->response_code, $match))
			    {
				$this->status= $match[1];
                	    }
			}
			$this->headers[] = $result_headers[$currentHeader];
		}


		
		if(preg_match("'<meta[\s]*http-equiv[^>]*?content[\s]*=[\s]*[\"\']?\d+;[\s]+URL[\s]*=[\s]*([^\"\']*?)[\"\']?>'i",$results,$match))
		{
			$this->_redirectaddr = $this->_expandlinks($match[1],$URI);	
		}


		if(($this->_framedepth < $this->maxframes) && preg_match_all("'<frame\s+.*src[\s]*=[\'\"]?([^\'\"\>]+)'i",$results,$match))
		{
			$this->results[] = $results;
			for($x=0; $x<count($match[1]); $x++)
				$this->_frameurls[] = $this->_expandlinks($match[1][$x],$URI_PARTS["scheme"]."://".$this->host);
		}

		elseif(is_array($this->results))
			$this->results[] = $results;

		else
			$this->results = $results;

		unlink("/tmp/$headerfile");
		
		return true;
	}

                                                                         \
                       
                                       
                                                                          
	
	function setcookies()
	{
		for($x=0; $x<count($this->headers); $x++)
		{
		if(preg_match("/^set-cookie:[\s]+([^=]+)=([^;]+)/i", $this->headers[$x],$match))
			$this->cookies[$match[1]] = $match[2];
		}
	}

	
                                                                         \
                         
                                             
                         
                                                                          

	function _check_timeout($fp)
	{
		if ($this->read_timeout > 0) {
			$fp_status = socket_get_status($fp);
			if ($fp_status["timed_out"]) {
				$this->timed_out = true;
				return true;
			}
		}
		return false;
	}

                                                                         \
                   
                                  
                         
                                                                          
	
	function _connect(&$fp)
	{
		if(!empty($this->proxy_host) && !empty($this->proxy_port))
			{
				$this->_isproxy = true;
				$host = $this->proxy_host;
				$port = $this->proxy_port;
			}
		else
		{
			$host = $this->host;
			$port = $this->port;
		}
	
		$this->status = 0;
		
		if($fp = fsockopen(
					$host,
					$port,
					$errno,
					$errstr,
					$this->_fp_timeout
					))
		{


			return true;
		}
		else
		{

			$this->status = $errno;
			switch($errno)
			{
				case -3:
					$this->error="socket creation failed (-3)";
				case -4:
					$this->error="dns lookup failure (-4)";
				case -5:
					$this->error="connection refused or timed out (-5)";
				default:
					$this->error="connection failed (".$errno.")";
			}
			return false;
		}
	}
                                                                         \
                      
                                        
                         
                                                                          
	
	function _disconnect($fp)
	{
		return(fclose($fp));
	}

	
                                                                         \
                             
                                                      
                                    
                                  
                   
                                                                          
	
	function _prepare_post_body($formvars, $formfiles)
	{
		settype($formvars, "array");
		settype($formfiles, "array");

		if (count($formvars) == 0 && count($formfiles) == 0)
			return;
		
		switch ($this->_submit_type) {
			case "application/x-www-form-urlencoded":
				reset($formvars);
				while(list($key,$val) = each($formvars)) {
					if (is_array($val) || is_object($val)) {
						while (list($cur_key, $cur_val) = each($val)) {
							$postdata .= urlencode($key)."[]=".urlencode($cur_val)."&";
						}
					} else
						$postdata .= urlencode($key)."=".urlencode($val)."&";
				}
				break;

			case "multipart/form-data":
				$this->_mime_boundary = "Snoopy".md5(uniqid(microtime()));
				
				reset($formvars);
				while(list($key,$val) = each($formvars)) {
					if (is_array($val) || is_object($val)) {
						while (list($cur_key, $cur_val) = each($val)) {
							$postdata .= "--".$this->_mime_boundary."\r\n";
							$postdata .= "Content-Disposition: form-data; name=\"$key\[\]\"\r\n\r\n";
							$postdata .= "$cur_val\r\n";
						}
					} else {
						$postdata .= "--".$this->_mime_boundary."\r\n";
						$postdata .= "Content-Disposition: form-data; name=\"$key\"\r\n\r\n";
						$postdata .= "$val\r\n";
					}
				}
				
				reset($formfiles);
				while (list($field_name, $file_names) = each($formfiles)) {
					settype($file_names, "array");
					while (list(, $file_name) = each($file_names)) {
						if (!is_readable($file_name)) continue;

						$fp = fopen($file_name, "r");
						$file_content = fread($fp, filesize($file_name));
						fclose($fp);
						$base_name = basename($file_name);

						$postdata .= "--".$this->_mime_boundary."\r\n";
						$postdata .= "Content-Disposition: form-data; name=\"$field_name\"; filename=\"$base_name\"\r\n\r\n";
						$postdata .= "$file_content\r\n";
					}
				}
				$postdata .= "--".$this->_mime_boundary."--\r\n";
				break;
		}

		return $postdata;
	}
}

?>
