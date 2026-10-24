#!/bin/awk -f
# scripts/options.awk - library build configuration control
#
# last changed in libpng version 1.6.11 - June 5, 2014
#
# Copyright (c) 1998-2014 Glenn Randers-Pehrson
#
# This code is released under the libpng license.
# For conditions of distribution and use, see the disclaimer
# and license in png.h





















BEGIN{
   out=""                        
   pre=-1                        
   version="libpng version unknown"  
   version_file=""               
   err=0                         










   dq="@'"                       
   start=" PNG_DFN \""           
   end="\" "                     
   subs="@\" "                   
   sube=" \"@"                   
   comment=start "/*"            
   cend="*/" end                 
   def=start "#define PNG_"      
   sup="_SUPPORTED" end          
   und=comment "#undef PNG_"     
   une="_SUPPORTED" cend         
   error=start "ERROR:"          


   deb=0                         
   everything=""                 
   logunsupported=0              



   option[""] = ""     
   done[""] = 1        
   requires[""] = ""   
   iffs[""] = ""       
   enabledby[""] = ""  
   sets[""] = ""       
   setval[""] = ""     

   setting[""] = ""    
   defaults[""] = ""   
   doneset[""] = 1     
   r[""] = ""          


   protect = ""
}


out == "" {
   print "out=output.file must be given on the command line"
   err = 1
   exit 1
}




pre == -1{
   if ($0 == "PREPROCESSED") {
      pre = 0
      next
   } else {
      pre = 1
      print "PREPROCESSED" >out

   }
}



pre && version == "search" && version_file == ""{
   version_file = FILENAME
}

pre && version == "search" && version_file != FILENAME{
   print "version string not found in", version_file
   err = 1
   exit 1
}

pre && version == "search" && $0 ~ /^ \* libpng version/{
   version = substr($0, 4)
   print "version =", version >out
   next
}

pre && FILENAME == version_file{
   next
}










$0 ~ /^[ 	]*version[ 	]*=/{
   sub(/^[  ]*version[  ]*=[  ]*/, "")
   version = $0
   next
}
$0 ~ /^[ 	]*everything[ 	=]*off[ 	]*$/{
   everything = "off"
   next
}
$0 ~ /^[ 	]*everything[ 	=]*on[ 	]*$/{
   everything = "on"
   next
}
$0 ~ /^[ 	]*logunsupported[ 	=]*0[ 	]*$/{
   logunsupported = 0
   next
}
$0 ~ /^[ 	]*logunsupported[ 	=]*1[ 	]*$/{
   logunsupported = 1
   next
}
$1 == "deb" && $2 == "=" && NF == 3{
   deb = $3
   next
}




pre && $1 != "chunk"{
   print >out
   next
}


























$1 ~ /^#/ || $0 ~ /^[ 	]*$/{
   next
}




$1 == "com"{
   if (NF > 1) {

      $1 = ""
      print comment $0, cend >out
   } else
      print start end >out
   next
}



$1 == "version" && NF == 1{
   if (version == "") {
      print "ERROR: no version string set"
      err = 1  
      exit 1
   }

   print comment, version, cend >out
   next
}






$1 == "file" && NF >= 2{
   print comment, $2, cend >out
   print comment, "Machine generated file: DO NOT EDIT", cend >out
   if (NF >= 3)
      print comment, "Derived from:", $3, cend >out
   protect = $4
   if (protect != "") {
      print start "#ifndef", protect end >out
      print start "#define", protect end >out
   }
   next
}

















$1 == "option" && NF >= 2{
   opt = $2
   sub(/,$/,"",opt)
   onoff = option[opt]   
   key = ""
   istart = 3
   do {
      if (istart == 1) {      
         val = getline

         if (val != 1) {  
            if (val == 0)
               print "option", opt ": ERROR: missing continuation line"
            else
               print "option", opt ": ERROR: error reading continuation line"


            err = 1  
            exit 1
         }
      }

      for (i=istart; i<=NF; ++i) {
         val=$(i)
         sub(/,$/,"",val)
         if (val == "on" || val == "off" || val == "disabled" || val =="enabled") {
            key = ""
            if (onoff != val) {

               if (onoff == "" || (onoff == "disabled" || onoff == "enabled") &&
                   (val == "on" || val == "off")) {


                  if (onoff == "" && (val == "on" || val == "off")) {
                     print "option", opt ": ERROR: turning unrecognized option", val

                     err = 1  
                     exit 1
                  }
                  onoff = val
               } else {


                  print "option", opt ": currently", onoff ": attempt to turn", val
                  break
               }
            }
         } else if (val == "requires" || val == "if" || val == "enables" || val =="sets") {
            key = val
         } else if (key == "requires") {
            requires[opt] = requires[opt] " " val
         } else if (key == "if") {
            iffs[opt] = iffs[opt] " " val
         } else if (key == "enables") {
            enabledby[val] = enabledby[val] " " opt
         } else if (key == "sets") {
            sets[opt] = sets[opt] " " val
            key = "setval"
            set = val
         } else if (key == "setval") {
            setval[opt " " set] = setval[opt " " set] " " val
         } else
            break  
      }

      istart = 1
   } while (i > NF && $0 ~ /,$/)

   if (i > NF) {

      if (onoff == "") onoff = "enabled"
      option[opt] = onoff
      next
   }

}












pre != 0 && $1 == "chunk" && NF >= 2{


   opt = $2
   sub(/,$/,"",opt)
   onoff = ""
   reqread = ""
   reqwrite = ""
   enables = ""
   req = 0
   istart = 3
   do {
      if (istart == 1) {      
         val = getline

         if (val != 1) {  
            if (val == 0)
               print "chunk", opt ": ERROR: missing continuation line"
            else
               print "chunk", opt ": ERROR: error reading continuation line"


            err = 1  
            exit 1
         }
      }


      for (i=istart; i<=NF; ++i) {
         val = $(i)
         sub(/,$/,"",val)
         if (val == "on" || val == "off" || val == "disabled") {
            if (onoff != val) {
               if (onoff == "")
                  onoff = val
               else
                  break  
            }
            req = 0
         } else if (val == "requires")
            req = 1
         else if (val == "enables")
            req = 2
         else if (req == 1){
            reqread = reqread " READ_" val
            reqwrite = reqwrite " WRITE_" val
         } else if (req == 2)
            enables = enables " " val
         else
            break  
      }

      istart = 1
   } while (i > NF && $0 ~ /,$/)

   if (i > NF) {

      print "option READ_" opt, "requires READ_ANCILLARY_CHUNKS" reqread, "enables", opt enables , onoff >out
      print "option WRITE_" opt, "requires WRITE_ANCILLARY_CHUNKS" reqwrite, "enables", opt enables, onoff >out
      next
   }

}












$1 == "setting" && (NF == 2 || NF >= 3 && ($3 == "requires" || $3 == "default")){
   reqs = ""
   deflt = ""
   isdef = 0
   key = ""
   for (i=3; i<=NF; ++i)
      if ($(i) == "requires" || $(i) == "default") {
         key = $(i)
         if (key == "default") isdef = 1
      } else if (key == "requires")
         reqs = reqs " " $(i)
      else if (key == "default")
         deflt = deflt " " $(i)
      else
         break  

   setting[$2] = reqs

   if (isdef && deflt == "")
      deflt = " "  
   defaults[$2] = deflt
   next
}





























$1 == "=" && NF == 3{
   print "#ifdef PNG_" $3 >out
   if ($2 ~ /^NO_/)
      print "#   define PNG_" $2 >out
   else
      print "#   define PNG_" $2 "_SUPPORTED" >out
   print "#endif" >out
   next
}





$1 ~ /^@/{

   $1 = substr($1, 2)
   print >out
   next
}




{
   print "options.awk: bad line (" NR "):", $0
   err = 1  
   exit 1
}



















END{

   if (err) exit 1

   if (pre) {

      print "deb =", deb >out
      if (everything != "") {
         print "everything =", everything >out
      }
      print "logunsupported =", logunsupported >out
      exit 0
   }











   print "" >out
   print "/* OPTIONS */" >out
   print comment, "options", cend >out
   for (opt in enabledby) tree[opt] = 1   
   for (opt in option) if (opt != "") {
      o = option[opt]

      if (o != "on" && o != "off" && o != "disabled" && o != "enabled") {
         print "internal option error (" o ")"
         exit 1
      }
      tree[opt] = ""    
   }
   for (opt in tree) if (opt != "") {
      if (tree[opt] == 1) {
         tree[opt] = ""
         if (option[opt] != "") {
            print "internal error (1)"
            exit 1
         }


         option[opt] = "disabled"
      }

      split("", list)  


      split(requires[opt] iffs[opt] enabledby[opt], r)
      for (i in r) list[r[i]] = 1
      for (i in list) tree[opt] = tree[opt] " " i
   }


   if (deb > 2) for (i in tree) if (i != "") print i, "depends-on" tree[i]







   npending = 0
   for (opt in option) if (opt != "") {
      if (option[opt] == "on" || option[opt] == "off") {
         pending[++npending] = opt
      }
   }

   err = 0  
   while (npending > 0) {
      opt = pending[npending--]
      if (option[opt] == "on") {
         nreqs = split(requires[opt], r)
         for (j=1; j<=nreqs; ++j) {
            if (option[r[j]] == "off") {
               print "option", opt, "turned on, but requirement", r[j], "is turned off"
               err = 1
            } else if (option[r[j]] != "on") {
               option[r[j]] = "on"
               pending[++npending] = r[j]
            }
         }
      } else {
         if (option[opt] != "off") {
            print "internal error (2)"
            exit 1
         }
         nreqs = split(enabledby[opt], r)
         for (j=1; j<=nreqs; ++j) {
            if (option[r[j]] == "on") {
               print "option", opt, "turned off, but enabled by", r[j], "which is turned on"
               err = 1
            } else if (option[r[j]] != "off") {
               option[r[j]] = "off"
               pending[++npending] = r[j]
            }
         }
      }
   }
   if (err) exit 1


   print "PNG_DFN_START_SORT 2" >out



   finished = 0
   while (!finished) {
      finished = 1
      movement = 0  
      for (i in option) if (!done[i]) {
         nreqs = split(tree[i], r)
         if (nreqs > 0) {
            for (j=1; j<=nreqs; ++j) if (!done[r[j]]) {
               break
            }
            if (j<=nreqs) {
               finished = 0
               continue   
            }
         }
















         if (deb) print "option", i
         print "" >out
         print "/* option:", i, option[i] >out
         print " *   requires:  " requires[i] >out
         print " *   if:        " iffs[i] >out
         print " *   enabled-by:" enabledby[i] >out
         print " *   sets:      " sets[i], "*/" >out
         print "#undef PNG_on" >out
         print "#define PNG_on 1" >out


         nreqs = split(requires[i], r)
         for (j=1; j<=nreqs; ++j) {
            print "#ifndef PNG_" r[j] "_SUPPORTED" >out
            print "#   undef PNG_on /*!" r[j] "*/" >out



            if (option[i] == "on") {
               print error, i, "requires", r[j] end >out
            }
            print "#endif" >out
         }


         have_ifs = 0
         nreqs = split(iffs[i], r)
         print "#undef PNG_no_if" >out
         if (nreqs > 0) {
            have_ifs = 1
            print "/* if" iffs[i], "*/" >out
            print "#define PNG_no_if 1" >out
            for (j=1; j<=nreqs; ++j) {
               print "#ifdef PNG_" r[j] "_SUPPORTED" >out
               print "#   undef PNG_no_if /*" r[j] "*/" >out
               print "#endif" >out
            }
            print "#ifdef PNG_no_if /*missing if*/" >out
            print "#   undef PNG_on" >out



            if (option[i] == "on") {
               print error, i, "needs one of:", iffs[i] end >out
            }
            print "#endif" >out
         }

         print "#ifdef PNG_on /*requires, if*/" >out

         print "#   undef PNG_not_enabled" >out
         print "#   define PNG_not_enabled 1" >out
         print "   /* enabled by" enabledby[i], "*/" >out
         nreqs = split(enabledby[i], r)
         for (j=1; j<=nreqs; ++j) {
            print "#ifdef PNG_" r[j] "_SUPPORTED" >out
            print "#   undef PNG_not_enabled /*" r[j] "*/" >out


            if (option[i] == "off") {
               print error, i, "enabled by:", r[j] end >out
            }
            print "#endif" >out
         }

         print "#   ifndef PNG_" i "_SUPPORTED /*!command line*/" >out
         print "#    ifdef PNG_not_enabled /*!enabled*/" >out



         if (option[i] == "off" || option[i] == "disabled" && everything != "on" || option[i] == "enabled" && everything == "off" && !have_ifs) {
            print "#      undef PNG_on /*default off*/" >out
         } else {
            print "#      ifdef PNG_NO_" i >out
            print "#       undef PNG_on /*turned off*/" >out
            print "#      endif" >out
            print "#      ifdef PNG_NO_" i "_SUPPORTED" >out
            print "#       undef PNG_on /*turned off*/" >out
            print "#      endif" >out
         }
         print "#    endif /*!enabled*/" >out
         print "#    ifdef PNG_on" >out


         print "#      define PNG_" i "_SUPPORTED" >out
         print "#    endif" >out
         print "#   endif /*!command line*/" >out


         print "#   ifdef PNG_on" >out
         if (i ~ /^fail_/) {
            print error, i, "is on: enabled by:" iffs[i] enabledby[i] ", requires" requires[i] end >out
         } else if (i !~ /^ok_/) {
            print def i sup >out

            nreqs = split(sets[i], r)
            for (j=1; j<=nreqs; ++j) {
               print "#    ifdef PNG_set_" r[j] >out

               print error, i, "sets", r[j] ": duplicate setting" end >out
               print error, "   previous value: " end "PNG_set_" r[j] >out
               print "#    else" >out




               print "#     define PNG_set_" r[j], setval[i " " r[j]] >out
               print "#    endif" >out
            }
         }
         print "#   endif /* definition */" >out
         print "#endif /*requires, if*/" >out
         if (logunsupported || i ~ /^ok_/) {
            print "#ifndef  PNG_on" >out
            if (logunsupported) {
               print und i une >out
            }
            if (i ~ /^ok_/) {
               print error, i, "not enabled: requires:" requires[i] ", enabled by:" iffs[i] enabledby[i] end >out
            }
            print "#endif" >out
         }

         done[i] = 1
         ++movement
      }

      if (!finished && !movement) {
         print "option: loop or missing option in dependency tree, cannot process:"
         for (i in option) if (!done[i]) {
            print "  option", i, "depends on" tree[i], "needs:"
            nreqs = split(tree[i], r)
            if (nreqs > 0) for (j=1; j<=nreqs; ++j) if (!done[r[j]]) {
               print "   " r[j]
            }
         }
         exit 1
      }
   }
   print "PNG_DFN_END_SORT" >out
   print comment, "end of options", cend >out





   print "" >out
   print "/* SETTINGS */" >out
   print comment, "settings", cend >out

   print "PNG_DFN_START_SORT 2" >out
   finished = 0
   while (!finished) {
      finished = 1
      movement = 0  
      for (i in setting) if (!doneset[i]) {
         nreqs = split(setting[i], r)
         if (nreqs > 0) {


            for (j=1; j<=nreqs; ++j) if (option[r[j]] == "" && !doneset[r[j]]) {
               break
            }
            if (j<=nreqs) {
               finished = 0
               continue  
            }
         }



         if (deb) print "setting", i
         deflt = defaults[i]

         sub(/ *$/,"",deflt)


         if (deflt ~ /^ @/)
            deflt = " " subs substr(deflt, 3) sube
         print "" >out
         print "/* setting: ", i >out
         print " *   requires:" setting[i] >out
         print " *   default: ", defaults[i] deflt, "*/" >out
         for (j=1; j<=nreqs; ++j) {
            if (option[r[j]] != "")
               print "#ifndef PNG_" r[j] "_SUPPORTED" >out
            else
               print "#ifndef PNG_" r[j] >out
            print error, i, "requires", r[j] end >out
            print "# endif" >out
         }






         print "#ifdef PNG_" i >out

         print def i, subs "PNG_" i sube end >out
         print "#else /* use default */" >out
         print "# ifdef PNG_set_" i >out

         print def i, subs "PNG_set_" i sube end >out

         print "#  define PNG_" i, "1" >out
         if (defaults[i] != "") {
            print "# else /*default*/" >out
            print def i deflt end >out
            print "#  define PNG_" i, "1" >out
         }
         print "# endif /* defaults */" >out
         print "#endif /* setting", i, "*/" >out

         doneset[i] = 1
         ++movement
      }

      if (!finished && !movement) {
         print "setting: loop or missing setting in 'requires', cannot process:"
         for (i in setting) if (!doneset[i]) {
            print "  setting", i, "requires" setting[i]
         }
         exit 1
      }
   }
   print "PNG_DFN_END_SORT" >out
   print comment, "end of settings", cend >out


   if (protect != "") {
      print start "#endif", "/*", protect, "*/" end >out
   }
}
