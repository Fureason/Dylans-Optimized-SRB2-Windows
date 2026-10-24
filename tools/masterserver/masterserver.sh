#!/bin/sh -e





. /lib/lsb/init-functions



SRB2MS=./server
SRB2MS_PORT=28900


[ -x $SRB2MS ] || exit 0;

case "$1" in
	start)
		log_begin_msg "Starting SRB2MS...\n"
		umask 002
		if exec $SRB2MS $SRB2MS_PORT & then
			log_end_msg 0
		else
			log_end_msg $?
		fi
	;;

	stop)
		log_begin_msg "Stopping SRB2MS...\n"
		if killall $SRB2MS -q & then
			log_end_msg 0
		else
			log_end_msg $?
		fi
	;;

	restart|force-reload)
		"$0" stop && "$0" start
	;;

	*)
	echo "Usage: $0 {start|stop|restart|force-reload}"
		exit 1
	;;
esac

exit 0
