#!/bin/sh


upnpc -s | sed -n -e 's/^ExternalIPAddress = \([0-9.]*\)$/\1/p'
