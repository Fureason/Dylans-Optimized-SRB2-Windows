#! /usr/bin/env python
# $Id: testupnpigd.py,v 1.7 2020/04/06 10:23:02 nanard Exp $
# MiniUPnP project
# Author : Thomas Bernard
# This Sample code is public domain.
# website : https://miniupnp.tuxfamily.org/


import miniupnpc
import socket

try:
    from http.server import BaseHTTPRequestHandler, HTTPServer
except ImportError:
    from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer


def list_redirections():
	i = 0
	while True:
		p = u.getgenericportmapping(i)
		if p==None:
			break
		print(i, p)
		i = i + 1


class handler_class(BaseHTTPRequestHandler):
	def do_GET(self):
		self.send_response(200)
		self.end_headers()
		self.wfile.write(b"OK MON GARS")


u = miniupnpc.UPnP()





u.discoverdelay = 200;

try:
	print('Discovering... delay=%ums' % u.discoverdelay)
	ndevices = u.discover()
	print(ndevices, 'device(s) detected')


	u.selectigd()

	print('local ip address :', u.lanaddr)
	externalipaddress = u.externalipaddress()
	print('external ip address :', externalipaddress)
	print(u.statusinfo(), u.connectiontype())


	httpd = HTTPServer((u.lanaddr, 0), handler_class)
	eport = httpd.server_port


	r = u.getspecificportmapping(eport, 'TCP')
	while r != None and eport < 65536:
		eport = eport + 1
		r = u.getspecificportmapping(eport, 'TCP')

	print('trying to redirect %s port %u TCP => %s port %u TCP' % (externalipaddress, eport, u.lanaddr, httpd.server_port))

	b = u.addportmapping(eport, 'TCP', u.lanaddr, httpd.server_port,
	                    'UPnP IGD Tester port %u' % eport, '')
	if b:
		print('Success. Now waiting for some HTTP request on http://%s:%u' % (externalipaddress ,eport))
		try:
			httpd.handle_request()
			httpd.server_close()
		except KeyboardInterrupt as details:
			print("CTRL-C exception!", details)
		b = u.deleteportmapping(eport, 'TCP')
		if b:
			print('Successfully deleted port mapping')
		else:
			print('Failed to remove port mapping')
	else:
		print('Failed')

	httpd.server_close()

except Exception as e:
	print('Exception :', e)
