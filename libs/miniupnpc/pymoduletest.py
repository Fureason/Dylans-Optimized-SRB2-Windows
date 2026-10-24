#! /usr/bin/env python
# vim: tabstop=2 shiftwidth=2 expandtab
# MiniUPnP project
# Author : Thomas Bernard
# Python 3
# This Sample code is public domain.
# website : https://miniupnp.tuxfamily.org/


import miniupnpc
import sys

try:
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('-m', '--multicastif')
  parser.add_argument('-p', '--minissdpdsocket')
  parser.add_argument('-d', '--discoverdelay', type=int, default=200)
  parser.add_argument('-z', '--localport', type=int, default=0)

  u = miniupnpc.UPnP(**vars(parser.parse_args()))
except:
  print('argparse not available')
  i = 1
  multicastif = None
  minissdpdsocket = None
  discoverdelay = 200
  localport = 0
  while i < len(sys.argv):
    print(sys.argv[i])
    if sys.argv[i] == '-m' or sys.argv[i] == '--multicastif':
      multicastif = sys.argv[i+1]
    elif sys.argv[i] == '-p' or sys.argv[i] == '--minissdpdsocket':
      minissdpdsocket = sys.argv[i+1]
    elif sys.argv[i] == '-d' or sys.argv[i] == '--discoverdelay':
      discoverdelay = int(sys.argv[i+1])
    elif sys.argv[i] == '-z' or sys.argv[i] == '--localport':
      localport = int(sys.argv[i+1])
    else:
      raise Exception('invalid argument %s' % sys.argv[i])
    i += 2

  u = miniupnpc.UPnP(multicastif, minissdpdsocket, discoverdelay, localport)

print('inital(default) values :')
print(' discoverdelay', u.discoverdelay)
print(' lanaddr', u.lanaddr)
print(' multicastif', u.multicastif)
print(' minissdpdsocket', u.minissdpdsocket)


print('Discovering... delay=%ums' % u.discoverdelay)
print('%d device(s) detected' % u.discover())

try:
  u.selectigd()
except Exception as e:
  print('Exception :', e)
  sys.exit(1)



print('local ip address :', u.lanaddr)
print('external ip address :', u.externalipaddress())
print( u.statusinfo(), u.connectiontype())
print('total bytes : sent', u.totalbytesent(), 'received', u.totalbytereceived())
print('total packets : sent', u.totalpacketsent(), 'received', u.totalpacketreceived())





port = 0
proto = 'UDP'

i = 0
while True:
	p = u.getgenericportmapping(i)
	if p==None:
		break
	print(i, p)
	(port, proto, (ihost,iport), desc, c, d, e) = p

	i = i + 1

print(u.getspecificportmapping(port, proto))
try:
  print(u.getportmappingnumberofentries())
except Exception as e:
  print('GetPortMappingNumberOfEntries() is not supported :', e)

