// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Discord Social SDK rich presence for the optimized Windows build.
//
// The SDK is loaded dynamically so the game remains usable when Discord is
// closed or discord_partner_sdk.dll is missing. Presence updates are local UI
// only: no SRB2 packets, sync state, physics, demos, or file hashes are changed.
//-----------------------------------------------------------------------------

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"

#include "command.h"
#include "console.h"
#include "d_main.h"
#include "discord_presence.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "g_state.h"
#include "i_system.h"
#include "i_time.h"
#include "m_argv.h"
#include "p_tick.h"
#include "r_skins.h"
#include "w_wad.h"
#include "netcode/d_clisrv.h"
#include "netcode/i_tcp.h"
#include "netcode/mserv.h"
#include "netcode/server_connection.h"

#define DRPC_APPLICATION_ID UINT64_C(1546030063236153344)
#define DRPC_JOIN_PREFIX "srb2-2215|"
#define DRPC_UPDATE_INTERVAL (2*TICRATE)
#define DRPC_ASSET_INDEX "discord-assets.tsv"
#define DRPC_MAX_ASSET_ENTRIES 4096
#define DRPC_ASSET_ID_SIZE 17
#define DRPC_ASSET_HASH_SIZE 65
#define DRPC_EXTERNAL_IMAGE_SIZE 300

consvar_t cv_discordpresence = CVAR_INIT(
	"discordpresence", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_discordjoinaddress = CVAR_INIT(
	"discordjoinaddress", "", CV_SAVE, NULL, NULL);

#ifdef _WIN32

/* Minimal ABI declarations from Discord Social SDK 1.10.19337. Keeping the
 * bridge on the C ABI lets the existing C engine load the DLL without adding
 * a C++ runtime or link-time dependency. */
typedef struct { void *opaque; } Discord_Client;
typedef struct { void *opaque; } Discord_ClientResult;
typedef struct { void *opaque; } Discord_Activity;
typedef struct { void *opaque; } Discord_ActivityAssets;
typedef struct { void *opaque; } Discord_ActivityTimestamps;
typedef struct { void *opaque; } Discord_ActivityParty;
typedef struct { void *opaque; } Discord_ActivitySecrets;
typedef struct { const uint8_t *ptr; size_t size; } Discord_String;

typedef enum
{
	Discord_ActivityTypes_Playing = 0
} Discord_ActivityTypes;

typedef enum
{
	Discord_ActivityPartyPrivacy_Private = 0,
	Discord_ActivityPartyPrivacy_Public = 1
} Discord_ActivityPartyPrivacy;

typedef enum
{
	Discord_ActivityGamePlatforms_Desktop = 1
} Discord_ActivityGamePlatforms;

typedef void (*Discord_Client_ActivityJoinCallback)(Discord_String, void *);
typedef void (*Discord_Client_UpdateRichPresenceCallback)(Discord_ClientResult *, void *);
typedef void (*Discord_FreeFn)(void *);

static void *drpc_library;
static Discord_Client drpc_client;
static boolean drpc_initialized;
static boolean drpc_presence_visible;
static tic_t drpc_next_update;
static tic_t drpc_previous_leveltime;
static INT16 drpc_previous_map = -1;
static time_t drpc_level_started;
static char drpc_previous_signature[1024];

typedef struct
{
	char kind;
	char md5[33];
	char id[DRPC_ASSET_ID_SIZE];
	char hash[DRPC_ASSET_HASH_SIZE];
} drpc_asset_entry_t;

static drpc_asset_entry_t *drpc_asset_entries;
static size_t drpc_asset_entry_count;
static char drpc_asset_base_url[256];

static void (*pDiscord_RunCallbacks)(void);
static void (*pDiscord_Client_Init)(Discord_Client *);
static void (*pDiscord_Client_Drop)(Discord_Client *);
static void (*pDiscord_Client_SetApplicationId)(Discord_Client *, uint64_t);
static boolean (*pDiscord_Client_RegisterLaunchCommand)(Discord_Client *, uint64_t, Discord_String);
static void (*pDiscord_Client_SetActivityJoinCallback)(Discord_Client *, Discord_Client_ActivityJoinCallback, Discord_FreeFn, void *);
static void (*pDiscord_Client_ClearRichPresence)(Discord_Client *);
static void (*pDiscord_Client_UpdateRichPresence)(Discord_Client *, Discord_Activity *, Discord_Client_UpdateRichPresenceCallback, Discord_FreeFn, void *);
static void (*pDiscord_ClientResult_Drop)(Discord_ClientResult *);

static void (*pDiscord_Activity_Init)(Discord_Activity *);
static void (*pDiscord_Activity_Drop)(Discord_Activity *);
static void (*pDiscord_Activity_SetName)(Discord_Activity *, Discord_String);
static void (*pDiscord_Activity_SetType)(Discord_Activity *, Discord_ActivityTypes);
static void (*pDiscord_Activity_SetState)(Discord_Activity *, Discord_String *);
static void (*pDiscord_Activity_SetDetails)(Discord_Activity *, Discord_String *);
static void (*pDiscord_Activity_SetAssets)(Discord_Activity *, Discord_ActivityAssets *);
static void (*pDiscord_Activity_SetTimestamps)(Discord_Activity *, Discord_ActivityTimestamps *);
static void (*pDiscord_Activity_SetParty)(Discord_Activity *, Discord_ActivityParty *);
static void (*pDiscord_Activity_SetSecrets)(Discord_Activity *, Discord_ActivitySecrets *);
static void (*pDiscord_Activity_SetSupportedPlatforms)(Discord_Activity *, Discord_ActivityGamePlatforms);

static void (*pDiscord_ActivityAssets_Init)(Discord_ActivityAssets *);
static void (*pDiscord_ActivityAssets_Drop)(Discord_ActivityAssets *);
static void (*pDiscord_ActivityAssets_SetLargeImage)(Discord_ActivityAssets *, Discord_String);
static void (*pDiscord_ActivityAssets_SetLargeText)(Discord_ActivityAssets *, Discord_String);
static void (*pDiscord_ActivityAssets_SetSmallImage)(Discord_ActivityAssets *, Discord_String);
static void (*pDiscord_ActivityAssets_SetSmallText)(Discord_ActivityAssets *, Discord_String);

static void (*pDiscord_ActivityTimestamps_Init)(Discord_ActivityTimestamps *);
static void (*pDiscord_ActivityTimestamps_Drop)(Discord_ActivityTimestamps *);
static void (*pDiscord_ActivityTimestamps_SetStart)(Discord_ActivityTimestamps *, uint64_t);

static void (*pDiscord_ActivityParty_Init)(Discord_ActivityParty *);
static void (*pDiscord_ActivityParty_Drop)(Discord_ActivityParty *);
static void (*pDiscord_ActivityParty_SetId)(Discord_ActivityParty *, Discord_String);
static void (*pDiscord_ActivityParty_SetCurrentSize)(Discord_ActivityParty *, int32_t);
static void (*pDiscord_ActivityParty_SetMaxSize)(Discord_ActivityParty *, int32_t);
static void (*pDiscord_ActivityParty_SetPrivacy)(Discord_ActivityParty *, Discord_ActivityPartyPrivacy);

static void (*pDiscord_ActivitySecrets_Init)(Discord_ActivitySecrets *);
static void (*pDiscord_ActivitySecrets_Drop)(Discord_ActivitySecrets *);
static void (*pDiscord_ActivitySecrets_SetJoin)(Discord_ActivitySecrets *, Discord_String);

static Discord_String DRPC_String(const char *text)
{
	Discord_String value;
	value.ptr = (const uint8_t *)(text ? text : "");
	value.size = text ? strlen(text) : 0;
	return value;
}

#define DRPC_LOAD(name) do { \
	*(void **)(&p##name) = SDL_LoadFunction(drpc_library, #name); \
	if (!p##name) goto load_failed; \
} while (0)

static boolean DRPC_LoadLibrary(void)
{
	drpc_library = SDL_LoadObject("discord_partner_sdk.dll");
	if (!drpc_library)
		return false;

	DRPC_LOAD(Discord_RunCallbacks);
	DRPC_LOAD(Discord_Client_Init);
	DRPC_LOAD(Discord_Client_Drop);
	DRPC_LOAD(Discord_Client_SetApplicationId);
	DRPC_LOAD(Discord_Client_RegisterLaunchCommand);
	DRPC_LOAD(Discord_Client_SetActivityJoinCallback);
	DRPC_LOAD(Discord_Client_ClearRichPresence);
	DRPC_LOAD(Discord_Client_UpdateRichPresence);
	DRPC_LOAD(Discord_ClientResult_Drop);
	DRPC_LOAD(Discord_Activity_Init);
	DRPC_LOAD(Discord_Activity_Drop);
	DRPC_LOAD(Discord_Activity_SetName);
	DRPC_LOAD(Discord_Activity_SetType);
	DRPC_LOAD(Discord_Activity_SetState);
	DRPC_LOAD(Discord_Activity_SetDetails);
	DRPC_LOAD(Discord_Activity_SetAssets);
	DRPC_LOAD(Discord_Activity_SetTimestamps);
	DRPC_LOAD(Discord_Activity_SetParty);
	DRPC_LOAD(Discord_Activity_SetSecrets);
	DRPC_LOAD(Discord_Activity_SetSupportedPlatforms);
	DRPC_LOAD(Discord_ActivityAssets_Init);
	DRPC_LOAD(Discord_ActivityAssets_Drop);
	DRPC_LOAD(Discord_ActivityAssets_SetLargeImage);
	DRPC_LOAD(Discord_ActivityAssets_SetLargeText);
	DRPC_LOAD(Discord_ActivityAssets_SetSmallImage);
	DRPC_LOAD(Discord_ActivityAssets_SetSmallText);
	DRPC_LOAD(Discord_ActivityTimestamps_Init);
	DRPC_LOAD(Discord_ActivityTimestamps_Drop);
	DRPC_LOAD(Discord_ActivityTimestamps_SetStart);
	DRPC_LOAD(Discord_ActivityParty_Init);
	DRPC_LOAD(Discord_ActivityParty_Drop);
	DRPC_LOAD(Discord_ActivityParty_SetId);
	DRPC_LOAD(Discord_ActivityParty_SetCurrentSize);
	DRPC_LOAD(Discord_ActivityParty_SetMaxSize);
	DRPC_LOAD(Discord_ActivityParty_SetPrivacy);
	DRPC_LOAD(Discord_ActivitySecrets_Init);
	DRPC_LOAD(Discord_ActivitySecrets_Drop);
	DRPC_LOAD(Discord_ActivitySecrets_SetJoin);
	return true;

load_failed:
	CONS_Alert(CONS_WARNING, "Discord Social SDK is missing a required function: %s\n", SDL_GetError());
	SDL_UnloadObject(drpc_library);
	drpc_library = NULL;
	return false;
}

#undef DRPC_LOAD

static boolean DRPC_ValidAddress(const char *address)
{
	const unsigned char *p = (const unsigned char *)address;
	size_t length = address ? strlen(address) : 0;

	if (!length || length > 100)
		return false;

	for (; *p; p++)
	{
		if (!(isalnum(*p) || *p == '.' || *p == '-' || *p == ':' || *p == '[' || *p == ']'))
			return false;
	}

	return true;
}

static boolean DRPC_HasPort(const char *address)
{
	const char *firstcolon = strchr(address, ':');
	const char *lastcolon = strrchr(address, ':');

	if (!lastcolon)
		return false;

	if (address[0] == '[')
	{
		const char *close = strchr(address, ']');
		return close && close[1] == ':' && close[2] != '\0';
	}

	/* A single colon is a hostname/IPv4 port separator. Unbracketed IPv6 is
	 * deliberately rejected by DRPC_BuildJoinAddress to avoid ambiguity. */
	return firstcolon == lastcolon && lastcolon[1] != '\0';
}

static boolean DRPC_BuildJoinAddress(char *output, size_t output_size)
{
	const char *configured = cv_discordjoinaddress.string;
	const char *firstcolon;
	const char *lastcolon;

	if (!configured || !DRPC_ValidAddress(configured))
		return false;

	firstcolon = strchr(configured, ':');
	lastcolon = strrchr(configured, ':');
	if (firstcolon && firstcolon != lastcolon && configured[0] != '[')
		return false;

	if (DRPC_HasPort(configured))
		snprintf(output, output_size, "%s", configured);
	else if (current_port)
		snprintf(output, output_size, "%s:%u", configured, (unsigned)current_port);
	else
		return false;

	return strlen(output) < output_size;
}

static void DRPC_OnJoin(Discord_String secret, void *userdata)
{
	char address[128];
	size_t prefix_length = strlen(DRPC_JOIN_PREFIX);
	size_t address_length;
	(void)userdata;

	if (!secret.ptr || secret.size <= prefix_length
		|| memcmp(secret.ptr, DRPC_JOIN_PREFIX, prefix_length) != 0)
		return;

	address_length = secret.size - prefix_length;
	if (address_length >= sizeof address)
		return;

	memcpy(address, secret.ptr + prefix_length, address_length);
	address[address_length] = '\0';
	if (!DRPC_ValidAddress(address))
		return;

	CONS_Printf("Discord invite accepted; joining %s\n", address);
	COM_BufAddText(va("connect \"%s\"\n", address));
}

static void DRPC_OnPresenceUpdated(Discord_ClientResult *result, void *userdata)
{
	(void)userdata;
	if (result)
		pDiscord_ClientResult_Drop(result);
}

static boolean DRPC_IsHex(const char *text, size_t length)
{
	size_t i;

	if (!text || strlen(text) != length)
		return false;

	for (i = 0; i < length; i++)
		if (!isxdigit((unsigned char)text[i]))
			return false;

	return true;
}

static void DRPC_ClearAssetIndex(void)
{
	free(drpc_asset_entries);
	drpc_asset_entries = NULL;
	drpc_asset_entry_count = 0;
	drpc_asset_base_url[0] = '\0';
}

static FILE *DRPC_OpenAssetIndex(void)
{
	FILE *stream;
	char path[MAX_WADPATH];

	snprintf(path, sizeof path, "%s%s%s", srb2path, PATHSEP, DRPC_ASSET_INDEX);
	stream = fopen(path, "rt");
	if (stream)
		return stream;

	if (strcmp(srb2home, srb2path))
	{
		snprintf(path, sizeof path, "%s%s%s", srb2home, PATHSEP, DRPC_ASSET_INDEX);
		stream = fopen(path, "rt");
	}

	return stream;
}

static void DRPC_LoadAssetIndex(void)
{
	FILE *stream;
	char line[512];

	DRPC_ClearAssetIndex();
	stream = DRPC_OpenAssetIndex();
	if (!stream)
		return;

	while (fgets(line, sizeof line, stream))
	{
		char *kind = strtok(line, "\t\r\n");
		char *md5 = strtok(NULL, "\t\r\n");
		char *id = strtok(NULL, "\t\r\n");
		char *hash = strtok(NULL, "\t\r\n");
		drpc_asset_entry_t *grown;
		drpc_asset_entry_t *entry;

		if (!kind || kind[0] == '#')
			continue;

		if (!stricmp(kind, "base"))
		{
			if (md5 && !strncmp(md5, "https://", 8)
				&& strlen(md5) < sizeof drpc_asset_base_url)
			{
				snprintf(drpc_asset_base_url, sizeof drpc_asset_base_url, "%s", md5);
				while (drpc_asset_base_url[0]
					&& drpc_asset_base_url[strlen(drpc_asset_base_url)-1] == '/')
					drpc_asset_base_url[strlen(drpc_asset_base_url)-1] = '\0';
			}
			continue;
		}

		if ((!stricmp(kind, "skin") || !stricmp(kind, "map"))
			&& DRPC_IsHex(md5, 32) && id && id[0]
			&& strlen(id) < DRPC_ASSET_ID_SIZE && DRPC_IsHex(hash, 64)
			&& drpc_asset_entry_count < DRPC_MAX_ASSET_ENTRIES)
		{
			grown = realloc(drpc_asset_entries,
				(drpc_asset_entry_count + 1) * sizeof(*drpc_asset_entries));
			if (!grown)
				break;
			drpc_asset_entries = grown;
			entry = &drpc_asset_entries[drpc_asset_entry_count++];
			entry->kind = (char)(tolower((unsigned char)kind[0]));
			snprintf(entry->md5, sizeof entry->md5, "%s", md5);
			strlwr(entry->md5);
			snprintf(entry->id, sizeof entry->id, "%s", id);
			strlwr(entry->id);
			snprintf(entry->hash, sizeof entry->hash, "%s", hash);
			strlwr(entry->hash);
		}
	}

	fclose(stream);
	if (drpc_asset_base_url[0] && drpc_asset_entry_count)
		CONS_Printf("Discord custom artwork index loaded (%u entries).\n",
			(unsigned)drpc_asset_entry_count);
	else
		DRPC_ClearAssetIndex();
}

static void DRPC_MD5Text(UINT16 wadnum, char output[33])
{
	static const char hex[] = "0123456789abcdef";
	size_t i;

	for (i = 0; i < 16; i++)
	{
		output[i*2] = hex[wadfiles[wadnum]->md5sum[i] >> 4];
		output[i*2+1] = hex[wadfiles[wadnum]->md5sum[i] & 15];
	}
	output[32] = '\0';
}

static boolean DRPC_ResolveAsset(char kind, const char *id, UINT16 wadnum,
	char *output, size_t output_size)
{
	char md5[33];
	size_t i;
	int written;

	if (!drpc_asset_base_url[0] || !id || !id[0]
		|| wadnum >= numwadfiles || !wadfiles[wadnum])
		return false;

	DRPC_MD5Text(wadnum, md5);
	for (i = 0; i < drpc_asset_entry_count; i++)
	{
		const drpc_asset_entry_t *entry = &drpc_asset_entries[i];
		if (entry->kind != kind || stricmp(entry->md5, md5)
			|| stricmp(entry->id, id))
			continue;

		written = snprintf(output, output_size, "%s/%s.png",
			drpc_asset_base_url, entry->hash);
		return written > 0 && (size_t)written < output_size
			&& written < DRPC_EXTERNAL_IMAGE_SIZE;
	}

	return false;
}

/* Add-ons are often rebuilt without changing their map preview.  The archive
 * MD5 then changes even though the content-addressed artwork is still valid.
 * Fall back only when every index entry for this map code points at the same
 * image; common slots such as MAP01 deliberately remain ambiguous. */
static boolean DRPC_ResolveUniqueMapAsset(const char *id, char *output,
	size_t output_size)
{
	const drpc_asset_entry_t *match = NULL;
	size_t i;
	int written;

	if (!drpc_asset_base_url[0] || !id || !id[0])
		return false;

	for (i = 0; i < drpc_asset_entry_count; i++)
	{
		const drpc_asset_entry_t *entry = &drpc_asset_entries[i];
		if (entry->kind != 'm' || stricmp(entry->id, id))
			continue;
		if (match && stricmp(match->hash, entry->hash))
			return false;
		match = entry;
	}

	if (!match)
		return false;
	written = snprintf(output, output_size, "%s/%s.png",
		drpc_asset_base_url, match->hash);
	return written > 0 && (size_t)written < output_size
		&& written < DRPC_EXTERNAL_IMAGE_SIZE;
}

static boolean DRPC_ResolveAssetHash(char kind, const char *id, UINT16 wadnum,
	char output[DRPC_ASSET_HASH_SIZE])
{
	char md5[33];
	size_t i;

	if (!drpc_asset_base_url[0] || !id || !id[0]
		|| wadnum >= numwadfiles || !wadfiles[wadnum])
		return false;

	DRPC_MD5Text(wadnum, md5);
	for (i = 0; i < drpc_asset_entry_count; i++)
	{
		const drpc_asset_entry_t *entry = &drpc_asset_entries[i];
		if (entry->kind == kind && !stricmp(entry->md5, md5)
			&& !stricmp(entry->id, id))
		{
			snprintf(output, DRPC_ASSET_HASH_SIZE, "%s", entry->hash);
			return true;
		}
	}

	return false;
}

static void DRPC_FallbackAsset(char *output, size_t output_size)
{
	int written;

	if (drpc_asset_base_url[0])
	{
		written = snprintf(output, output_size, "%s/srb2.png",
			drpc_asset_base_url);
		if (written > 0 && (size_t)written < output_size
			&& written < DRPC_EXTERNAL_IMAGE_SIZE)
			return;
	}

	/* This key remains a last-resort fallback for installs without the local
	 * artwork index. */
	snprintf(output, output_size, "srb2");
}

static void DRPC_MapAssetKey(char *output, size_t output_size, const char *map_code)
{
	char preview_name[8];
	lumpnum_t preview_lump;

	snprintf(preview_name, sizeof preview_name, "%sP", map_code);
	preview_lump = W_CheckNumForName(preview_name);
	if (preview_lump != LUMPERROR
		&& DRPC_ResolveAsset('m', map_code, WADFILENUM(preview_lump),
			output, output_size))
		return;
	if (preview_lump != LUMPERROR
		&& DRPC_ResolveUniqueMapAsset(map_code, output, output_size))
		return;

	DRPC_FallbackAsset(output, output_size);
}

static boolean DRPC_SkinAssetKey(char *output, size_t output_size,
	INT32 skin, UINT16 color)
{
	static const char hex[] = "0123456789abcdef";
	char source_hash[DRPC_ASSET_HASH_SIZE];
	char ramp[COLORRAMPSIZE * 2 + 1];
	INT32 i;
	int written;

	if (skin < 0 || skin >= numskins || !skins[skin]
		|| color >= numskincolors
		|| !DRPC_ResolveAssetHash('s', skins[skin]->name,
			skins[skin]->wadnum, source_hash))
		return false;

	for (i = 0; i < COLORRAMPSIZE; i++)
	{
		UINT8 value = skincolors[color].ramp[i];
		ramp[i*2] = hex[value >> 4];
		ramp[i*2+1] = hex[value & 15];
	}
	ramp[COLORRAMPSIZE * 2] = '\0';

	written = snprintf(output, output_size, "%s/skin/%s/%02x/%s.png",
		drpc_asset_base_url, source_hash, skins[skin]->starttranscolor, ramp);
	return written > 0 && (size_t)written < output_size
		&& written < DRPC_EXTERNAL_IMAGE_SIZE;
}

static void DRPC_SetPresence(void)
{
	Discord_Activity activity;
	Discord_ActivityAssets assets;
	Discord_ActivityTimestamps timestamps;
	Discord_ActivityParty party;
	Discord_ActivitySecrets secrets;
	Discord_String text;
	char details[128];
	char state[128];
	char large_key[DRPC_EXTERNAL_IMAGE_SIZE];
	char small_key[DRPC_EXTERNAL_IMAGE_SIZE];
	char character_name[SKINNAMESIZE+7];
	char map_name[64];
	char join_address[128];
	char join_secret[160];
	char party_id[32];
	char signature[1024];
	const char *gametype_name = "Playing";
	INT32 player_count = 1;
	INT32 maximum_players = 1;
	INT32 rings = 0;
	boolean in_level = (gamestate == GS_LEVEL && gamemap > 0 && gamemap <= NUMMAPS);
	boolean joinable = false;
	boolean have_small_asset = false;
	boolean have_assets = false;
	INT32 player_skin = -1;
	UINT16 player_color = 0;

	large_key[0] = '\0';
	small_key[0] = '\0';
	character_name[0] = '\0';

	if (gametype >= 0 && gametype < gametypecount
		&& Gametype_Names[gametype] && Gametype_Names[gametype][0])
		gametype_name = Gametype_Names[gametype];

	if (in_level)
	{
		const char *map_code = G_BuildMapName(gamemap);
		const char *title = mapheaderinfo[gamemap-1] ? mapheaderinfo[gamemap-1]->lvlttl : NULL;

		snprintf(map_name, sizeof map_name, "%s%s%s", map_code,
			title && title[0] ? " - " : "", title && title[0] ? title : "");
		DRPC_MapAssetKey(large_key, sizeof large_key, map_code);

		if (consoleplayer >= 0 && consoleplayer < MAXPLAYERS && playeringame[consoleplayer])
		{
			rings = players[consoleplayer].rings < 0 ? 0 : players[consoleplayer].rings;
			player_skin = players[consoleplayer].skin;
			player_color = players[consoleplayer].mo
				? players[consoleplayer].mo->color : players[consoleplayer].skincolor;
			if (player_skin >= 0 && player_skin < numskins && skins[player_skin])
			{
				snprintf(character_name, sizeof character_name, "%s",
					skins[player_skin]->realname[0]
						? skins[player_skin]->realname : skins[player_skin]->name);
				have_small_asset = DRPC_SkinAssetKey(small_key, sizeof small_key,
					player_skin, player_color);
			}
		}

		if (netgame)
		{
			player_count = D_NumPlayers();
			maximum_players = cv_maxplayers.value;
			snprintf(details, sizeof details, "%s",
				cv_servername.string && cv_servername.string[0]
					? cv_servername.string : "SRB2 Server");
			snprintf(state, sizeof state, "%s | %s | %d Rings", map_name,
				gametype_name, rings);
		}
		else
		{
			snprintf(state, sizeof state, "%s - Single Player%s%s", gametype_name,
				character_name[0] ? " | " : "", character_name);
			snprintf(details, sizeof details, "%s | %d Rings", map_name, rings);
		}

		if (drpc_previous_map != gamemap || leveltime + TICRATE < drpc_previous_leveltime || !drpc_level_started)
			drpc_level_started = time(NULL) - (time_t)(leveltime / TICRATE);
		drpc_previous_map = gamemap;
		drpc_previous_leveltime = leveltime;

		joinable = server && serverrunning && netgame && cv_allownewplayer.value
			&& player_count < maximum_players
			&& DRPC_BuildJoinAddress(join_address, sizeof join_address);
	}
	else
	{
		snprintf(details, sizeof details, "%s", gamestate == GS_TITLESCREEN ? "Main Menu" : "Starting SRB2");
		state[0] = '\0';
		map_name[0] = '\0';
		drpc_previous_map = -1;
		drpc_previous_leveltime = 0;
		drpc_level_started = 0;
	}

	snprintf(signature, sizeof signature, "%d|%d|%s|%s|%s|%s|%d|%d|%s",
		(int)gamestate, (int)gamemap, details, state, large_key,
		small_key, player_count, maximum_players, joinable ? join_address : "");
	if (!strcmp(signature, drpc_previous_signature))
		return;
	snprintf(drpc_previous_signature, sizeof drpc_previous_signature, "%s", signature);

	pDiscord_Activity_Init(&activity);
	text = DRPC_String("Dylan's Optimized SRB2 Build");
	pDiscord_Activity_SetName(&activity, text);
	pDiscord_Activity_SetType(&activity, Discord_ActivityTypes_Playing);
	text = DRPC_String(details);
	pDiscord_Activity_SetDetails(&activity, &text);
	if (state[0])
	{
		text = DRPC_String(state);
		pDiscord_Activity_SetState(&activity, &text);
	}

	have_assets = (large_key[0] != '\0') || have_small_asset;
	if (have_assets)
	{
		pDiscord_ActivityAssets_Init(&assets);
		if (large_key[0])
		{
			text = DRPC_String(large_key);
			pDiscord_ActivityAssets_SetLargeImage(&assets, text);
			if (map_name[0])
			{
				text = DRPC_String(map_name);
				pDiscord_ActivityAssets_SetLargeText(&assets, text);
			}
		}
		if (have_small_asset)
		{
			text = DRPC_String(small_key);
			pDiscord_ActivityAssets_SetSmallImage(&assets, text);
			text = DRPC_String(character_name);
			pDiscord_ActivityAssets_SetSmallText(&assets, text);
		}
		pDiscord_Activity_SetAssets(&activity, &assets);
		pDiscord_ActivityAssets_Drop(&assets);
	}

	if (in_level && drpc_level_started)
	{
		pDiscord_ActivityTimestamps_Init(&timestamps);
		pDiscord_ActivityTimestamps_SetStart(&timestamps, (uint64_t)drpc_level_started);
		pDiscord_Activity_SetTimestamps(&activity, &timestamps);
		pDiscord_ActivityTimestamps_Drop(&timestamps);
	}

	if (netgame && in_level)
	{
		snprintf(party_id, sizeof party_id, "srb2-%.8s", server_context);
		pDiscord_ActivityParty_Init(&party);
		text = DRPC_String(party_id);
		pDiscord_ActivityParty_SetId(&party, text);
		pDiscord_ActivityParty_SetCurrentSize(&party, player_count);
		pDiscord_ActivityParty_SetMaxSize(&party, maximum_players);
		pDiscord_ActivityParty_SetPrivacy(&party, joinable
			? Discord_ActivityPartyPrivacy_Public : Discord_ActivityPartyPrivacy_Private);
		pDiscord_Activity_SetParty(&activity, &party);
		pDiscord_ActivityParty_Drop(&party);
	}

	if (joinable)
	{
		snprintf(join_secret, sizeof join_secret, "%s%s", DRPC_JOIN_PREFIX, join_address);
		pDiscord_ActivitySecrets_Init(&secrets);
		text = DRPC_String(join_secret);
		pDiscord_ActivitySecrets_SetJoin(&secrets, text);
		pDiscord_Activity_SetSecrets(&activity, &secrets);
		pDiscord_ActivitySecrets_Drop(&secrets);
		pDiscord_Activity_SetSupportedPlatforms(&activity, Discord_ActivityGamePlatforms_Desktop);
	}

	pDiscord_Client_UpdateRichPresence(&drpc_client, &activity,
		DRPC_OnPresenceUpdated, NULL, NULL);
	pDiscord_Activity_Drop(&activity);
	drpc_presence_visible = true;
}

void DRPC_Init(void)
{
	Discord_String empty;

	if (drpc_initialized || dedicated || M_CheckParm("-nodiscord") || !cv_discordpresence.value)
		return;

	if (!DRPC_LoadLibrary())
	{
		CONS_Printf("Discord Rich Presence unavailable (discord_partner_sdk.dll not loaded).\n");
		return;
	}

	pDiscord_Client_Init(&drpc_client);
	pDiscord_Client_SetApplicationId(&drpc_client, DRPC_APPLICATION_ID);
	pDiscord_Client_SetActivityJoinCallback(&drpc_client, DRPC_OnJoin, NULL, NULL);
	DRPC_LoadAssetIndex();
	empty = DRPC_String("");
	if (!pDiscord_Client_RegisterLaunchCommand(&drpc_client, DRPC_APPLICATION_ID, empty))
		CONS_Alert(CONS_WARNING, "Discord could not register SRB2's launch command.\n");

	drpc_initialized = true;
	drpc_next_update = 0;
	drpc_previous_signature[0] = '\0';
	I_AddExitFunc(DRPC_Shutdown);
	CONS_Printf("Discord Rich Presence initialized.\n");
}

void DRPC_Update(void)
{
	tic_t now;

	if (!drpc_initialized)
	{
		if (cv_discordpresence.value)
			DRPC_Init();
		return;
	}

	pDiscord_RunCallbacks();

	if (!cv_discordpresence.value)
	{
		if (drpc_presence_visible)
		{
			pDiscord_Client_ClearRichPresence(&drpc_client);
			drpc_presence_visible = false;
			drpc_previous_signature[0] = '\0';
		}
		return;
	}

	now = I_GetTime();
	if (drpc_next_update && now < drpc_next_update)
		return;
	drpc_next_update = now + DRPC_UPDATE_INTERVAL;
	DRPC_SetPresence();
}

void DRPC_Shutdown(void)
{
	if (!drpc_initialized)
		return;

	pDiscord_Client_ClearRichPresence(&drpc_client);
	pDiscord_RunCallbacks();
	pDiscord_Client_Drop(&drpc_client);
	drpc_initialized = false;
	drpc_presence_visible = false;
	drpc_previous_signature[0] = '\0';
	DRPC_ClearAssetIndex();
	SDL_UnloadObject(drpc_library);
	drpc_library = NULL;
}

#else

void DRPC_Init(void) {}
void DRPC_Update(void) {}
void DRPC_Shutdown(void) {}

#endif // _WIN32

void DRPC_RegisterVariables(void)
{
	CV_RegisterVar(&cv_discordpresence);
	CV_RegisterVar(&cv_discordjoinaddress);
}
