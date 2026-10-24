// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2022 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#include "net_command.h"
#include "tic_command.h"
#include "gamestate.h"
#include "server_connection.h"
#include "d_clisrv.h"
#include "i_net.h"
#include "../byteptr.h"
#include "../g_game.h"
#include "../console.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../doomtype.h"

typedef struct textcmdbuf_s textcmdbuf_t;

struct textcmdbuf_s
{
	textcmdbuf_t *next;
	UINT8 cmd[MAXTEXTCMD];
};

static textcmdbuf_t *textcmdbuf;
static textcmdbuf_t *textcmdbuf2;
textcmdtic_t *textcmds[TEXTCMD_HASH_SIZE] = {NULL};
UINT8 localtextcmd[MAXTEXTCMD];
UINT8 localtextcmd2[MAXTEXTCMD];  
static void (*listnetxcmd[MAXNETXCMD])(UINT8 **p, INT32 playernum);

#define ADDON_NOTICE_SLOTS 8

typedef struct
{
	boolean active;
	tic_t due;
	char actor[MAXPLAYERNAME+1];
	char filename[MAX_WADPATH+1];
} addonnotice_t;

static addonnotice_t addonnotices[ADDON_NOTICE_SLOTS];
static UINT8 nextaddonnotice;

void D_ClearAddonNotices(void)
{
	memset(addonnotices, 0, sizeof addonnotices);
	nextaddonnotice = 0;
}

void D_QueueAddonNotice(const char *actor, const char *filename)
{
	addonnotice_t *notice = &addonnotices[nextaddonnotice++ % ADDON_NOTICE_SLOTS];
	const char *name = actor && actor[0] ? actor : "SERVER";
	const char *base = filename ? filename : "add-on";


	for (const char *scan = base; *scan; scan++)
		if (*scan == '/' || *scan == '\\' || *scan == ':')
			base = scan + 1;

	strlcpy(notice->actor, name, sizeof notice->actor);
	strlcpy(notice->filename, base, sizeof notice->filename);
	notice->due = gametic + TICRATE;
	notice->active = true;
}

static void D_TickAddonNotices(void)
{
	for (size_t i = 0; i < ADDON_NOTICE_SLOTS; i++)
	{
		addonnotice_t *notice = &addonnotices[i];
		if (!notice->active || (INT32)(gametic - notice->due) < 0)
			continue;

		CONS_Printf("\x89%s added mod: %s\x80\n", notice->actor, notice->filename);
		notice->active = false;
	}
}

                                                                        
                                                                          
                                                                             
                                                                               
                                                                 
static boolean CommandWatchNetXCmd(netxcmd_t id)
{
	switch (id)
	{
		case XD_KICK:
		case XD_EXITLEVEL:
		case XD_ADDFILE:
		case XD_ADDFOLDER:
		case XD_PAUSE:
		case XD_TEAMCHANGE:
		case XD_CLEARSCORES:
		case XD_VERIFIED:
		case XD_RUNSOC:
		case XD_REQADDFILE:
		case XD_REQADDFOLDER:
		case XD_SETMOTD:
		case XD_SUICIDE:
		case XD_DEMOTED:
		case XD_MUTEPLAYER:
			return true;
		default:
			return false;
	}
}

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum))
{
#ifdef PARANOIA
	if (id >= MAXNETXCMD)
		I_Error("Command id %d too big", id);
	if (listnetxcmd[id] != 0)
		I_Error("Command id %d already used", id);
#endif
	listnetxcmd[id] = cmd_f;
}

static void WriteNetXCmd(UINT8 *cmd, netxcmd_t id, const void *param, size_t nparam)
{
	cmd[0]++;
	cmd[cmd[0]] = (UINT8)id;
	if (param && nparam)
	{
		M_Memcpy(&cmd[cmd[0]+1], param, nparam);
		cmd[0] = (UINT8)(cmd[0] + (UINT8)nparam);
	}
}

void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd[0]+2+nparam > MAXTEXTCMD)
	{
		textcmdbuf_t *buf = textcmdbuf;

		if (2+nparam > MAXTEXTCMD)
		{
			CONS_Alert(CONS_ERROR, M_GetText("packet too large to fit NetXCmd, cannot add netcmd %d! (size: %s, max: %d)\n"), id, sizeu1(2+nparam), MAXTEXTCMD);
			return;
		}


		CONS_Alert(CONS_NOTICE, M_GetText("NetXCmd buffer full, delaying netcmd %d... (size: %d, needed: %s)\n"), id, localtextcmd[0], sizeu1(nparam));
		if (buf == NULL)
		{
			textcmdbuf = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			textcmdbuf->cmd[0] = 0;
			textcmdbuf->next = NULL;
			WriteNetXCmd(textcmdbuf->cmd, id, param, nparam);
			return;
		}

		while (buf->next != NULL)
			buf = buf->next;

		if (buf->cmd[0]+2+nparam > MAXTEXTCMD)
		{
			buf->next = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			buf->next->cmd[0] = 0;
			buf->next->next = NULL;
			WriteNetXCmd(buf->next->cmd, id, param, nparam);
		}
		else
		{
			WriteNetXCmd(buf->cmd, id, param, nparam);
		}
		return;
	}
	WriteNetXCmd(localtextcmd, id, param, nparam);
}


void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam)
{
	if (localtextcmd2[0]+2+nparam > MAXTEXTCMD)
	{
		textcmdbuf_t *buf = textcmdbuf2;

		if (2+nparam > MAXTEXTCMD)
		{
			CONS_Alert(CONS_ERROR, M_GetText("packet too large to fit NetXCmd, cannot add netcmd %d! (size: %s, max: %d)\n"), id, sizeu1(2+nparam), MAXTEXTCMD);
			return;
		}


		CONS_Alert(CONS_NOTICE, M_GetText("NetXCmd buffer full, delaying netcmd %d... (size: %d, needed: %s)\n"), id, localtextcmd2[0], sizeu1(nparam));
		if (buf == NULL)
		{
			textcmdbuf2 = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			textcmdbuf2->cmd[0] = 0;
			textcmdbuf2->next = NULL;
			WriteNetXCmd(textcmdbuf2->cmd, id, param, nparam);
			return;
		}

		while (buf->next != NULL)
			buf = buf->next;

		if (buf->cmd[0]+2+nparam > MAXTEXTCMD)
		{
			buf->next = Z_Malloc(sizeof(textcmdbuf_t), PU_STATIC, NULL);
			buf->next->cmd[0] = 0;
			buf->next->next = NULL;
			WriteNetXCmd(buf->next->cmd, id, param, nparam);
		}
		else
		{
			WriteNetXCmd(buf->cmd, id, param, nparam);
		}
		return;
	}
	WriteNetXCmd(localtextcmd2, id, param, nparam);
}

UINT8 GetFreeXCmdSize(void)
{

	return (UINT8)(localtextcmd[0] - 2);
}


void D_FreeTextcmd(tic_t tic)
{
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t *textcmdtic = *tctprev;

	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}

	if (textcmdtic)
	{

		*tctprev = textcmdtic->next;


		for (INT32 i = 0; i < TEXTCMD_HASH_SIZE; i++)
		{
			textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[i];

			while (textcmdplayer)
			{
				textcmdplayer_t *tcpnext = textcmdplayer->next;
				Z_Free(textcmdplayer);
				textcmdplayer = tcpnext;
			}
		}


		Z_Free(textcmdtic);
	}
}


UINT8* D_GetExistingTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	while (textcmdtic && textcmdtic->tic != tic) textcmdtic = textcmdtic->next;


	if (textcmdtic)
	{
		textcmdplayer_t *textcmdplayer = textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
		while (textcmdplayer && textcmdplayer->playernum != playernum) textcmdplayer = textcmdplayer->next;

		if (textcmdplayer) return textcmdplayer->cmd;
	}

	return NULL;
}


UINT8* D_GetTextcmd(tic_t tic, INT32 playernum)
{
	textcmdtic_t *textcmdtic = textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdtic_t **tctprev = &textcmds[tic & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer_t *textcmdplayer, **tcpprev;


	while (textcmdtic && textcmdtic->tic != tic)
	{
		tctprev = &textcmdtic->next;
		textcmdtic = textcmdtic->next;
	}


	if (!textcmdtic)
	{
		textcmdtic = *tctprev = Z_Calloc(sizeof (textcmdtic_t), PU_STATIC, NULL);
		textcmdtic->tic = tic;
	}

	tcpprev = &textcmdtic->playercmds[playernum & (TEXTCMD_HASH_SIZE - 1)];
	textcmdplayer = *tcpprev;


	while (textcmdplayer && textcmdplayer->playernum != playernum)
	{
		tcpprev = &textcmdplayer->next;
		textcmdplayer = textcmdplayer->next;
	}


	if (!textcmdplayer)
	{
		textcmdplayer = *tcpprev = Z_Calloc(sizeof (textcmdplayer_t), PU_STATIC, NULL);
		textcmdplayer->playernum = playernum;
	}

	return textcmdplayer->cmd;
}

void ExtraDataTicker(void)
{
	for (INT32 i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i] || i == 0)
		{
			UINT8 *bufferstart = D_GetExistingTextcmd(gametic, i);

			if (bufferstart)
			{
				UINT8 *curpos = bufferstart;
				UINT8 *bufferend = &curpos[curpos[0]+1];

				curpos++;
				while (curpos < bufferend)
				{
					if (*curpos < MAXNETXCMD && listnetxcmd[*curpos])
					{
						const UINT8 id = *curpos;
						curpos++;
						if (cv_commandwatch_originals.value && netgame && !demoplayback
						&& i != consoleplayer && CommandWatchNetXCmd((netxcmd_t)id))
						{
							const char *actor = player_names[i][0] ? player_names[i]
								: ((i == serverplayer) ? "SERVER" : va("Player %d", i + 1));
							CONS_Printf("\x82%s: %s\x80\n", actor, netxcmdnames[id - 1]);
						}
						DEBFILE(va("executing x_cmd %s ply %u ", netxcmdnames[id - 1], i));
						(listnetxcmd[id])(&curpos, i);
						DEBFILE("done\n");
					}
					else
					{
						if (server)
						{
							SendKick(i, KICK_MSG_CON_FAIL | KICK_MSG_KEEP_BODY);
							DEBFILE(va("player %d kicked [gametic=%u] reason as follows:\n", i, gametic));
						}
						CONS_Alert(CONS_WARNING, M_GetText("Got unknown net command [%s]=%d (max %d)\n"), sizeu1(curpos - bufferstart), *curpos, bufferstart[0]);
						break;
					}
				}
			}
		}

	D_TickAddonNotices();




	if (client)
		D_FreeTextcmd(gametic);
}


size_t TotalTextCmdPerTic(tic_t tic)
{
	size_t total = 1;  

	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 *textcmd = D_GetExistingTextcmd(tic, i);
		if ((!i || playeringame[i]) && textcmd)
			total += 2 + textcmd[0];  
	}

	return total;
}

void PT_TextCmd(SINT8 node, INT32 netconsole)
{
	if (client)
		return;


	if (netbuffer->packettype == PT_TEXTCMD2)
		netconsole = netnodes[node].player2;

	if (netconsole < 0 || netconsole >= MAXPLAYERS)
		Net_UnAcknowledgePacket(node);
	else
	{
		size_t j;
		tic_t tic = maketic;
		UINT8 *textcmd;



		if (!netbuffer->u.textcmd[0])
		{
			DEBFILE(va("GetPacket: Textcmd with size 0 detected! (node %u, player %d)\n",
				node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}



		if (netbuffer->u.textcmd[0] > (size_t)doomcom->datalength-BASEPACKETSIZE-1)
		{
			DEBFILE(va("GetPacket: Bad Textcmd packet size! (expected %d, actual %s, node %u, player %d)\n",
			netbuffer->u.textcmd[0], sizeu1((size_t)doomcom->datalength-BASEPACKETSIZE-1),
				node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}



		j = software_MAXPACKETLENGTH
			- (netbuffer->u.textcmd[0]+2+BASESERVERTICSSIZE
			+ (doomcom->numslots+1)*sizeof(ticcmd_t));


		while ((textcmd = D_GetExistingTextcmd(tic, netconsole)),
			(TotalTextCmdPerTic(tic) > j || netbuffer->u.textcmd[0] + (textcmd ? textcmd[0] : 0) > MAXTEXTCMD)
			&& tic < firstticstosend + BACKUPTICS)
			tic++;

		if (tic >= firstticstosend + BACKUPTICS)
		{
			DEBFILE(va("GetPacket: Textcmd too long (max %s, used %s, mak %d, "
				"tosend %u, node %u, player %d)\n", sizeu1(j), sizeu2(TotalTextCmdPerTic(maketic)),
				maketic, firstticstosend, node, netconsole));
			Net_UnAcknowledgePacket(node);
			return;
		}


		if (!textcmd) textcmd = D_GetTextcmd(tic, netconsole);

		DEBFILE(va("textcmd put in tic %u at position %d (player %d) ftts %u mk %u\n",
			tic, textcmd[0]+1, netconsole, firstticstosend, maketic));

		M_Memcpy(&textcmd[textcmd[0]+1], netbuffer->u.textcmd+1, netbuffer->u.textcmd[0]);
		textcmd[0] += (UINT8)netbuffer->u.textcmd[0];
	}
}

void SV_WriteNetCommandsForTic(tic_t tic, UINT8 **buf)
{
	UINT8 *numcmds;

	numcmds = (*buf)++;
	*numcmds = 0;
	for (INT32 i = 0; i < MAXPLAYERS; i++)
	{
		UINT8 *cmd = D_GetExistingTextcmd(tic, i);
		INT32 size = cmd ? cmd[0] : 0;

		if ((!i || playeringame[i]) && size)
		{
			(*numcmds)++;
			WRITEUINT8(*buf, i);
			M_Memcpy(*buf, cmd, size + 1);
			*buf += size + 1;
		}
	}
}

void CL_CopyNetCommandsFromServerPacket(tic_t tic, UINT8 **buf)
{
	UINT8 numcmds = *(*buf)++;

	for (UINT32 i = 0; i < numcmds; i++)
	{
		INT32 playernum = *(*buf)++;  
		size_t size = (*buf)[0]+1;

		if (tic >= gametic)  
			M_Memcpy(D_GetTextcmd(tic, playernum), *buf, size);
		*buf += size;
	}
}

void CL_SendNetCommands(void)
{

	if (localtextcmd[0])
	{
		netbuffer->packettype = PT_TEXTCMD;
		M_Memcpy(netbuffer->u.textcmd,localtextcmd, localtextcmd[0]+1);

		if (HSendPacket(servernode, true, 0, localtextcmd[0]+1))  
		{
			localtextcmd[0] = 0;
			if (textcmdbuf != NULL)
			{
				textcmdbuf_t *buf = textcmdbuf;
				M_Memcpy(localtextcmd, textcmdbuf->cmd, textcmdbuf->cmd[0]+1);
				textcmdbuf = textcmdbuf->next;
				Z_Free(buf);
			}
		}
	}


	if (localtextcmd2[0])
	{
		netbuffer->packettype = PT_TEXTCMD2;
		M_Memcpy(netbuffer->u.textcmd, localtextcmd2, localtextcmd2[0]+1);

		if (HSendPacket(servernode, true, 0, localtextcmd2[0]+1))  
		{
			localtextcmd2[0] = 0;
			if (textcmdbuf2 != NULL)
			{
				textcmdbuf_t *buf = textcmdbuf2;
				M_Memcpy(localtextcmd2, textcmdbuf2->cmd, textcmdbuf2->cmd[0]+1);
				textcmdbuf2 = textcmdbuf2->next;
				Z_Free(buf);
			}
		}
	}
}

void SendKick(UINT8 playernum, UINT8 msg)
{
	UINT8 buf[2];

	if (!(server && cv_rejointimeout.value))
		msg &= ~KICK_MSG_KEEP_BODY;

	buf[0] = playernum;
	buf[1] = msg;
	SendNetXCmd(XD_KICK, &buf, 2);
}

void SendKicksForNode(SINT8 node, UINT8 msg)
{
	if (!netnodes[node].ingame)
		return;

	for (INT32 playernum = netnodes[node].player; playernum != -1; playernum = netnodes[node].player2)
		if (playernum != -1 && playeringame[playernum])
			SendKick(playernum, msg);
}
