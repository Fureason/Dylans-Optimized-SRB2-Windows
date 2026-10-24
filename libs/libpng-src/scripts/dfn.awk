#!/bin/awk -f
# scripts/dfn.awk - process a .dfn file
#
# last changed in libpng version 1.5.19 - August 21, 2014
#
# Copyright (c) 2013-2014 Glenn Randers-Pehrson
#
# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h






BEGIN{
   out="/dev/null"        
   out_count=0            
   err=0                  
   sort=0                 
   array[""]=""
}


NR==1 && out == "/dev/null" {
   print "out=output.file must be given on the command line"


}


$1 == "PNG_DFN_START_SORT"{
   sort=0+$2
   next
}

$1 ~ /^PNG_DFN_END_SORT/{


   for (entry in array) {
      while (array[entry] != "") {
         key = entry
         value = array[key]
         array[key] = ""

         for (alt in array) {
            if (array[alt] != "" && alt < key) {
               array[key] = value
               value = array[alt]
               key = alt
               array[alt] = ""
            }
         }

         print value >out
      }
   }
   sort=0
   next
}

/^[^"]*PNG_DFN *".*"[^"]*$/{









   orig=$0
   line=$0
   lineno=FNR
   if (lineno == "") lineno=NR

   if (sub(/^[^"]*PNG_DFN *"/,"",line) != 1) {
	print "line", lineno ": processing failed:"
	print orig
	err=1
       next
   } else {
	++out_count
   }






















   while (1) {

      if (line ~ /@"/) {
         if (line ~ /@".*"@/) {
            # Do this special case first to avoid swallowing extra spaces
            # before or after the @ stuff:
            if (!sub(/@" *"@/, "", line)) {
               # Ok, do it in pieces - there has to be a non-space between the
               # two.  NOTE: really weird things happen if a leading @" is

               if (!sub(/@" */, "", line) || !sub(/ *"@/, "", line)) {
                  print "line", lineno, ": internal error:", orig
                  exit 1
               }
            }
         }


         else while (1) {
            if (getline nextline) {


               if (nextline !~ /^#/) {
                  line = line " " nextline
                  break
               }
            } else {

               print "line", lineno ": unbalanced @\" ... \"@ pair"
               err=1
               next
            }
         }


         continue
      }



      if (sub(/"[^"]*$/, "", line))
         break


      while (1) {
         if (getline nextline) {
            if (nextline !~ /^#/) {
               line = line " " nextline

               break
            }
         } else {
            print "line", lineno ": unterminated PNG_DFN string"
            err=1
            next
         }
      }
   }



   gsub(/@'/,"\"", line)

   # Remove any trailing spaces (not really required, but for
   # editorial consistency
   sub(/ *$/, "", line)

   # Remove trailing CR
   sub(/$/, "", line)

   if (sort) {
      if (split(line, parts) < sort) {
         print "line", lineno ": missing sort field:", line
         err=1
      } else
         array[parts[sort]] = line
   }

   else
      print line >out
   next
}

/PNG_DFN/{
   print "line", NR, "incorrectly formatted PNG_DFN line:"
   print $0
   err = 1
}

END{
   if (out_count > 0 || err > 0)
	exit err

   print "no definition lines found"
   exit 1
}
