// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2023 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "doomdef.h"
#include "m_cond.h"
#include "deh_soc.h"
#include "deh_tables.h"

boolean deh_loaded = false;

boolean gamedataadded = false;
boolean titlechanged = false;
boolean introchanged = false;
boolean bootmapchanged = false;

static int dbg_line;
static INT32 deh_num_warning = 0;

FUNCPRINTF void deh_warning(const char *first, ...)
{
	va_list argptr;
	char *buf = Z_Malloc(1000, PU_STATIC, NULL);

	va_start(argptr, first);
	vsnprintf(buf, 1000, first, argptr);  
	va_end(argptr);

	if(dbg_line == -1)  
		CONS_Alert(CONS_WARNING, "%s\n", buf);
	else
		CONS_Alert(CONS_WARNING, "Line %u: %s\n", dbg_line, buf);

	deh_num_warning++;

	Z_Free(buf);
}

void deh_strlcpy(char *dst, const char *src, size_t size, const char *warntext)
{
	size_t len = strlen(src)+1;  
	if (len > size)
		deh_warning("%s exceeds max length of %s", warntext, sizeu1(size-1));
	strlcpy(dst, src, size);
}

ATTRINLINE static FUNCINLINE char myfget_color(MYFILE *f)
{
	char c = *f->curpos++;
	if (c == '^')  
		return '^';

	if (c >= '0' && c <= '9')
		return 0x80+(c-'0');

	c = tolower(c);

	if (c >= 'a' && c <= 'f')
		return 0x80+10+(c-'a');

	return 0x80;  
}

ATTRINLINE static FUNCINLINE char myfget_hex(MYFILE *f)
{
	char c = *f->curpos++, endchr = 0;
	if (c == '\\')  
		return '\\';

	if (c >= '0' && c <= '9')
		endchr += (c-'0') << 4;
	else if (c >= 'A' && c <= 'F')
		endchr += ((c-'A') + 10) << 4;
	else if (c >= 'a' && c <= 'f')
		endchr += ((c-'a') + 10) << 4;
	else  
		return '?';

	c = *f->curpos++;
	if (c >= '0' && c <= '9')
		endchr += (c-'0');
	else if (c >= 'A' && c <= 'F')
		endchr += ((c-'A') + 10);
	else if (c >= 'a' && c <= 'f')
		endchr += ((c-'a') + 10);
	else  
		return '?';

	return endchr;
}

char *myfgets(char *buf, size_t bufsize, MYFILE *f)
{
	size_t i = 0;
	if (myfeof(f))
		return NULL;

	bufsize--;
	while (i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if (c == '^')
			buf[i++] = myfget_color(f);
		else if (c == '\\')
			buf[i++] = myfget_hex(f);
		else if (c != '\r')
			buf[i++] = c;
		if (c == '\n')
			break;
	}
	buf[i] = '\0';

	dbg_line++;
	return buf;
}

char *myhashfgets(char *buf, size_t bufsize, MYFILE *f)
{
	size_t i = 0;
	if (myfeof(f))
		return NULL;

	bufsize--;
	while (i < bufsize && !myfeof(f))
	{
		char c = *f->curpos++;
		if (c == '^')
			buf[i++] = myfget_color(f);
		else if (c == '\\')
			buf[i++] = myfget_hex(f);
		else if (c != '\r')
			buf[i++] = c;
		if (c == '\n')  
			dbg_line++;
		if (c == '#')
		{
			if (i > 0)  
				i--;  
			break;
		}
	}
	if (buf[i] != '#')  
		i++;
	buf[i] = '\0';

	return buf;
}



static void ignorelines(MYFILE *f)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '\n')
				break;
		}
	} while (!myfeof(f));
	Z_Free(s);
}

void ignorelinesuntilhash(MYFILE *f)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	do
	{
		if (myfgets(s, MAXLINELEN, f))
		{
			if (s[0] == '#')
				break;
		}
	} while (!myfeof(f));
	Z_Free(s);
}

static void DEH_LoadDehackedFile(MYFILE *f, boolean mainfile)
{
	char *s = Z_Malloc(MAXLINELEN, PU_STATIC, NULL);
	char textline[MAXLINELEN];
	char *word;
	char *word2;
	INT32 i;

	if (!deh_loaded)
	{
		initfreeslots();
		deh_loaded = true;
	}

	deh_num_warning = 0;

	gamedataadded = titlechanged = introchanged = bootmapchanged = false;


	dbg_line = -1;  
	while (!myfeof(f))
	{
		myfgets(s, MAXLINELEN, f);
		memcpy(textline, s, MAXLINELEN);
		if (s[0] == '\n' || s[0] == '#')
			continue;

		if (NULL != (word = strtok(s, " "))) {
			strupr(word);
			if (word[strlen(word)-1] == '\n')
				word[strlen(word)-1] = '\0';
		}
		if (word)
		{
			if (fastcmp(word, "FREESLOT"))
			{
				readfreeslots(f);
				continue;
			}
			else if (fastcmp(word, "MAINCFG"))
			{
				readmaincfg(f);
				continue;
			}
			else if (fastcmp(word, "WIPES"))
			{
				readwipes(f);
				continue;
			}
			word2 = strtok(NULL, " ");
			if (word2) {
				strupr(word2);
				if (word2[strlen(word2) - 1] == '\n')
					word2[strlen(word2) - 1] = '\0';
				i = atoi(word2);
			}
			else
				i = 0;
			if (fastcmp(word, "CHARACTER"))
			{
				readPlayer(f, i);
				continue;
			}
			else if (fastcmp(word, "EMBLEM"))
			{
				if (!mainfile && !gamedataadded)
				{
					deh_warning("You must define a custom gamedata to use \"%s\"", word);
					ignorelines(f);
				}
				else
				{
					if (!word2)
						i = numemblems + 1;

					if (i > 0 && i <= MAXEMBLEMS)
					{
						if (numemblems < i)
							numemblems = i;
						reademblemdata(f, i);
					}
					else
					{
						deh_warning("Emblem number %d out of range (1 - %d)", i, MAXEMBLEMS);
						ignorelines(f);
					}
				}
				continue;
			}
			else if (fastcmp(word, "EXTRAEMBLEM"))
			{
				if (!mainfile && !gamedataadded)
				{
					deh_warning("You must define a custom gamedata to use \"%s\"", word);
					ignorelines(f);
				}
				else
				{
					if (!word2)
						i = numextraemblems + 1;

					if (i > 0 && i <= MAXEXTRAEMBLEMS)
					{
						if (numextraemblems < i)
							numextraemblems = i;
						readextraemblemdata(f, i);
					}
					else
					{
						deh_warning("Extra emblem number %d out of range (1 - %d)", i, MAXEXTRAEMBLEMS);
						ignorelines(f);
					}
				}
				continue;
			}
			if (word2)
			{
				if (fastcmp(word, "THING") || fastcmp(word, "MOBJ") || fastcmp(word, "OBJECT"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_mobjtype(word2);  
					if (i < NUMMOBJTYPES && i > 0)
						readthing(f, i);
					else
					{
						deh_warning("Thing %d out of range (1 - %d)", i, NUMMOBJTYPES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SKINCOLOR") || fastcmp(word, "COLOR"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_skincolor(word2);  
					if (i && i < numskincolors)
						readskincolor(f, i);
					else
					{
						deh_warning("Skincolor %d out of range (1 - %d)", i, numskincolors-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SPRITE2"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_sprite2(word2);  
					if (i < (INT32)free_spr2 && i >= (INT32)SPR2_FIRSTFREESLOT)
						readsprite2(f, i);
					else
					{
						deh_warning("Sprite2 number %d out of range (%d - %d)", i, SPR2_FIRSTFREESLOT, free_spr2-1);
						ignorelines(f);
					}
				}
#ifdef HWRENDER
				else if (fastcmp(word, "LIGHT"))
				{

					if (i > 0 && i < NUMLIGHTS)
						readlight(f, i);
					else
					{
						deh_warning("Light number %d out of range (1 - %d)", i, NUMLIGHTS-1);
						ignorelines(f);
					}
				}
#endif
				else if (fastcmp(word, "SPRITE") || fastcmp(word, "SPRITEINFO"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_sprite(word2);  
					if (i < NUMSPRITES && i > 0)
						readspriteinfo(f, i, false);
					else
					{
						deh_warning("Sprite number %d out of range (0 - %d)", i, NUMSPRITES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SPRITE2INFO"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_sprite2(word2);  
					if (i < NUMPLAYERSPRITES && i >= 0)
						readspriteinfo(f, i, true);
					else
					{
						deh_warning("Sprite2 number %d out of range (0 - %d)", i, NUMPLAYERSPRITES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "LEVEL"))
				{




					if (word2[0] >= 'A' && word2[0] <= 'Z')
						i = M_MapNumber(word2[0], word2[1]);

					if (i > 0 && i <= NUMMAPS)
						readlevelheader(f, i);
					else
					{
						deh_warning("Level number %d out of range (1 - %d)", i, NUMMAPS);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "GAMETYPE"))
				{



					INT32 c;
					for (c = 0; c < MAXLINELEN; c++)
					{
						if (textline[c] == '\0')
							break;
						if (textline[c] == ' ')
						{
							char *gtname = (textline+c+1);
							if (gtname)
							{

								INT32 j;
								for (j = 0; j < (MAXLINELEN - c); j++)
								{
									if (gtname[j] == '\0')
										break;
									if (gtname[j] < 32)
										gtname[j] = '\0';
								}
								readgametype(f, gtname);
							}
							break;
						}
					}
				}
				else if (fastcmp(word, "CUTSCENE"))
				{
					if (i > 0 && i < 129)
						readcutscene(f, i - 1);
					else
					{
						deh_warning("Cutscene number %d out of range (1 - 128)", i);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "PROMPT"))
				{
					if (i > 0 && i < MAX_PROMPTS)
						readtextprompt(f, i - 1);
					else
					{
						deh_warning("Prompt number %d out of range (1 - %d)", i, MAX_PROMPTS);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "FRAME") || fastcmp(word, "STATE"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_state(word2);  
					if (i < NUMSTATES && i >= 0)
						readframe(f, i);
					else
					{
						deh_warning("Frame %d out of range (0 - %d)", i, NUMSTATES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SOUND"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_sfx(word2);  
					if (i < NUMSFX && i > 0)
						readsound(f, i);
					else
					{
						deh_warning("Sound %d out of range (1 - %d)", i, NUMSFX-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "HUDITEM"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_huditem(word2);  
					if (i >= 0 && i < NUMHUDITEMS)
						readhuditem(f, i);
					else
					{
						deh_warning("HUD item number %d out of range (0 - %d)", i, NUMHUDITEMS-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "MENU"))
				{
					if (i == 0 && word2[0] != '0')  
						i = get_menutype(word2);  
					if (i >= 1 && i < NUMMENUTYPES)
						readmenu(f, i);
					else
					{

						deh_warning("Menu number %d out of range (1 - %d)", i, NUMMENUTYPES-1);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "UNLOCKABLE"))
				{
					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						ignorelines(f);
					}
					else if (i > 0 && i <= MAXUNLOCKABLES)
						readunlockable(f, i - 1);
					else
					{
						deh_warning("Unlockable number %d out of range (1 - %d)", i, MAXUNLOCKABLES);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "CONDITIONSET"))
				{
					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						ignorelines(f);
					}
					else if (i > 0 && i <= MAXCONDITIONSETS)
						readconditionset(f, (UINT8)i);
					else
					{
						deh_warning("Condition set number %d out of range (1 - %d)", i, MAXCONDITIONSETS);
						ignorelines(f);
					}
				}
				else if (fastcmp(word, "SRB2"))
				{
					if (isdigit(word2[0]))
					{
						i = atoi(word2);
						if (i != PATCHVERSION)
						{
							deh_warning(
									"Patch is for SRB2 version %d, "
									"only version %d is supported",
									i,
									PATCHVERSION
							);
						}
					}
					else
					{
						deh_warning(
								"SRB2 version definition has incorrect format, "
								"use \"SRB2 %d\"",
								PATCHVERSION
						);
					}
				}




				else if (fastcmp(word, "CLEAR"))
				{
					boolean clearall = (fastcmp(word2, "ALL"));

					if (!mainfile && !gamedataadded)
					{
						deh_warning("You must define a custom gamedata to use \"%s\"", word);
						continue;
					}

					if (clearall || fastcmp(word2, "UNLOCKABLES"))
						clear_unlockables();

					if (clearall || fastcmp(word2, "EMBLEMS"))
						clear_emblems();

					if (clearall || fastcmp(word2, "EXTRAEMBLEMS"))
					{
						memset(&extraemblems, 0, sizeof(extraemblems));
						numextraemblems = 0;
					}

					if (clearall || fastcmp(word2, "CONDITIONSETS"))
						clear_conditionsets();

					if (clearall || fastcmp(word2, "LEVELS"))
						clear_levels();
				}
				else
					deh_warning("Unknown word: %s", word);
			}
			else
				deh_warning("missing argument for '%s'", word);
		}
		else
			deh_warning("No word in this line: %s", s);
	}  

	if (gamedataadded)
		G_LoadGameData(clientGamedata);

	if (gamestate == GS_TITLESCREEN)
	{
		if (bootmapchanged && bootmap)
		{
			menuactive = false;
			D_MapChange(bootmap, gametype, ultimatemode, true, 0, false, false);
		}
		else if (introchanged)
		{
			menuactive = false;
			I_UpdateMouseGrab();
			COM_BufAddText("playintro");
		}
		else if (titlechanged)
		{
			menuactive = false;
			I_UpdateMouseGrab();
			COM_BufAddText("exitgame");  
		}
	}

	dbg_line = -1;
	if (deh_num_warning)
	{
		CONS_Printf(M_GetText("%d warning%s in the SOC lump\n"), deh_num_warning, deh_num_warning == 1 ? "" : "s");
		if (devparm)
			I_Error("%s%s",va(M_GetText("%d warning%s in the SOC lump\n"), deh_num_warning, deh_num_warning == 1 ? "" : "s"), M_GetText("See log.txt for details.\n"));
	}

	Z_Free(s);
}



void DEH_LoadDehackedLumpPwad(UINT16 wad, UINT16 lump, boolean mainfile)
{
	MYFILE f;
	f.wad = wad;
	f.size = W_LumpLengthPwad(wad, lump);
	f.data = Z_Malloc(f.size + 1, PU_STATIC, NULL);
	W_ReadLumpPwad(wad, lump, f.data);
	f.curpos = f.data;
	f.data[f.size] = 0;
	DEH_LoadDehackedFile(&f, mainfile);
	Z_Free(f.data);
}

void DEH_LoadDehackedLump(lumpnum_t lumpnum)
{
	DEH_LoadDehackedLumpPwad(WADFILENUM(lumpnum),LUMPNUM(lumpnum), false);
}
