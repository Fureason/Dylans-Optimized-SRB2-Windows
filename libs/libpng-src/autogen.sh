#! /bin/sh























clean=
maintainer=
while test $# -gt 0
do
   case "$1" in
      --maintainer)
         maintainer=1;;

      --clean)
         clean=1;;

      *)
         exec >&2
         echo "$0: usage: ./autogen.sh"
         if test -d .git
         then
            echo "  ./autogen.sh generates the configure script and"
            echo "  Makefile.in, or refreshes them after changes to Makefile.am"
            echo "  or configure.ac.  You may prefer to just run autoreconf."
         elif test -z "$maintainer"
         then
            echo "  DO NOT RUN THIS SCRIPT."
            echo "  If you need to change Makefile.am or configure.ac then you"
            echo "  also need to run ./configure --enable-maintainer-mode and"
            echo "  use the appropriate autotools, *NOT* this script, to update"
            echo "  everything, please check the documentation of autoreconf."
            echo "  WARNING: libpng is intentionally generated with a known,"
            echo "  fixed, set of autotools.  It is known *NOT* to work with"
            echo "  the collection of autotools distributed on highly reputable"
            echo "  operating systems."
            echo "  Remember: autotools is GNU software, you are expected to"
            echo "  pay for support."
         else
            echo "  You have run autogen.sh with --maintainer enabled and you"
            echo "  are not using a GIT distribution, then you have given an"
            echo "  unrecognized argument.  This is not good. --maintainer"
            echo "  switches off any assumptions that you might not know what"
            echo "  you are doing."
         fi
         exit 1;;
   esac

   shift
done







libpng_autotools_files="Makefile.in aclocal.m4 config.guess config.h.in
   config.sub configure depcomp install-sh ltmain.sh missing\
   test-driver"



libpng_autotools_extra="compile config.h.in~"


libpng_libtool_files="scripts/libtool.m4 scripts/ltoptions.m4\
   scripts/ltsugar.m4 scripts/ltversion.m4 scripts/lt~obsolete.m4"

libpng_autotools_dirs="autom4te.cache"  


libpng_configure_files="Makefile config.h config.log config.status\
   libpng-config libpng.pc libtool stamp-h1"

libpng_configure_dirs=".deps"





clean_autotools(){
   rm -rf $libpng_autotools_files $libpng_libtool_files $libpng_autotools_dirs
   rm -rf $libpng_autotools_extra
}

clean_configure(){
   rm -rf $libpng_configure_files $libpng_configure_dirs
}


if test -n "$clean"
then
   clean_configure
   if test -n "$maintainer"
   then
      clean_autotools
   fi

   exit 0
fi


libpng_autotools_file_found=
libpng_autotools_file_missing=
for file in $libpng_autotools_files
do
   if test -f  "$file"
   then
      libpng_autotools_file_found=1
   else
      libpng_autotools_file_missing=1
   fi
done



for file in $libpng_libtool_files
do
   if test ! -f "$file"
   then
      libpng_autotools_file_missing=1
   fi
done






mode=
if test -z "$libpng_autotools_file_found" -o -n "$maintainer"
then
   mode="autoreconf"
else
   if test -n "$libpng_autotools_file_missing"
   then
      mode="broken"
   else
      mode="configure"
   fi
fi


case "$mode" in
   autoreconf)

      clean_configure
      clean_autotools

      if autoreconf --warnings=all --force --install
      then
         missing=
         for file in $libpng_autotools_files
         do
            test -f "$file" || missing=1
         done

         test -z "$missing" || {
            exec >&2
            echo "autoreconf was run, but did not produce all the expected"
            echo "files.  It is likely that your autotools installation is"
            echo "not compatible with that expected by libpng."
            exit 1
         }
      else
         exec >&2
         echo "autoreconf failed: your version of autotools is incompatible"
         echo "with this libpng version.  Please use a distributed archive"
         echo "(which includes the autotools generated files) and run configure"
         echo "instead."
         exit 1
      fi;;

   configure)
      if test -d .git
      then
         exec >&2
         echo "ERROR: running autoreconf on an initialized system"
         echo "  This is not necessary; it is only necessary to remake the"
         echo "  autotools generated files if Makefile.am or configure.ac"
         echo "  change and make does the right thing with:"
         echo
         echo "     ./configure --enable-maintainer-mode."
         echo
         echo "  You can run autoreconf yourself if you don't like maintainer"
         echo "  mode and you can also just run autoreconf -f -i to initialize"
         echo "  everything in the first place; this script is only for"
         echo "  compatibility with prior releases."
         exit 1
      else
         exec >&2
         echo "autogen.sh is intended only to generate 'configure' on systems"
         echo "that do not have it.  You have a complete 'configure', if you"
         echo "need to change Makefile.am or configure.ac you also need to"
         echo "run configure with the --enable-maintainer-mode option."
         exit 1
      fi;;

   broken)
      exec >&2
      echo "Your system has a partial set of autotools generated files."
      echo "autogen.sh is unable to proceed.  The full set of files is"
      echo "contained in the libpng 'tar' distribution archive and you do"
      echo "not need to run autogen.sh if you use it."
      exit 1;;
esac
