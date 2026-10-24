/*
 * Function to write the SRB2 end message text
 *
 * Copyright (C) 1998 by Udo Munk <udo@umserver.umnet.de>
 *
 * This code is provided AS IS and there are no guarantees, none.
 * Feel free to share and modify.
 */







#include <stdio.h>
#include <stdlib.h>


#include "../doomdef.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "endtxt.h"
                                  


                                                                             

             


  

void ShowEndTxt(void)
{
	INT32 i;
	UINT16 j, att = 0;
	INT32 nlflag = 1;
#ifdef _WIN32
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode, bytesWritten;
	CONSOLE_SCREEN_BUFFER_INFO backupcon;
	COORD resizewin = {80,-1};
	CHAR let = 0;
#endif
	UINT16 *ptext;
	void *data;
	lumpnum_t endoomnum = W_GetNumForName("ENDOOM");


	 
	                                                                   
                           
           
                     
            
  
   

	 
	data = ptext = W_CacheLumpNum(endoomnum, PU_CACHE);

#ifdef _WIN32
	if (co == INVALID_HANDLE_VALUE || GetFileType(co) != FILE_TYPE_CHAR || !GetConsoleMode(co, &mode))  
	{
		Z_Free(data);
		return;
	}

	backupcon.wAttributes = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;  
	GetConsoleScreenBufferInfo(co, &backupcon);  
	resizewin.Y = backupcon.dwSize.Y;
	if (backupcon.dwSize.X < resizewin.X)
		SetConsoleScreenBufferSize(co, resizewin);

	for (i=1; i<=80*25; i++)  
	{
		j = (UINT16)(*ptext >> 8);  
		let = (char)(*ptext & 0xff);  
		if (j != att)  
		{
			att = j;  
			SetConsoleTextAttribute(co, j);  
		}

		WriteConsoleA(co, &let,  1, &bytesWritten, NULL);  

		if (nlflag && !(i % 80) && backupcon.dwSize.X > resizewin.X)  
		{
			att = backupcon.wAttributes;
			SetConsoleTextAttribute(co, att);  
			WriteConsoleA(co, "\n",  1, &bytesWritten, NULL);  
		}
		ptext++;
	}
	SetConsoleTextAttribute(co, backupcon.wAttributes);  
#else
	 
	for (i=1; i<=80*25; i++) {
		 
		 
		if ((j = *ptext >> 8) != att) {
			 
			att = j;
			 
			printf("\033[");
			switch (j & 0x0f) {
			case 0:		 
				printf("30");
				break;
			case 1:		 
				printf("34");
				break;
			case 2:		 
				printf("32");
				break;
			case 3:		 
				printf("36");
				break;
			case 4:		 
				printf("31");
				break;
			case 5:		 
				printf("35");
				break;
			case 6:		 
				printf("33");
				break;
			case 7:		 
				printf("37");
				break;
			case 8:		 
				printf("1;30");
				break;
			case 9:		 
				printf("1;34");
				break;
			case 10:	 
				printf("1;32");
				break;
			case 11:	 
				printf("1;36");
				break;
			case 12:	 
				printf("1;31");
				break;
			case 13:	 
				printf("1;35");
				break;
			case 14:	 
				printf("1;33");
				break;
			case 15:	 
				printf("1;37");
				break;
			}
			printf("m");
			 
			printf("\033[");
			switch ((j >> 4) & 0x0f) {
			case 0:		 
				printf("40");
				break;
			case 1:		 
				printf("44");
				break;
			case 2:		 
				printf("42");
				break;
			case 3:		 
				printf("46");
				break;
			case 4:		 
				printf("41");
				break;
			case 5:		 
				printf("45");
				break;
			case 6:		 
				printf("43");
				break;
			case 7:		 
				printf("47");
				break;
			case 8:		 
				printf("1;40");
				break;
			case 9:		 
				printf("1;44");
				break;
			case 10:	 
				printf("1;42");
				break;
			case 11:	 
				printf("1;46");
				break;
			case 12:	 
				printf("1;41");
				break;
			case 13:	 
				printf("1;45");
				break;
			case 14:	 
				printf("1;43");
				break;
			case 15:	 
				printf("1;47");
				break;
			}
			printf("m");
		}

		 
		printf("%c",*ptext++ & 0xff);

		 
		if (nlflag)
		{
			if (!(i % 80))
			{
				printf("\033[0m");
				att = 0;
				printf("\n");
			}
		}
	}
	 
	printf("\033[0m");
#endif
	if (nlflag)
		printf("\n");

	Z_Free(data);
}
