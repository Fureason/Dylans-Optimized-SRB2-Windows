#! /bin/sh
#
# Copyright (C) 2003, 2005-2007 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
# USA.
#


if test "X`(echo '\t') 2>/dev/null`" = 'X\t'; then
  echo='echo'
else
  if test "X`(printf '%s\n' '\t') 2>/dev/null`" = 'X\t'; then
    echo='printf %s\n'
  else
    echo_func () {
      cat <<EOT
$*
EOT
    }
    echo='echo_func'
  fi
fi







if test -z "$ZSH_VERSION"; then





  case "$0" in
    gettext.sh | */gettext.sh | *\\gettext.sh)
      progname=$0
      package=gettext-runtime
      version=0.18.1


      func_usage ()
      {
        echo "GNU gettext shell script function library version $version"
        echo "Usage: . gettext.sh"
      }


      func_version ()
      {
        echo "$progname (GNU $package) $version"
        echo "Copyright (C) 2003-2007 Free Software Foundation, Inc.
License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law."
        echo "Written by" "Bruno Haible"
      }
      if test $# = 1; then
        case "$1" in
          --help | --hel | --he | --h )
            func_usage; exit 0 ;;
          --version | --versio | --versi | --vers | --ver | --ve | --v )
            func_version; exit 0 ;;
        esac
      fi
      func_usage 1>&2
      exit 1
      ;;
  esac
fi




eval_gettext () {
  gettext "$1" | (export PATH `envsubst --variables "$1"`; envsubst "$1")
}




eval_ngettext () {
  ngettext "$1" "$2" "$3" | (export PATH `envsubst --variables "$1 $2"`; envsubst "$1 $2")
}

























