<?php
/*
 * Project:     MagpieRSS: a simple RSS integration tool
 * File:        rss_utils.inc, utility methods for working with RSS
 * Author:      Kellan Elliott-McCrea <kellan@protest.net>
 * Version:     0.51
 * License:     GPL
 *
 * The lastest version of MagpieRSS can be obtained from:
 * http://magpierss.sourceforge.net
 *
 * For questions, help, comments, discussion, etc., please join the
 * Magpie mailing list:
 * magpierss-general@lists.sourceforge.net
 */


                                                                         \
                          
                                                 

                                            
                                                                          

function parse_w3cdtf ( $date_str ) {
    

    $pat = "/(\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2})(:(\d{2}))?(?:([-+])(\d{2}):?(\d{2})|(Z))?/";
    
    if ( preg_match( $pat, $date_str, $match ) ) {
        list( $year, $month, $day, $hours, $minutes, $seconds) = 
            array( $match[1], $match[2], $match[3], $match[4], $match[5], $match[6]);
        

        $epoch = gmmktime( $hours, $minutes, $seconds, $month, $day, $year);
        
        $offset = 0;
        if ( $match[10] == 'Z' ) {

        }
        else {
            list( $tz_mod, $tz_hour, $tz_min ) =
                array( $match[8], $match[9], $match[10]);
            

            if ( ! $tz_hour ) { $tz_hour = 0; }
            if ( ! $tz_min ) { $tz_min = 0; }
        
            $offset_secs = (($tz_hour*60)+$tz_min)*60;
            


            if ( $tz_mod == '+' ) {
                $offset_secs = $offset_secs * -1;
            }
            
            $offset = $offset_secs; 
        }
        $epoch = $epoch + $offset;
        return $epoch;
    }
    else {
        return -1;
    }
}

?>
