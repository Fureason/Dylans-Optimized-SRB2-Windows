#!/bin/awk -f













BEGIN{
   err=0
   master=""         
   official[1] = ""  
   symbol[1] = ""    
   removed[1] = ""   
   lasto = 0         
   mastero = 0       
   symbolo = 0       
   missing = "error"# log an error on missing symbols
   of="symbols.new"  
}







master == "" {
   master = FILENAME
}
FILENAME==master && NF==2 && $2~/^@/ && $1!~/^;/ {
   o=0+substr($2,2)
   if (o > 0) {
      if (official[o] == "") {
         official[o] = $1
         if (o > mastero) mastero = o
         next
      } else
         print master ": duplicated symbol:", official[o] ":", $0
   } else
      print master ": bad export line format:", $0
   err = 1
}
FILENAME==master && $1==";missing" && NF==2{




   missing = $2
}
FILENAME==master {
   next
}










NF==2 && $1 == ";" && $2 ~ /^@[1-9][0-9]*$/ {  
   o=0+substr($2,2)
   if (lasto == 0 || lasto == o)
      lasto=o
   else {
      print "png.h: duplicated last ordinal:", lasto, o
      err = 1
   }
   next
}
NF==3 && $1 == ";" && $3 ~ /^@[1-9][0-9]*$/ {  
   o=0+substr($3,2)
   if (removed[o] == "" || removed[o] == $2) {
      removed[o] = $2
      if (o > symbolo) symbolo = o
   } else {
      print "png.h: duplicated removed symbol", o ": '" removed[o] "' != '" $2 "'"
      err = 1
   }
   next
}
NF==2 && $2 ~ /^@[1-9][0-9]*$/ {  
   o=0+substr($2,2)
   if (symbol[o] == "" || symbol[o] == $1) {
      symbol[o] = $1
      if (o > symbolo) symbolo = o
   } else {
      print "png.h: duplicated symbol", o ": '" symbol[o] "' != '" $1 "'"
      err = 1
   }
}
{
   next  
}


END{
   if (symbolo > lasto) {
      print "highest symbol ordinal in png.h,", symbolo ", exceeds last ordinal from png.h", lasto
      err = 1
   }
   if (mastero > lasto) {
      print "highest symbol ordinal in", master ",", mastero ", exceeds last ordinal from png.h", lasto
      err = 1
   }
   unexported=0

   print ";Version INSERT-VERSION-HERE" >of
   print ";--------------------------------------------------------------" >of
   print "; LIBPNG symbol list as a Win32 DEF file" >of
   print "; Contains all the symbols that can be exported from libpng" >of
   print ";--------------------------------------------------------------" >of
   print "LIBRARY" >of
   print "" >of
   print "EXPORTS" >of

   for (o=1; o<=lasto; ++o) {
      if (symbol[o] == "" && removed[o] == "") {
         if (unexported == 0) unexported = o
         if (official[o] == "") {

            if (o < lasto) continue
         }
      }
      if (unexported != 0) {





         if (missing != "ignore") {
            if (o-1 > unexported)
               print "png.h:", missing ": missing symbols:", unexported "-" o-1
            else
               print "png.h:", missing ": missing symbol:", unexported
            if (missing != "warning")
               err = 1
         }
         unexported = 0
      }
      if (symbol[o] != "" && removed[o] != "") {
         print "png.h: symbol", o, "both exported as '" symbol[o] "' and removed as '" removed[o] "'"
         err = 1
      } else if (symbol[o] != official[o]) {

         err = 1
         if (symbol[o] == "")
            print "png.h: symbol", o, "is exported as '" official[o] "' in", master
         else if (official[o] == "")
            print "png.h: exported symbol", o, "'" symbol[o] "' not present in", master
         else
            print "png.h: exported symbol", o, "'" symbol[o] "' exists as '" official[o] "' in", master
      }


      if (symbol[o] != "")
         print " " symbol[o], "@" o > of
   }

   if (err != 0) {
      print "*** A new list is in", of, "***"
      exit 1
   }
}
