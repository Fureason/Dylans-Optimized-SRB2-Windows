// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __I_NET__
#define __I_NET__

#ifdef __GNUG__
#pragma interface
#endif

#include "../doomdef.h"
#include "../command.h"


#define DOOMCOM_ID (INT32)0x12345678l



#define MAXPACKETLENGTH 1450


#define INETPACKETLENGTH 1024

extern INT16 hardware_MAXPACKETLENGTH;

#if defined(_MSC_VER)
#pragma pack(1)
#endif

typedef struct
{

	INT16 remotenode;

	INT16 datalength;



	INT16 numnodes;

	INT16 extratics;


	INT16 numslots;


	char data[MAXPACKETLENGTH];
} ATTRPACK doomcom_t;

#if defined(_MSC_VER)
#pragma pack()
#endif

extern doomcom_t *doomcom;

                                          
  
extern boolean (*I_NetGet)(void);

                                            
  
extern void (*I_NetSend)(void);

                             

                                 

             


  
extern void (*I_NetFreeNodenum)(INT32 nodenum);

   
                                            

                               

                                                             

                          
  
extern char *I_NetSplitAddress(char *address, char **port);

                                                   

                                     

                       


  
extern SINT8 I_NetMakeNode(const char *address);

                                                            

                                     

                               

                       


  
extern SINT8 (*I_NetMakeNodewPort)(const char *address, const char *port);

                          
  
extern boolean (*I_NetOpenSocket)(void);

                                                                
  
extern void (*I_NetCloseSocket)(void);


extern boolean (*I_Ban) (INT32 node);
extern void (*I_ClearBans)(void);
extern const char *(*I_GetNodeAddress) (INT32 node);
extern const char *(*I_GetBanAddress) (size_t ban);
extern const char *(*I_GetBanMask) (size_t ban);
extern boolean (*I_SetBanAddress) (const char *address,const char *mask);
extern boolean *bannednode;


boolean I_InitNetwork(void);

#endif
