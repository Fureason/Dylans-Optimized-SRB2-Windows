


#include "server_browser.h"
#include "i_net.h"
#include "../command.h"
#include "../console.h"
#include "../d_main.h"
#include "../doomstat.h"
#include "../i_time.h"
#include "../z_zone.h"
#include "../m_misc.h"
#ifdef _WIN32
#include <windows.h>
#endif

static sb_server_t *entries;
static size_t entrycount;
static boolean active, initialized;
static INT32 selected = -1;
static sb_addon_t *addons;
static size_t addoncount;
static boolean complete, failed;
static tic_t filequery, filestart, querytick;
static UINT8 fileattempts;
static UINT32 revision;
static size_t querycursor;
static INT32 lookup = -1;
static tic_t lookupnext;
static UINT8 lookupattempts;
static sb_player_t roster[MAXPLAYERS];
static size_t roster_count;
static boolean roster_open, roster_received;
static tic_t roster_start, roster_next, roster_seen;

static void SB_ResetAddons(void);

boolean SB_ValidAddress(const char *address)
{
	size_t length = address ? strlen(address) : 0;
	if (!length || length >= SB_ADDRESSLEN) return false;
	for (size_t i = 0; i < length; i++)
	{
		unsigned char c = (unsigned char)address[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
		|| (c >= '0' && c <= '9') || strchr(".:-_[]%", c))) return false;
	}
	return true;
}

static void SB_PlainText(char *to, size_t capacity, const char *from)
{
	size_t n = 0;
	while (*from && n + 1 < capacity)
	{
		unsigned char c = (unsigned char)*from++;
		if (c >= 32 && c < 127) to[n++] = (char)c;
	}
	to[n] = '\0';
}


static void SB_ColoredName(char *to, size_t capacity, const char *from)
{
	size_t n = 0;
	for (size_t i = 0; i + 1 < capacity && from[i]; i++)
	{
		unsigned char c = (unsigned char)from[i];
		if ((c >= 32 && c < 127) || (c >= 0x80 && c <= 0x8f)) to[n++] = (char)c;
	}
	to[n] = '\0';
}



static boolean SB_LookupAddress(const char *text, char *address)
{
	char host[SB_ADDRESSLEN];
	const char *first, *last, *port = NULL;
	size_t length;
	unsigned long portnum = 0;

	if (!SB_ValidAddress(text)) return false;
	first = strchr(text, ':');
	last = strrchr(text, ':');
	if (*text == '[')
	{
		const char *close = strchr(++text, ']');
		if (!close || !first || first >= close || (close[1] && close[1] != ':')) return false;
		length = (size_t)(close - text);
		if (close[1]) port = close + 2;
	}
	else if (first != last)
		length = strlen(text);
	else
	{
		length = first ? (size_t)(first - text) : strlen(text);
		if (first) port = first + 1;
	}
	memcpy(host, text, length);
	host[length] = '\0';
	if (port)
	{
		if (!*port || strlen(port) > 5 || port[strspn(port, "0123456789")]) return false;
		portnum = strtoul(port, NULL, 10);
		if (!portnum || portnum > UINT16_MAX) return false;
	}

	if (strchr(host, ':'))
	{
		if (host[strspn(host, "0123456789abcdefABCDEF:.%")]) return false;
		return snprintf(address, SB_ADDRESSLEN, port ? "[%s]:%lu" : "[%s]", host, portnum) < SB_ADDRESSLEN;
	}
	else
	{
		unsigned int octet[4];
		char extra;
		if (host[strspn(host, "0123456789.")] || strlen(host) > 15
			|| sscanf(host, "%3u.%3u.%3u.%3u%c", &octet[0], &octet[1], &octet[2], &octet[3], &extra) != 4)
			return false;
		for (size_t i = 0; i < 4; i++) if (octet[i] > UINT8_MAX) return false;
		snprintf(address, SB_ADDRESSLEN, port ? "%u.%u.%u.%u:%lu" : "%u.%u.%u.%u",
			octet[0], octet[1], octet[2], octet[3], portnum);
		return true;
	}
}

static INT32 SB_Find(const char *address)
{
	for (size_t i = 0; i < entrycount; i++)
		if (!strcasecmp(entries[i].address, address)) return (INT32)i;
	return -1;
}

static INT32 SB_Ensure(const char *address)
{
	INT32 index = SB_Find(address);
	if (index >= 0) return index;

	if (entrycount >= 4096 || !SB_ValidAddress(address)) return -1;
	entries = Z_Realloc(entries, (entrycount + 1) * sizeof(*entries), PU_STATIC, NULL);
	index = (INT32)entrycount++;
	revision++;
	memset(&entries[index], 0, sizeof(*entries));
	strlcpy(entries[index].address, address, SB_ADDRESSLEN);
	strlcpy(entries[index].info.servername, address, MAXSERVERNAME);
	strlcpy(entries[index].displayname, address, MAXSERVERNAME);
	return index;
}

boolean SB_Lookup(const char *text)
{
	char address[SB_ADDRESSLEN];
	const INT32 previous = lookup;
	lookup = -1;
	if (active && SB_LookupAddress(text, address))
		lookup = SB_Ensure(address);
	if (lookup >= 0)
	{
		lookupattempts = 0;
		lookupnext = I_GetTime();
		entries[lookup].checkuntil = lookupnext + 5*TICRATE;
	}
	if (previous != lookup) revision++;
	return lookup >= 0;
}

INT32 SB_LookupIndex(void) { return lookup; }

static boolean SB_Save(void)
{
	char filename[512], temporary[sizeof filename + 5];
	FILE *file;
	boolean ok;
	if (snprintf(filename, sizeof filename, "%s" PATHSEP "serverfavorites.txt", srb2home) >= (int)sizeof filename)
		return false;
	snprintf(temporary, sizeof temporary, "%s.tmp", filename);
	file = fopen(temporary, "w");
	if (!file)
	{
		CONS_Alert(CONS_WARNING, "Could not save server favorites.\n");
		return false;
	}
	fputs("# SRB2 server favorites: address<TAB>last known name\n", file);
	for (size_t i = 0; i < entrycount; i++)
		if (entries[i].favorite)
			fprintf(file, "%s\t%s\n", entries[i].address, SB_DisplayName(&entries[i]));
	ok = !ferror(file);
	if (fflush(file)) ok = false;
	if (fclose(file)) ok = false;
	if (ok)
	{
#ifdef _WIN32
		ok = MoveFileExA(temporary, filename, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
		ok = rename(temporary, filename) == 0;
#endif
	}
	if (!ok)
	{
		remove(temporary);
		CONS_Alert(CONS_WARNING, "Could not save server favorites; the previous file was preserved.\n");
	}
	return ok;
}

void SB_Init(void)
{
	char filename[512], line[256];
	FILE *file;
	if (initialized) return;
	initialized = true;
	snprintf(filename, sizeof filename, "%s" PATHSEP "serverfavorites.txt", srb2home);
	file = fopen(filename, "r");
	if (!file) return;
	while (fgets(line, sizeof line, file))
	{
		char *name, *end;
		INT32 index;
		if (line[0] == '#') continue;
		end = strpbrk(line, "\r\n");
		if (end) *end = '\0';
		name = strchr(line, '\t');
		if (name) *name++ = '\0';
		index = SB_Ensure(line);
		if (index < 0) continue;
		entries[index].favorite = true;
		if (name && *name)
		{
			SB_ColoredName(entries[index].displayname, sizeof entries[index].displayname, name);
			SB_PlainText(entries[index].info.servername, MAXSERVERNAME, entries[index].displayname);
		}
	}
	fclose(file);
}

size_t SB_Count(void) { return entrycount; }
UINT32 SB_Revision(void) { return revision; }
sb_server_t *SB_Server(size_t index) { return index < entrycount ? &entries[index] : NULL; }
boolean SB_Active(void) { return active; }
const char *SB_DisplayName(const sb_server_t *entry)
{
	return entry ? (entry->displayname[0] ? entry->displayname : entry->info.servername) : "";
}

boolean SB_Online(const sb_server_t *entry)
{
	return entry && entry->known && !entry->problem[0] && (tic_t)(I_GetTime() - entry->seen) < 20 * TICRATE;
}

const char *SB_Status(const sb_server_t *entry)
{
	if (!entry) return "Select a server";
	if (entry->known && entry->problem[0] && (tic_t)(I_GetTime()-entry->seen) < 20*TICRATE) return entry->problem;
	if (!SB_Online(entry))
		return (INT32)(entry->checkuntil - I_GetTime()) > 0 ? "Checking..." : "Unreachable";
	if (entry->info.maxplayer && entry->info.numberofplayer >= entry->info.maxplayer) return "Full";
	switch (entry->info.refusereason)
	{
		case REFUSE_SLOTS_FULL: return "Full";
		case REFUSE_JOINS_DISABLED: return "Joins disabled";
		case REFUSE_BANNED: return "Banned";
		default: return "Online";
	}
}

static INT32 SB_Node(const char *address)
{
	if (!I_GetNodeAddress) return -1;
	for (UINT32 i = 0; i < serverlistcount; i++)
	{
		const char *candidate = I_GetNodeAddress(serverlist[i].node);
		if (candidate && !strcasecmp(address, candidate)) return serverlist[i].node;
	}
	return -1;
}

static void SB_Query(size_t index)
{
	INT32 node = SB_Node(entries[index].address);
	boolean transient = node < 0;
	entries[index].queried = I_GetTime();
	if (transient) node = I_NetMakeNode(entries[index].address);
	if (node <= 0 || node >= MAXNETNODES) return;
	if (I_GetNodeAddress)
	{
		const char *resolved = I_GetNodeAddress(node);
		if (resolved && SB_ValidAddress(resolved) && strcasecmp(entries[index].address, resolved))
		{
			INT32 existing = SB_Find(resolved);
			boolean favorite = entries[index].favorite;
			if (existing >= 0 && existing != (INT32)index)
			{
				entries[existing].favorite |= favorite;
				entries[index].favorite = entries[index].listed = false;
				if (lookup == (INT32)index)
				{
					lookup = existing;
					entries[existing].queried = entries[index].queried;
					entries[existing].checkuntil = entries[index].checkuntil;
				}
			}
			else strlcpy(entries[index].address, resolved, SB_ADDRESSLEN);
			revision++;
			if (favorite) SB_Save();
		}
	}
	netbuffer->packettype = PT_ASKINFO;
	netbuffer->u.askinfo.version = VERSION;
	netbuffer->u.askinfo.time = (tic_t)LONG(I_GetTime());
	HSendPacket(node, false, 0, sizeof(askinfo_pak));
	if (transient) Net_CloseConnection(node | 0x8000);
}

void SB_Open(void)
{
	SB_Init();
	active = true;
	SB_Select(-1);
}

void SB_Close(void)
{
	active = false;
	SB_Lookup(NULL);
	SB_ShowPlayers(false);
	SB_Select(-1);
}

void SB_Refresh(void)
{
	revision++;
	const tic_t now = I_GetTime();
	for (size_t i = 0; i < entrycount; i++)
	{
		entries[i].listed = false;
		entries[i].queried = now - 10 * TICRATE;
		entries[i].checkuntil = now + 5 * TICRATE;
	}
	SB_ResetAddons();
	lookupattempts = 0;
	lookupnext = now;
	if (roster_open) SB_ShowPlayers(true);
}

boolean SB_Favorite(size_t index)
{
	boolean old;
	if (index >= entrycount) return false;
	old = entries[index].favorite;
	entries[index].favorite = !old;
	if (!SB_Save()) { entries[index].favorite = old; return false; }
	revision++;
	return true;
}

INT32 SB_AddFavorite(const char *address)
{
	INT32 index;
	char canonical[SB_ADDRESSLEN];
	SB_Init();
	strlcpy(canonical, address, sizeof canonical);
	if (active && netgame && SB_ValidAddress(address))
	{
		INT32 node = I_NetMakeNode(address);
		if (node > 0 && node < MAXNETNODES)
		{
			const char *resolved = I_GetNodeAddress ? I_GetNodeAddress(node) : NULL;
			boolean retained = node == servernode || netnodes[node].ingame;
			if (resolved) strlcpy(canonical, resolved, sizeof canonical);
			for (UINT32 i = 0; i < serverlistcount; i++) if (serverlist[i].node == node) retained = true;
			if (!retained) Net_CloseConnection(node | 0x8000);
		}
	}
	index = SB_Ensure(canonical);
	if (index >= 0 && !entries[index].favorite && !SB_Favorite((size_t)index)) return -1;
	if (index >= 0)
	{
		entries[index].queried = I_GetTime() - 10 * TICRATE;
		entries[index].checkuntil = I_GetTime() + 5 * TICRATE;
	}
	return index;
}



static boolean SB_ParseFiles(const UINT8 *data, size_t length, UINT8 count)
{
	const UINT8 *p = data, *end = data + length;
	sb_addon_t *page = count ? Z_Calloc(count * sizeof(*page), PU_STATIC, NULL) : NULL;
	for (UINT32 i = 0; i < count; i++)
	{
		const UINT8 *zero;
		size_t namelen;
		if ((size_t)(end - p) < 23) goto invalid;
		page[i].download = *p++ >> 4;
		page[i].folder = *p++ != 0;
		page[i].size = (UINT32)p[0] | ((UINT32)p[1] << 8) | ((UINT32)p[2] << 16) | ((UINT32)p[3] << 24);
		p += 4;
		zero = memchr(p, 0, (size_t)(end - p) - 16);
		if (!zero || (namelen = (size_t)(zero - p)) >= MAX_WADPATH) goto invalid;
		memcpy(page[i].name, p, namelen);
		SB_PlainText(page[i].name, sizeof page[i].name, page[i].name);
		p = zero + 1;
		memcpy(page[i].md5, p, 16); p += 16;
	}
	if (addoncount + count > UINT16_MAX) goto invalid;
	if (count)
	{
		addons = Z_Realloc(addons, (addoncount + count) * sizeof(*addons), PU_STATIC, NULL);
		memcpy(addons + addoncount, page, count * sizeof(*page));
		addoncount += count;
	}
	Z_Free(page);
	return true;
invalid:
	Z_Free(page);
	return false;
}

static void SB_ResetAddons(void)
{
	sb_server_t *entry;
	addoncount = 0; complete = failed = false; fileattempts = 0;
	filequery = I_GetTime() - TICRATE;
	filestart = I_GetTime();
	if (selected < 0 || (size_t)selected >= entrycount) return;
	entry = &entries[selected];
	if (!SB_Online(entry)) return;
	failed = !SB_ParseFiles(entry->info.fileneeded, entry->filebytes, entry->info.fileneedednum);
	complete = !failed && !(entry->info.flags & SV_LOTSOFADDONS);
}

void SB_Select(INT32 index)
{
	if (index == selected) return;
	selected = index;
	SB_ResetAddons();
	if (roster_open) SB_ShowPlayers(true);
}

void SB_Unavailable(SINT8 node, const char *reason)
{
 const char *address;
 INT32 index;
 if (!active || !I_GetNodeAddress || !(address = I_GetNodeAddress(node))) return;
 index = SB_Ensure(address);
 if (index < 0) return;
 strlcpy(entries[index].problem, reason, sizeof entries[index].problem);
 entries[index].known = true;
 entries[index].seen = I_GetTime();
 if (index != lookup) entries[index].listed = true;
 revision++;
}

void SB_Info(SINT8 node, const serverinfo_pak *info, size_t filebytes)
{
	const char *address;
	INT32 index;
	boolean changed, renamed;
	char name[MAXSERVERNAME];
	char colored[MAXSERVERNAME];
	if (!active || !I_GetNodeAddress) return;
	address = I_GetNodeAddress(node);
	if (!address || (index = SB_Ensure(address)) < 0) return;
	changed = entries[index].filebytes != filebytes
		|| entries[index].info.fileneedednum != info->fileneedednum
		|| (entries[index].info.flags & SV_LOTSOFADDONS) != (info->flags & SV_LOTSOFADDONS)
		|| memcmp(entries[index].info.fileneeded, info->fileneeded, filebytes);
	SB_ColoredName(colored, sizeof colored, info->servername);
	SB_PlainText(name, sizeof name, colored);
	renamed = strcmp(entries[index].info.servername, name) != 0
		|| strcmp(entries[index].displayname, colored) != 0;
	entries[index].info = *info;
	strlcpy(entries[index].displayname, colored, MAXSERVERNAME);
	strlcpy(entries[index].info.servername, name, MAXSERVERNAME);
	SB_PlainText(entries[index].info.gametypename, sizeof info->gametypename, info->gametypename);
	SB_PlainText(entries[index].info.maptitle, sizeof info->maptitle, info->maptitle);
	entries[index].filebytes = filebytes;
	entries[index].known = true;
	entries[index].problem[0] = 0;
	if (index != lookup) entries[index].listed = true;
	entries[index].seen = I_GetTime();
	revision++;
	if (selected == index && (changed || (!complete && !addoncount)))
		SB_ResetAddons();
	if (renamed && entries[index].favorite) SB_Save();
}

void SB_ShowPlayers(boolean show)
{
	roster_open = show;
	roster_count = 0;
	roster_received = false;
	roster_start = roster_next = I_GetTime();
}

void SB_PlayerInfo(SINT8 node)
{
	const char *address;
	if (!active || !roster_open || selected < 0 || !I_GetNodeAddress
		|| doomcom->datalength != BASEPACKETSIZE + sizeof netbuffer->u.playerinfo) return;
	address = I_GetNodeAddress(node);
	if (!address || strcasecmp(address, entries[selected].address)) return;

	roster_count = 0;
	for (size_t i = 0; i < MAXPLAYERS; i++)
	{
		const plrinfo_pak *player = &netbuffer->u.playerinfo[i];
		sb_player_t *entry = &roster[roster_count];
		if (player->num != i) continue;
		SB_ColoredName(entry->name, sizeof entry->name, player->name);
		if (!entry->name[0]) continue;
		entry->spectator = player->team == UINT8_MAX;
		roster_count++;
	}
	roster_received = true;
	roster_seen = I_GetTime();
	roster_next = roster_seen + 5*TICRATE;
}

size_t SB_PlayerCount(void)
{
	return roster_received && I_GetTime() - roster_seen < 15*TICRATE ? roster_count : 0;
}

const sb_player_t *SB_Player(size_t index)
{
	return index < roster_count ? &roster[index] : NULL;
}

const char *SB_PlayerStatus(void)
{
	const tic_t now = I_GetTime();
	if (roster_received && now - roster_seen < 15*TICRATE)
		return roster_count ? "Live player list" : "No players reported.";
	if (now - roster_start < 5*TICRATE) return "Waiting for the server's player list...";
	return "Player list unavailable.";
}

boolean SB_MoreFiles(SINT8 node)
{
	INT32 first;
	size_t header = BASEPACKETSIZE + offsetof(filesneededconfig_pak, files);
	const char *address;
	if (!active) return false;

	if (selected < 0 || (size_t)selected >= entrycount || !I_GetNodeAddress) return true;
	address = I_GetNodeAddress(node);
	if (!address || strcasecmp(address, entries[selected].address) || complete || failed) return true;
	if ((size_t)doomcom->datalength < header) { failed = true; return true; }
	first = netbuffer->u.filesneededcfg.first;
	if (first < 0 || (size_t)first != addoncount) return true;
	if (!netbuffer->u.filesneededcfg.num && netbuffer->u.filesneededcfg.more) { failed = true; return true; }
	failed = !SB_ParseFiles(netbuffer->u.filesneededcfg.files,
		min((size_t)doomcom->datalength - header, (size_t)MAXFILENEEDED), netbuffer->u.filesneededcfg.num);
	complete = !failed && !netbuffer->u.filesneededcfg.more;
	fileattempts = 0;
	filequery = I_GetTime() - TICRATE;
	return true;
}

void SB_Ticker(void)
{
	tic_t now = I_GetTime();
	if (!active || Playing() || !netgame || !I_NetMakeNodewPort) return;

	if (querytick != now)
	{
		querytick = now;
		if (lookup >= 0 && !SB_Online(&entries[lookup]) && lookupattempts < 3
			&& (INT32)(now - lookupnext) >= 0)
		{
			lookupattempts++;
			lookupnext = now + TICRATE;
			SB_Query((size_t)lookup);
		}
		else if (roster_open && selected >= 0 && (INT32)(now - roster_next) >= 0)
		{
			roster_next = now + (now - roster_start < 5*TICRATE ? TICRATE : 5*TICRATE);
			SB_Query((size_t)selected);
		}
		else for (size_t scanned = 0; scanned < entrycount; scanned++)
		{
			size_t i = (querycursor + scanned) % entrycount;
			if ((entries[i].listed || entries[i].favorite || ((INT32)i == lookup && SB_Online(&entries[i])))
			&& (tic_t)(now - entries[i].queried) >= 10 * TICRATE)
			{
				SB_Query(i);
				querycursor = (i + 1) % entrycount;
				break;
			}
		}
	}
	if (selected >= 0 && (size_t)selected < entrycount && !complete && !failed
	&& SB_Online(&entries[selected]) && (tic_t)(now - filequery) >= max(1, TICRATE/4))
	{
		INT32 node = SB_Node(entries[selected].address);
		if (fileattempts >= 20 || (tic_t)(now - filestart) > 30 * TICRATE) { failed = true; return; }
		boolean transient = node < 0;
		if (transient) node = I_NetMakeNode(entries[selected].address);
		filequery = now;
		fileattempts++;
		if (node <= 0 || node >= MAXNETNODES) return;
		netbuffer->packettype = PT_TELLFILESNEEDED;
		netbuffer->u.filesneedednum = (INT32)addoncount;
		HSendPacket(node, false, 0, sizeof(INT32));
		if (transient) Net_CloseConnection(node | 0x8000);
	}
}

size_t SB_AddonCount(void) { return addoncount; }
const sb_addon_t *SB_Addon(size_t index) { return index < addoncount ? &addons[index] : NULL; }
boolean SB_AddonsComplete(void) { return complete; }
boolean SB_AddonsFailed(void) { return failed; }
boolean SB_NeedsCleanSession(void)
{
	size_t i = 0;
	if (!complete) return false;
	for (UINT16 j = mainwads; j < numwadfiles; j++)
	{
		if (!wadfiles[j]->important) continue;
		if (i >= addoncount || memcmp(wadfiles[j]->md5sum, addons[i].md5, 16)) return true;
		i++;
	}
	return false;
}
