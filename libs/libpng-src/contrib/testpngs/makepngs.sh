#!/bin/sh




# Copyright (c) 2015 John Cunningham Bowler



# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h




test -n "$MAKEPNG" || MAKEPNG=./makepng
opts=

mp(){
   ${MAKEPNG} $opts $1 "$3" "$4" "$3-$4$2.png"
}

mpg(){
   if test "$1" = "none"
   then
      mp "" "" "$2" "$3"
   else
      mp "--$1" "-$1" "$2" "$3"
   fi
}

mptrans(){
   if test "$1" = "none"
   then
      mp "--tRNS" "-tRNS" "$2" "$3"
   else
      mp "--tRNS --$1" "-$1-tRNS" "$2" "$3"
   fi
}

case "$1" in
   --small)
      opts="--small";;&

   --all|--small)
      for g in none sRGB linear 1.8
      do
         for c in gray palette
         do
            for b in 1 2 4
            do
               mpg "$g" "$c" "$b"
               mptrans "$g" "$c" "$b"
            done
         done

         mpg "$g" palette 8
         mptrans "$g" palette 8

         for b in 8 16
         do
            for c in gray gray-alpha rgb rgb-alpha
            do
               mpg "$g" "$c" "$b"
            done
            for c in gray rgb
            do
               mptrans "$g" "$c" "$b"
            done
         done
      done;;

   --coverage)



      mpg none gray-alpha 8  
      mpg none palette 8  
      mpg 1.8 gray 2  
      mpg 1.8 palette 2  
      mpg 1.8 palette 4  
      mpg 1.8 palette 8  
      mpg linear palette 8  
      mpg linear rgb-alpha 16  
      mpg sRGB palette 1  
      mpg sRGB rgb-alpha 16  
      :;;

   *)
      echo "$0 $1: unknown argument, usage:" >&2
      echo "  $0 [--all|--coverage|--small]" >&2
      exit 1
esac
