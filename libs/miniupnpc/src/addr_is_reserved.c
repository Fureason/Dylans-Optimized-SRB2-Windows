 
/* vim: tabstop=4 shiftwidth=4 noexpandtab
 * Project : miniupnp
 * Web : http://miniupnp.free.fr/ or https://miniupnp.tuxfamily.org/
 * Author : Thomas BERNARD
 * copyright (c) 2005-2021 Thomas Bernard
 * This software is subjet to the conditions detailed in the
 * provided LICENSE file. */
#ifdef _WIN32
 
#include <winsock2.h>
#include <ws2tcpip.h>
#if !defined(_MSC_VER)
#include <stdint.h>
#else  
typedef unsigned long uint32_t;
#endif  
#else  
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif  

 
#define IP(a, b, c, d) (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))
#define MSK(m) (32-(m))
static const struct { uint32_t address; uint32_t rmask; } reserved[] = {
	{ IP(  0,   0,   0, 0), MSK( 8) },  
	{ IP( 10,   0,   0, 0), MSK( 8) },  
	{ IP(100,  64,   0, 0), MSK(10) },  
	{ IP(127,   0,   0, 0), MSK( 8) },  
	{ IP(169, 254,   0, 0), MSK(16) },  
	{ IP(172,  16,   0, 0), MSK(12) },  
	{ IP(192,   0,   0, 0), MSK(24) },  
	{ IP(192,   0,   2, 0), MSK(24) },  
	{ IP(192,  31, 196, 0), MSK(24) },  
	{ IP(192,  52, 193, 0), MSK(24) },  
	{ IP(192,  88,  99, 0), MSK(24) },  
	{ IP(192, 168,   0, 0), MSK(16) },  
	{ IP(192, 175,  48, 0), MSK(24) },  
	{ IP(198,  18,   0, 0), MSK(15) },  
	{ IP(198,  51, 100, 0), MSK(24) },  
	{ IP(203,   0, 113, 0), MSK(24) },  
	{ IP(224,   0,   0, 0), MSK( 4) },  
	{ IP(240,   0,   0, 0), MSK( 4) },  
};
#undef IP
#undef MSK

   
                 
   
int addr_is_reserved(const char * addr_str)
{
	uint32_t addr_n, address;
	size_t i;

#if defined(_WIN32) && _WIN32_WINNT < 0x0600  
	addr_n = inet_addr(addr_str);
	if (addr_n == INADDR_NONE)
		return 1;
#else
	 
	if (inet_pton(AF_INET, addr_str, &addr_n) <= 0) {
		 
		return 1;
	}
#endif

	address = ntohl(addr_n);

	for (i = 0; i < sizeof(reserved)/sizeof(reserved[0]); ++i) {
		if ((address >> reserved[i].rmask) == (reserved[i].address >> reserved[i].rmask))
			return 1;
	}

	return 0;
}
