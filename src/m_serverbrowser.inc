// The native browser shares m_menu's pointer targets and controller responder.
// Coordinates use SRB2's 320x200 canvas in both renderers.
#define BROWSER_ROWS 9
#define BROWSER_DETAIL_ROWS 17
#define BROWSER_STANDARD_ROOM 33
static boolean browser_room_initialized;
static INT32 browser_selected = -1, browser_scroll, browser_detailscroll;
static INT32 *browser_rows;
static size_t browser_count;
static size_t browser_capacity;
typedef enum { BDETAIL_NONE, BDETAIL_ADDONS, BDETAIL_PLAYERS } browser_detail_t;
static browser_detail_t browser_detail;
static boolean browser_favorites, browser_searching;
static char browser_search[SB_ADDRESSLEN];
static boolean browser_addressediting;
static char browser_address[SB_ADDRESSLEN];
static boolean browser_dragbar;
static INT32 browser_bargrab;
static INT32 browser_filter;
static INT32 browser_lastclick = -1, browser_clickx, browser_clicky;
static tic_t browser_clicktime;

typedef enum
{
	BFOCUS_LIST, BFOCUS_INTERNET, BFOCUS_FAVORITES, BFOCUS_ROOM, BFOCUS_REFRESH,
	BFOCUS_SEARCH, BFOCUS_FILTER, BFOCUS_SORT, BFOCUS_PLAYERS, BFOCUS_ADDONS, BFOCUS_FAVORITE,
	BFOCUS_JOIN, BFOCUS_DETAIL_UP, BFOCUS_DETAIL_DOWN, BFOCUS_DETAIL_BACK
} browser_focus_t;
static browser_focus_t browser_focus = BFOCUS_LIST;
static const char *browser_focus_names[] = {"servers", "Internet / LAN", "Favorites", "room / address",
	"Refresh", "Search", "Filter", "Sort", "Players", "Add-ons", "Favorite server", "Join server",
	"Scroll up", "Scroll down", "Back to list"};

static const char *browser_filters[] = {"All servers", "Has players", "Has room", "Unmodified", "No cheats"};

static boolean M_BrowserContains(const char *text, const char *query)
{
	if (!*query) return true;
	for (; *text; text++) if (!strncasecmp(text, query, strlen(query))) return true;
	return false;
}

static int M_BrowserCompare(const void *a, const void *b)
{
	const sb_server_t *sa = SB_Server(*(const INT32 *)a), *sb = SB_Server(*(const INT32 *)b);
	INT32 result = 0;
	boolean oa = SB_Online(sa), ob = SB_Online(sb);
	if (oa != ob) return ob - oa;
	if (oa)
	{
		switch (cv_serversort.value)
		{
			case 0:
				if (LONG(sa->info.time) != LONG(sb->info.time))
					return (UINT32)LONG(sa->info.time) < (UINT32)LONG(sb->info.time) ? -1 : 1;
				break;
			case 1: result = (sa->info.modifiedgame * 2 + sa->info.cheatsenabled) - (sb->info.modifiedgame * 2 + sb->info.cheatsenabled); break;
			case 2: result = sb->info.numberofplayer - sa->info.numberofplayer; break;
			case 3: result = sa->info.numberofplayer - sb->info.numberofplayer; break;
			case 4: result = sb->info.maxplayer - sa->info.maxplayer; break;
			case 5: result = strcasecmp(sa->info.gametypename, sb->info.gametypename); break;
		}
	}
	if (!result) result = strcasecmp(sa->info.servername, sb->info.servername);
	return result ? result : strcmp(sa->address, sb->address);
}

static void M_BrowserBuildRows(void)
{
	static UINT32 lastrevision = UINT32_MAX;
	static tic_t lastsecond = (tic_t)-1;
	static INT32 lastsort = -1, lastfilter = -1;
	static boolean lastfavorites;
	static char lastsearch[sizeof browser_search];
	const UINT32 revision = SB_Revision();
	const tic_t second = I_GetTime()/TICRATE;
	const INT32 lookup = SB_LookupIndex();
	size_t count = SB_Count();
	// Data arrives at network/tic speed. A high render cap must not turn
	// filtering and qsort into thousands of identical operations per second.
	// The one-second key also advances timeout ordering for unreachable rows.
	if (lastrevision == revision && lastsecond == second
	&& lastsort == cv_serversort.value && lastfilter == browser_filter
	&& lastfavorites == browser_favorites && !strcmp(lastsearch, browser_search))
	{
		SB_Select(browser_selected);
		return;
	}
	lastrevision = revision; lastsecond = second;
	lastsort = cv_serversort.value; lastfilter = browser_filter;
	lastfavorites = browser_favorites;
	strlcpy(lastsearch, browser_search, sizeof lastsearch);
	if (count > browser_capacity || !browser_rows)
	{
		browser_capacity = max((size_t)1, count);
		browser_rows = Z_Realloc(browser_rows, browser_capacity * sizeof(*browser_rows), PU_STATIC, NULL);
	}
	browser_count = 0;
	for (size_t i = 0; i < count; i++)
	{
		const sb_server_t *entry = SB_Server(i);
		if ((INT32)i == lookup && !browser_favorites)
		{
			browser_rows[browser_count++] = lookup;
			continue;
		}
		if (browser_favorites ? !entry->favorite : (!entry->listed && !entry->favorite)) continue;
		if (!M_BrowserContains(entry->info.servername, browser_search)
		&& !M_BrowserContains(entry->address, browser_search)
		&& (!SB_Online(entry) || (!M_BrowserContains(entry->info.maptitle, browser_search)
		&& !M_BrowserContains(entry->info.gametypename, browser_search)))) continue;
		if (browser_filter && !SB_Online(entry)) continue;
		if (browser_filter == 1 && !entry->info.numberofplayer) continue;
		if (browser_filter == 2 && (entry->info.numberofplayer >= entry->info.maxplayer || entry->info.refusereason)) continue;
		if (browser_filter == 3 && entry->info.modifiedgame) continue;
		if (browser_filter == 4 && entry->info.cheatsenabled) continue;
		browser_rows[browser_count++] = (INT32)i;
	}
	qsort(browser_rows, browser_count, sizeof(*browser_rows), M_BrowserCompare);
	// The selected identity is an endpoint, not a row number or sorted net node.
	// Refreshing player counts cannot silently change the Join target.
	if (browser_selected >= 0)
	{
		boolean found = false;
		for (size_t i = 0; i < browser_count; i++) if (browser_rows[i] == browser_selected) { found = true; break; }
		if (!found) browser_selected = -1;
	}
	if (browser_selected < 0 && browser_count) browser_selected = browser_rows[0];
	browser_scroll = max(0, min(browser_scroll, (INT32)browser_count - BROWSER_ROWS));
	SB_Select(browser_selected);
}

static void M_BrowserMove(INT32 delta)
{
	INT32 row = 0;
	M_BrowserBuildRows();
	if (!browser_count) return;
	for (size_t i = 0; i < browser_count; i++) if (browser_rows[i] == browser_selected) { row = (INT32)i; break; }
	row = max(0, min((INT32)browser_count - 1, row + delta));
	browser_selected = browser_rows[row];
	if (row < browser_scroll) browser_scroll = row;
	if (row >= browser_scroll + BROWSER_ROWS) browser_scroll = row - BROWSER_ROWS + 1;
	browser_detailscroll = 0;
	SB_Select(browser_selected);
}

static void M_BrowserWheel(INT32 direction)
{
	browser_lastclick = -1;
	if (browser_addressediting) return;
	if (menumouse_navigation) browser_focus = BFOCUS_LIST;
	if (browser_detail)
	{
		size_t count = browser_detail == BDETAIL_PLAYERS ? SB_PlayerCount() : SB_AddonCount();
		browser_detailscroll = max(0, min(browser_detailscroll + direction * 3, (INT32)count - BROWSER_DETAIL_ROWS));
	}
	else browser_scroll = max(0, min(browser_scroll + direction * 3, (INT32)browser_count - BROWSER_ROWS));
}

static boolean M_BrowserPointer(event_t *ev)
{
	if (ev->type == ev_keyup && ev->key == KEY_MOUSE1) { browser_dragbar = false; return false; }
	if (ev->type == ev_keydown && ev->key == KEY_MOUSE1 && !browser_detail && !browser_addressediting
	&& menumouse_position && menumouse_x >= 198 && menumouse_x < 203
	&& menumouse_y >= 55 && menumouse_y < 163 && browser_count > BROWSER_ROWS)
	{
		INT32 height = max(7, 108 * BROWSER_ROWS / (INT32)browser_count);
		INT32 top = 55 + (108-height)*browser_scroll/((INT32)browser_count-BROWSER_ROWS);
		browser_bargrab = menumouse_y >= top && menumouse_y < top+height ? menumouse_y-top : height/2;
		browser_dragbar = true;
	}
	if (browser_dragbar && (ev->type == ev_mouseabsolute || (ev->type == ev_keydown && ev->key == KEY_MOUSE1)))
	{
		INT32 range = max(0, (INT32)browser_count-BROWSER_ROWS);
		INT32 height = max(7, 108 * BROWSER_ROWS / max(BROWSER_ROWS+1, (INT32)browser_count));
		INT32 travel = 108-height;
		browser_scroll = (min(travel, max(0, menumouse_y-55-browser_bargrab)) * range + travel/2) / travel;
		return true;
	}
	return false;
}

static void M_Connect(INT32 choice)
{
	sb_server_t *entry = browser_selected < 0 ? NULL : SB_Server((size_t)browser_selected);
	INT32 node = -1;
	(void)choice;
	if (!entry || !SB_Online(entry))
	{
		if (entry && entry->problem[0]) { M_StartMessage(va("%s.\nThis client cannot join that server.\n", SB_Status(entry)), NULL, MM_NOTHING); return; }
		M_StartMessage("This server is not responding.\nIt may be offline or unreachable.\n\nRefresh to try again.\n", NULL, MM_NOTHING);
		return;
	}
	if (modifiedgame && !SB_AddonsComplete())
	{
		M_StartMessage("Checking this server's add-ons.\nPlease wait for the list to finish,\nor refresh if it could not be read.\n", NULL, MM_NOTHING);
		return;
	}
	if (SB_NeedsCleanSession())
	{
#ifdef HAVE_SDL
		CONS_Printf("Restarting with a clean session to join %s.\n", entry->address);
		if (I_RestartForServer(entry->address)) I_Quit();
#endif
		M_StartMessage("The clean restart could not be started.\nYour current session is still open.\n\nPlease restart and join this server again.\n", NULL, MM_NOTHING);
		return;
	}
	for (UINT32 i = 0; i < serverlistcount; i++)
	{
		const char *address = I_GetNodeAddress ? I_GetNodeAddress(serverlist[i].node) : NULL;
		if (address && !strcasecmp(address, entry->address)) { node = serverlist[i].node; break; }
	}
	char address[SB_ADDRESSLEN];
	strlcpy(address, entry->address, sizeof address);
	SB_Close();
	M_ClearMenus(false); // retain the selected transport node for stock joining
	if (node >= 0) COM_BufAddText(va("connect node %d\n", node));
	else COM_BufAddText(va("connect \"%s\"\n", address));
}

static void M_Refresh(INT32 choice)
{
	(void)choice;
	if (m_waiting_mode == M_WAITING_SERVERS) return;
	if (!SB_Active()) SB_Open();
	SB_Refresh();
	CL_UpdateServerList(cv_masterserver_room_id.value >= 0, cv_masterserver_room_id.value);
}

enum { SBUI_SELECT, SBUI_FAVORITE, SBUI_TAB, SBUI_ROOM, SBUI_REFRESH, SBUI_SEARCH,
	SBUI_SORT, SBUI_FILTER, SBUI_JOIN, SBUI_ADDONS, SBUI_BACK, SBUI_SCROLL,
	SBUI_ADDRESS, SBUI_ADDRESSSAVE, SBUI_ADDRESSCANCEL, SBUI_CLEARSEARCH, SBUI_PLAYERS };

static void M_BrowserShowDetail(browser_detail_t detail)
{
	browser_detail = detail;
	browser_detailscroll = 0;
	SB_ShowPlayers(detail == BDETAIL_PLAYERS);
}

static void M_BrowserApplyLookup(void)
{
	if (SB_Lookup(browser_search))
	{
		browser_favorites = false;
		browser_selected = SB_LookupIndex();
	}
	browser_scroll = 0;
}

static browser_focus_t M_BrowserActionFocus(INT32 action, INT32 value)
{
	switch (action)
	{
		case SBUI_TAB: return value ? BFOCUS_FAVORITES : BFOCUS_INTERNET;
		case SBUI_ROOM: case SBUI_ADDRESS: return BFOCUS_ROOM;
		case SBUI_REFRESH: return BFOCUS_REFRESH;
		case SBUI_SEARCH: case SBUI_CLEARSEARCH: return BFOCUS_SEARCH;
		case SBUI_FILTER: return BFOCUS_FILTER;
		case SBUI_SORT: return BFOCUS_SORT;
		case SBUI_PLAYERS: return BFOCUS_PLAYERS;
		case SBUI_ADDONS: return BFOCUS_ADDONS;
		case SBUI_FAVORITE: return BFOCUS_FAVORITE;
		case SBUI_JOIN: return BFOCUS_JOIN;
		case SBUI_BACK: return BFOCUS_DETAIL_BACK;
		case SBUI_SCROLL: return browser_detail ? (value < 0 ? BFOCUS_DETAIL_UP : BFOCUS_DETAIL_DOWN) : BFOCUS_LIST;
		default: return BFOCUS_LIST;
	}
}

static boolean M_BrowserFocusAvailable(browser_focus_t focus)
{
	const sb_server_t *entry = browser_selected < 0 ? NULL : SB_Server((size_t)browser_selected);
	if (focus <= BFOCUS_REFRESH) return true;
	if (browser_detail) return focus >= BFOCUS_DETAIL_UP && focus <= BFOCUS_DETAIL_BACK;
	if (focus <= BFOCUS_SORT) return true;
	if (focus == BFOCUS_FAVORITE) return entry != NULL;
	if (focus == BFOCUS_PLAYERS || focus == BFOCUS_ADDONS || focus == BFOCUS_JOIN) return SB_Online(entry);
	return false;
}

static void M_BrowserSetFocus(browser_focus_t focus)
{
	browser_focus = M_BrowserFocusAvailable(focus) ? focus : BFOCUS_LIST;
	if (browser_focus == BFOCUS_LIST && !browser_detail) M_BrowserMove(0);
}

static void M_BrowserFocusBorder(INT32 x, INT32 y, INT32 width, INT32 height, browser_focus_t focus)
{
	if (menumouse_navigation || browser_addressediting || browser_focus != focus) return;
	V_DrawFill(x, y, width, 1, yellowmap[3]);
	V_DrawFill(x, y+height-1, width, 1, yellowmap[3]);
	V_DrawFill(x, y, 1, height, yellowmap[3]);
	V_DrawFill(x+width-1, y, 1, height, yellowmap[3]);
}

static void M_BrowserClick(INT32 action, INT32 value)
{
	boolean doubleclick = false;
	if (action == SBUI_SELECT && menumouse_navigation)
	{
		doubleclick = browser_lastclick == value && I_GetTime() - browser_clicktime <= TICRATE/2
			&& abs(menumouse_x-browser_clickx) <= 4 && abs(menumouse_y-browser_clicky) <= 4;
		browser_lastclick = doubleclick ? -1 : value;
		browser_clicktime = I_GetTime();
		browser_clickx = menumouse_x; browser_clicky = menumouse_y;
	}
	else browser_lastclick = -1;
	if (menumouse_navigation)
	{
		browser_focus = M_BrowserActionFocus(action, value);
		if (action == SBUI_FAVORITE && menumouse_x < 203) browser_focus = BFOCUS_LIST;
	}
	switch (action)
	{
		case SBUI_SELECT:
			browser_focus = BFOCUS_LIST;
			browser_selected = value; browser_detailscroll = 0; browser_searching = false;
			SB_Select(value);
			if (doubleclick && SB_Online(SB_Server(value))) M_Connect(0);
			break;
		case SBUI_FAVORITE:
			if (value >= 0) SB_Favorite((size_t)value);
			break;
		case SBUI_TAB:
			browser_favorites = value != 0; browser_scroll = 0; browser_searching = false;
			M_BrowserShowDetail(BDETAIL_NONE);
			break;
		case SBUI_ROOM: M_BrowserShowDetail(BDETAIL_NONE); M_RoomMenu(0); break;
		case SBUI_REFRESH: M_Refresh(0); break;
		case SBUI_SEARCH: browser_focus = BFOCUS_SEARCH; browser_searching = true; break;
		case SBUI_CLEARSEARCH:
			browser_search[0] = '\0'; browser_searching = false; browser_scroll = 0;
			SB_Lookup(NULL);
			break;
		case SBUI_SORT: CV_AddValue(&cv_serversort, 1); break;
		case SBUI_FILTER: browser_filter = (browser_filter + 1) % 5; browser_scroll = 0; break;
		case SBUI_JOIN: M_Connect(0); break;
		case SBUI_ADDONS: case SBUI_PLAYERS:
			M_BrowserShowDetail(action == SBUI_PLAYERS ? BDETAIL_PLAYERS : BDETAIL_ADDONS);
			browser_searching = false; browser_focus = BFOCUS_LIST;
			break;
		case SBUI_BACK:
		{
			browser_focus_t focus = browser_detail == BDETAIL_PLAYERS ? BFOCUS_PLAYERS : BFOCUS_ADDONS;
			M_BrowserShowDetail(BDETAIL_NONE);
			M_BrowserSetFocus(focus);
			break;
		}
		case SBUI_SCROLL: M_BrowserWheel(value); break;
		case SBUI_ADDRESS:
			M_BrowserShowDetail(BDETAIL_NONE);
			browser_addressediting = true; browser_address[0] = '\0'; browser_searching = false; break;
		case SBUI_ADDRESSSAVE:
			if (SB_ValidAddress(browser_address))
			{
				INT32 index = SB_AddFavorite(browser_address);
				if (index >= 0)
				{
					browser_addressediting = false; browser_favorites = true;
					browser_search[0] = '\0'; browser_filter = browser_scroll = 0;
					SB_Lookup(NULL);
					browser_selected = index; SB_Select(index);
					browser_focus = BFOCUS_LIST;
				}
			}
			break;
		case SBUI_ADDRESSCANCEL: browser_addressediting = false; browser_focus = BFOCUS_ROOM; break;
	}
}

static void M_BrowserTarget(INT32 action, INT32 value, INT32 x, INT32 y, INT32 width, INT32 height)
{
	M_MenuMouseAdd(0, 0, x, y, width, height, M_BrowserClick, action, value, NULL);
	if (menumouse_count) menumouse_hits[menumouse_count - 1].hoverselect = false;
}

static void M_BrowserText(INT32 x, INT32 y, INT32 width, INT32 color, const char *text)
{
	char clipped[160];
	size_t count = min(strlen(text), sizeof clipped - 1);
	memcpy(clipped, text, count); clipped[count] = '\0';
	while (count && V_SmallStringWidth(clipped, V_ALLOWLOWERCASE) > width) clipped[--count] = '\0';
	if (count < strlen(text) && count >= 2) { clipped[count-2] = '.'; clipped[count-1] = '.'; }
	V_DrawSmallString(x, y, V_ALLOWLOWERCASE | color, clipped);
}

static void M_BrowserButton(INT32 x, INT32 y, INT32 width, const char *text, INT32 action, INT32 value, boolean selectedbutton)
{
	boolean focused = !menumouse_navigation && !browser_addressediting
		&& browser_focus == M_BrowserActionFocus(action, value);
	boolean hovered = menumouse_navigation && menumouse_position && menumouse_x >= x
		&& menumouse_x < x+width && menumouse_y >= y && menumouse_y < y+11;
	V_DrawFill(x, y, width, 11, focused || hovered ? 148 : 159);
	M_BrowserText(x+3, y+3, width-6, selectedbutton ? V_YELLOWMAP : 0, text);
	M_BrowserTarget(action, value, x, y, width, 11);
	M_BrowserFocusBorder(x, y, width, 11, M_BrowserActionFocus(action, value));
}

static void M_DrawBrowserDetail(const sb_server_t *entry)
{
	const boolean players_view = browser_detail == BDETAIL_PLAYERS;
	const size_t count = players_view ? SB_PlayerCount() : SB_AddonCount();
	V_DrawFill(6, 31, 308, 145, 31);
	M_BrowserText(10, 35, 275, 0, entry ? SB_DisplayName(entry) : "Server details");
	M_BrowserText(10, 44, 290, 0, players_view ? "Players in this server" : "Required files, in load order");
	V_DrawFill(9, 53, 302, 1, 158);
	M_BrowserText(11, 56, 200, V_YELLOWMAP, players_view ? "PLAYER" : "FILE");
	M_BrowserText(252, 56, 55, V_YELLOWMAP, players_view ? "STATUS" : "SIZE");
	browser_detailscroll = max(0, min(browser_detailscroll, (INT32)count-BROWSER_DETAIL_ROWS));
	for (INT32 row = 0; row < BROWSER_DETAIL_ROWS && (size_t)(row+browser_detailscroll) < count; row++)
	{
		const size_t index = (size_t)(row + browser_detailscroll);
		INT32 y = 65 + row * 6;
		if (row & 1) V_DrawFill(9, y-1, 294, 6, 30);
		if (players_view)
		{
			const sb_player_t *player = SB_Player(index);
			M_BrowserText(11, y, 239, 0, player->name);
			M_BrowserText(252, y, 49, 0, player->spectator ? "Spectating" : "Playing");
		}
		else
		{
			const sb_addon_t *addon = SB_Addon(index);
			M_BrowserText(11, y, 239, 0, va("%u. %s", (UINT32)index+1, addon->name));
			M_BrowserText(252, y, 49, 0, addon->folder ? "Folder" : addon->size >= 1048576
				? va("%.1f MB", (double)addon->size/1048576) : va("%u KB", (addon->size+1023)/1024));
		}
	}
	if (players_view)
	{
		if (!count) M_BrowserText(11, 75, 290, 0, SB_PlayerStatus());
		else M_BrowserText(10, 180, 220, 0, va("%u player%s - live updates", (UINT32)count, count == 1 ? "" : "s"));
	}
	else
	{
		if (!count)
			M_BrowserText(11, 75, 290, 0, SB_AddonsComplete() ? "No required gameplay add-ons." : "Waiting for the server's file list...");
		M_BrowserText(10, 180, 220, 0, va("%u file%s%s", (UINT32)count, count == 1 ? "" : "s",
			SB_AddonsFailed() ? " - incomplete; refresh to retry" : SB_AddonsComplete() ? " - complete" : " - receiving..."));
	}
	M_BrowserButton(248, 179, 66, "Back to list", SBUI_BACK, 0, false);
	M_BrowserButton(301, 65, 12, "+", SBUI_SCROLL, -1, false);
	M_BrowserButton(301, 155, 12, "-", SBUI_SCROLL, 1, false);
	M_BrowserFocusBorder(9, 53, 290, 118, BFOCUS_LIST);
}

static void M_DrawConnectMenu(void)
{
	sb_server_t *entry;
	M_BrowserBuildRows();
	if (!browser_addressediting && !M_BrowserFocusAvailable(browser_focus)) M_BrowserSetFocus(BFOCUS_LIST);
	entry = browser_selected < 0 ? NULL : SB_Server((size_t)browser_selected);
	V_DrawFill(0, 0, 320, 200, 31);
	V_DrawFill(0, 0, 320, 15, 159);
	V_DrawCenteredString(164, 4, V_YELLOWMAP, "SERVER BROWSER");
	M_BrowserButton(6, 18, 76, "Internet / LAN", SBUI_TAB, 0, !browser_favorites);
	M_BrowserButton(85, 18, 64, "Favorites", SBUI_TAB, 1, browser_favorites);
	M_BrowserButton(152, 18, 92, browser_favorites ? "+ Add address" :
		cv_masterserver_room_id.value < 0 ? "Room: LAN / unlisted" :
		cv_masterserver_room_id.value == BROWSER_STANDARD_ROOM ? "Room: Standard" : "Change room",
		browser_favorites ? SBUI_ADDRESS : SBUI_ROOM, 0, false);
	M_BrowserButton(247, 18, 67, "Refresh", SBUI_REFRESH, 0, false);
	if (browser_detail) { M_DrawBrowserDetail(entry); return; }
	V_DrawFill(6, 32, 139, 10, browser_searching ? 148 : 30);
	M_BrowserText(9, 35, 120, 0, va("%s%s%s", *browser_search ? "" : "Search name or IP", browser_search,
		browser_searching && (I_GetTime() / 8 & 1) ? "_" : ""));
	M_BrowserTarget(SBUI_SEARCH, 0, 6, 32, 139, 10);
	M_BrowserFocusBorder(6, 32, 139, 10, BFOCUS_SEARCH);
	if (*browser_search || browser_searching)
	{
		M_BrowserText(135, 35, 8, V_YELLOWMAP, "x");
		M_BrowserTarget(SBUI_CLEARSEARCH, 0, 131, 32, 14, 10);
	}
	M_BrowserButton(148, 32, 79, browser_filters[browser_filter], SBUI_FILTER, 0, browser_filter != 0);
	M_BrowserButton(230, 32, 84, va("Sort: %s", cv_serversort.string), SBUI_SORT, 0, false);
	V_DrawFill(6, 45, 197, 126, 30);
	V_DrawFill(208, 45, 106, 126, 30);
	M_BrowserText(18, 47, 108, V_YELLOWMAP, "SERVER / MAP");
	M_BrowserText(135, 47, 33, V_YELLOWMAP, "PLAYERS");
	M_BrowserText(170, 47, 29, V_YELLOWMAP, "PING");
	M_BrowserTarget(SBUI_SORT, 0, 133, 45, 69, 10);
	for (INT32 row = 0; row < BROWSER_ROWS && (size_t)(row+browser_scroll) < browser_count; row++)
	{
		INT32 index = browser_rows[row+browser_scroll], y = 55 + row * 12;
		const sb_server_t *serverentry = SB_Server((size_t)index);
		boolean online = SB_Online(serverentry), chosen = index == browser_selected;
		boolean focused = chosen && browser_focus == BFOCUS_LIST && !browser_searching
			&& (!menumouse_navigation || (menumouse_position && menumouse_x >= 7 && menumouse_x < 197
				&& menumouse_y >= y && menumouse_y < y+12));
		V_DrawFill(7, y, 190, 12, focused ? 148 : (row & 1) ? 29 : 30);
		M_BrowserText(19, y+1, 110, 0, SB_DisplayName(serverentry));
		M_BrowserText(19, y+7, 110, online ? V_AQUAMAP : V_GRAYMAP,
			online ? va("%s  %s", serverentry->info.mapname, serverentry->info.gametypename) : SB_Status(serverentry));
		M_BrowserText(135, y+2, 30, 0, online ? va("%u/%u", serverentry->info.numberofplayer, serverentry->info.maxplayer) : "--");
		M_BrowserText(170, y+2, 25, 0, online ? va("%u", (UINT32)LONG(serverentry->info.time)) : "--");
		if (online) M_BrowserText(135, y+8, 61, V_YELLOWMAP, va("%s%s", serverentry->info.modifiedgame ? "MOD " : "", serverentry->info.cheatsenabled ? "CHEATS" : ""));
		M_BrowserTarget(SBUI_SELECT, index, 18, y, 179, 12);
		M_BrowserText(9, y+3, 8, V_YELLOWMAP, serverentry->favorite ? "*" : "+");
		M_BrowserTarget(SBUI_FAVORITE, index, 7, y, 11, 12);
		if (chosen) M_BrowserFocusBorder(7, y, 190, 12, BFOCUS_LIST);
	}
	if (!browser_count)
	{
		M_BrowserText(12, 67, 182, V_YELLOWMAP, browser_favorites ? "No matching favorites" : "No matching servers");
		M_BrowserText(12, 79, 182, 0, browser_favorites ? "Click + beside a server to save it." : "Choose a room, or refresh LAN.");
		M_BrowserFocusBorder(7, 55, 190, 108, BFOCUS_LIST);
	}
	// A scrollbar reflects the continuous list, rather than discrete pages.
	V_DrawFill(198, 55, 4, 108, 159);
	if (browser_count > BROWSER_ROWS)
	{
		INT32 height = max(7, 108 * BROWSER_ROWS / (INT32)browser_count);
		V_DrawFill(198, 55 + (108-height)*browser_scroll/((INT32)browser_count-BROWSER_ROWS), 4, height, 148);
		for (INT32 y = 55; y < 163; y += 12) M_BrowserTarget(SBUI_SCROLL, y < 109 ? -1 : 1, 198, y, 4, min(12,163-y));
	}
	M_BrowserText(212, 49, 98, 0, entry ? SB_DisplayName(entry) : "SERVER DETAILS");
	if (entry)
	{
		boolean online = SB_Online(entry);
		M_BrowserText(212, 58, 98, online ? V_GREENMAP : V_GRAYMAP, SB_Status(entry));
		M_BrowserText(212, 67, 98, 0, entry->address);
		if (online)
		{
			M_BrowserText(212, 77, 98, V_AQUAMAP, entry->info.maptitle);
			M_BrowserText(212, 85, 98, 0, va("%s  %s", entry->info.mapname, entry->info.gametypename));
			M_BrowserButton(211, 93, 100, va("Players: %u/%u - view", entry->info.numberofplayer, entry->info.maxplayer), SBUI_PLAYERS, 0, false);
			M_BrowserText(212, 107, 98, 0, va("Ping: %u ms", (UINT32)LONG(entry->info.time)));
			M_BrowserText(212, 115, 98, entry->info.modifiedgame ? V_YELLOWMAP : 0, va("Modified: %s", entry->info.modifiedgame ? "Yes" : "No"));
			M_BrowserText(212, 123, 98, entry->info.cheatsenabled ? V_REDMAP : 0, va("Cheats: %s", entry->info.cheatsenabled ? "On" : "Off"));
			M_BrowserText(212, 131, 98, 0, entry->info.flags & SV_DEDICATED ? "Dedicated server" : "Listen server");
			M_BrowserButton(211, 139, 100, va("Add-ons: %u%s", (UINT32)SB_AddonCount(), SB_AddonsComplete() ? " - view" : SB_AddonsFailed() ? " - partial" : "+ - reading"), SBUI_ADDONS, 0, false);
		}
		else
		{
			M_BrowserText(212, 82, 98, 0, "No recent reply.");
			M_BrowserText(212, 91, 98, 0, "May be offline or");
			M_BrowserText(212, 99, 98, 0, "blocked by a firewall.");
			if (entry->favorite) M_BrowserText(212, 113, 98, 0, "Saved to favorites.");
		}
		M_BrowserButton(211, 154, 100, entry->favorite ? "Remove favorite" : "Favorite server", SBUI_FAVORITE, browser_selected, entry->favorite);
	}
	M_BrowserText(7, 176, 192, 0, va("%u server%s%s", (UINT32)browser_count, browser_count == 1 ? "" : "s",
		m_waiting_mode ? " - searching..." : " - live updates"));
	M_BrowserText(7, 185, 194, V_GRAYMAP, "Double-click: join  F: favorite  /: search");
	if (entry && SB_Online(entry)) M_BrowserButton(208, 175, 106, "Join server", SBUI_JOIN, 0, true);
	M_BrowserText(7, 194, 306, V_GRAYMAP, browser_searching ? "Ctrl+V: paste  Enter: results  Esc: stop editing" : "Arrows: move focus  Enter: select  Tab: favorites");
	if (browser_addressediting)
	{
		menumouse_count = 0; // modal: rows and Join behind it cannot receive clicks
		V_DrawFill(16, 69, 288, 67, 159);
		V_DrawString(24, 75, V_YELLOWMAP, "FAVORITE SERVER ADDRESS");
		M_BrowserText(24, 88, 272, 0, "Enter address:port, e.g. 127.0.0.1:5029");
		V_DrawFill(24, 98, 272, 11, 30);
		M_BrowserText(27, 101, 266, 0, va("%s%s", browser_address, I_GetTime()/8 & 1 ? "_" : ""));
		M_BrowserButton(174, 118, 58, "Cancel", SBUI_ADDRESSCANCEL, 0, false);
		M_BrowserButton(238, 118, 58, "Save", SBUI_ADDRESSSAVE, 0, true);
	}
}

static void M_BrowserPaste(char *text, size_t capacity, boolean address)
{
	const char *paste = I_ClipboardPaste();
	size_t length = strlen(text);
	if (!paste) return;
	if (!address)
	{
		while (*paste && (unsigned char)*paste <= ' ') paste++;
		length = 0;
	}
	while (*paste && length+1 < capacity)
	{
		unsigned char c = (unsigned char)*paste++;
		if (!address && (c == '\r' || c == '\n')) break;
		if (c < 32 || c >= 127) continue;
		text[length] = (char)c; text[length+1] = '\0';
		if (!address || SB_ValidAddress(text)) length++;
		else text[length] = '\0';
	}
	if (!address)
	{
		while (length && text[length-1] == ' ') length--;
		text[length] = '\0';
	}
}

// Focus is separate from the selected endpoint, so live sorting cannot move
// the keyboard out of a toolbar control or change which server Enter joins.
static boolean M_BrowserNavigate(INT32 key)
{
	INT32 row = 0;
	if (key != KEY_UPARROW && key != KEY_DOWNARROW && key != KEY_LEFTARROW && key != KEY_RIGHTARROW) return false;
	M_BrowserBuildRows();
	if (!M_BrowserFocusAvailable(browser_focus)) M_BrowserSetFocus(BFOCUS_LIST);
	if (browser_focus == BFOCUS_LIST)
	{
		if (browser_detail)
		{
			if (key == KEY_UPARROW && !browser_detailscroll) M_BrowserSetFocus(BFOCUS_INTERNET);
			else if (key == KEY_UPARROW || key == KEY_DOWNARROW) M_BrowserWheel(key == KEY_UPARROW ? -1 : 1);
			else M_BrowserSetFocus(key == KEY_LEFTARROW ? BFOCUS_INTERNET : BFOCUS_DETAIL_UP);
			return true;
		}
		for (size_t i = 0; i < browser_count; i++) if (browser_rows[i] == browser_selected) { row = (INT32)i; break; }
		if (key == KEY_LEFTARROW) M_BrowserSetFocus(BFOCUS_SEARCH);
		else if (key == KEY_RIGHTARROW) M_BrowserSetFocus(M_BrowserFocusAvailable(BFOCUS_PLAYERS) ? BFOCUS_PLAYERS : M_BrowserFocusAvailable(BFOCUS_FAVORITE) ? BFOCUS_FAVORITE : BFOCUS_SORT);
		else if (key == KEY_UPARROW && (!browser_count || !row)) M_BrowserSetFocus(BFOCUS_SEARCH);
		else if (key == KEY_DOWNARROW && (size_t)(row+1) >= browser_count)
			M_BrowserSetFocus(M_BrowserFocusAvailable(BFOCUS_JOIN) ? BFOCUS_JOIN : M_BrowserFocusAvailable(BFOCUS_FAVORITE) ? BFOCUS_FAVORITE : BFOCUS_INTERNET);
		else M_BrowserMove(key == KEY_UPARROW ? -1 : 1);
	}
	else if (browser_focus >= BFOCUS_INTERNET && browser_focus <= BFOCUS_REFRESH)
	{
		if (key == KEY_LEFTARROW) M_BrowserSetFocus(browser_focus == BFOCUS_INTERNET ? BFOCUS_REFRESH : browser_focus-1);
		else if (key == KEY_RIGHTARROW) M_BrowserSetFocus(browser_focus == BFOCUS_REFRESH ? BFOCUS_INTERNET : browser_focus+1);
		else if (key == KEY_UPARROW || browser_detail) M_BrowserSetFocus(BFOCUS_LIST);
		else M_BrowserSetFocus(browser_focus <= BFOCUS_FAVORITES ? BFOCUS_SEARCH : browser_focus == BFOCUS_ROOM ? BFOCUS_FILTER : BFOCUS_SORT);
	}
	else if (browser_focus >= BFOCUS_SEARCH && browser_focus <= BFOCUS_SORT)
	{
		if (key == KEY_LEFTARROW) M_BrowserSetFocus(browser_focus == BFOCUS_SEARCH ? BFOCUS_SORT : browser_focus-1);
		else if (key == KEY_RIGHTARROW) M_BrowserSetFocus(browser_focus == BFOCUS_SORT ? BFOCUS_SEARCH : browser_focus+1);
		else if (key == KEY_DOWNARROW) M_BrowserSetFocus(BFOCUS_LIST);
		else M_BrowserSetFocus(browser_focus == BFOCUS_SEARCH ? BFOCUS_INTERNET : browser_focus == BFOCUS_FILTER ? BFOCUS_ROOM : BFOCUS_REFRESH);
	}
	else if (browser_focus >= BFOCUS_PLAYERS && browser_focus <= BFOCUS_JOIN)
	{
		if (key == KEY_LEFTARROW || key == KEY_RIGHTARROW) M_BrowserSetFocus(BFOCUS_LIST);
		else
		{
			INT32 next = browser_focus + (key == KEY_UPARROW ? -1 : 1);
			while (next >= BFOCUS_PLAYERS && next <= BFOCUS_JOIN && !M_BrowserFocusAvailable(next)) next += key == KEY_UPARROW ? -1 : 1;
			M_BrowserSetFocus(next < BFOCUS_PLAYERS ? BFOCUS_SORT : next > BFOCUS_JOIN ? BFOCUS_REFRESH : next);
		}
	}
	else
	{
		if (key == KEY_LEFTARROW) M_BrowserSetFocus(BFOCUS_LIST);
		else if (key == KEY_RIGHTARROW) M_BrowserSetFocus(BFOCUS_REFRESH);
		else if (key == KEY_UPARROW) M_BrowserSetFocus(browser_focus == BFOCUS_DETAIL_UP ? BFOCUS_REFRESH : browser_focus-1);
		else M_BrowserSetFocus(browser_focus == BFOCUS_DETAIL_BACK ? BFOCUS_REFRESH : browser_focus+1);
	}
	return true;
}

static void M_BrowserActivate(void)
{
	if (!M_BrowserFocusAvailable(browser_focus)) { M_BrowserSetFocus(BFOCUS_LIST); return; }
	switch (browser_focus)
	{
		case BFOCUS_LIST:
			if (!browser_detail && browser_count) M_Connect(0);
			else if (!browser_detail) M_BrowserSetFocus(BFOCUS_SEARCH);
			break;
		case BFOCUS_INTERNET: M_BrowserClick(SBUI_TAB, 0); break;
		case BFOCUS_FAVORITES: M_BrowserClick(SBUI_TAB, 1); break;
		case BFOCUS_ROOM: M_BrowserClick(browser_favorites ? SBUI_ADDRESS : SBUI_ROOM, 0); break;
		case BFOCUS_REFRESH: M_BrowserClick(SBUI_REFRESH, 0); break;
		case BFOCUS_SEARCH: M_BrowserClick(SBUI_SEARCH, 0); break;
		case BFOCUS_FILTER: M_BrowserClick(SBUI_FILTER, 0); break;
		case BFOCUS_SORT: M_BrowserClick(SBUI_SORT, 0); break;
		case BFOCUS_PLAYERS: M_BrowserClick(SBUI_PLAYERS, 0); break;
		case BFOCUS_ADDONS: M_BrowserClick(SBUI_ADDONS, 0); break;
		case BFOCUS_FAVORITE: M_BrowserClick(SBUI_FAVORITE, browser_selected); break;
		case BFOCUS_JOIN: M_BrowserClick(SBUI_JOIN, 0); break;
		case BFOCUS_DETAIL_UP: M_BrowserClick(SBUI_SCROLL, -1); break;
		case BFOCUS_DETAIL_DOWN: M_BrowserClick(SBUI_SCROLL, 1); break;
		case BFOCUS_DETAIL_BACK: M_BrowserClick(SBUI_BACK, 0); break;
	}
}

static void M_HandleServerPage(INT32 choice)
{
	browser_lastclick = -1;
	if (ctrldown && (browser_searching || browser_addressediting))
	{
		char *text = browser_addressediting ? browser_address : browser_search;
		if (choice == 'v' || choice == 'V') M_BrowserPaste(text,
			browser_addressediting ? sizeof browser_address : sizeof browser_search, browser_addressediting);
		else if (choice == KEY_BACKSPACE || choice == 'a' || choice == 'A') text[0] = '\0';
		else return;
		if (browser_searching) M_BrowserApplyLookup();
		browser_scroll = 0;
		return;
	}
	if (browser_addressediting)
	{
		size_t length = strlen(browser_address);
		if (choice == KEY_ESCAPE) M_BrowserClick(SBUI_ADDRESSCANCEL, 0);
		else if (choice == KEY_ENTER) M_BrowserClick(SBUI_ADDRESSSAVE, 0);
		else if (choice == KEY_BACKSPACE && length) browser_address[length-1] = '\0';
		else if (choice == KEY_DEL) browser_address[0] = '\0';
		else if (choice >= 32 && choice < 127 && length < sizeof browser_address-1)
		{
			char candidate[SB_ADDRESSLEN];
			strlcpy(candidate, browser_address, sizeof candidate);
			candidate[length] = (char)choice; candidate[length+1] = '\0';
			if (SB_ValidAddress(candidate)) strlcpy(browser_address, candidate, sizeof browser_address);
		}
		return;
	}
	if (browser_searching)
	{
		size_t length = strlen(browser_search);
		if (choice == KEY_ESCAPE || choice == KEY_ENTER)
		{
			if (choice == KEY_ENTER) M_BrowserApplyLookup();
			browser_searching = false;
			M_BrowserSetFocus(choice == KEY_ENTER ? BFOCUS_LIST : BFOCUS_SEARCH);
		}
		else if (choice == KEY_UPARROW || choice == KEY_DOWNARROW)
		{ browser_searching = false; browser_focus = BFOCUS_SEARCH; M_BrowserNavigate(choice); }
		else if (choice == KEY_BACKSPACE && length) browser_search[length-1] = '\0';
		else if (choice == KEY_DEL) browser_search[0] = '\0';
		else if (choice >= 32 && choice < 127 && length < sizeof browser_search-1)
		{ browser_search[length] = (char)choice; browser_search[length+1] = '\0'; }
		if (browser_searching && (choice == KEY_BACKSPACE || choice == KEY_DEL || (choice >= 32 && choice < 127)))
			SB_Lookup(NULL);
		browser_scroll = 0;
		return;
	}
	if (M_BrowserNavigate(choice)) return;
	if (choice == KEY_ENTER) { M_BrowserActivate(); return; }
	if (browser_detail)
	{
		if (choice == KEY_ESCAPE || choice == KEY_BACKSPACE) M_BrowserClick(SBUI_BACK, 0);
		else if (choice == KEY_PGDN) M_BrowserWheel(1);
		else if (choice == KEY_PGUP) M_BrowserWheel(-1);
		else if (choice == 'r' || choice == 'R') M_Refresh(0);
		return;
	}
	switch (choice)
	{
		case KEY_PGDN: M_BrowserSetFocus(BFOCUS_LIST); M_BrowserMove(BROWSER_ROWS); break;
		case KEY_PGUP: M_BrowserSetFocus(BFOCUS_LIST); M_BrowserMove(-BROWSER_ROWS); break;
		case KEY_HOME: M_BrowserSetFocus(BFOCUS_LIST); M_BrowserMove(-(INT32)browser_count); break;
		case KEY_END: M_BrowserSetFocus(BFOCUS_LIST); M_BrowserMove((INT32)browser_count); break;
		case KEY_TAB: M_BrowserClick(SBUI_TAB, !browser_favorites); M_BrowserSetFocus(BFOCUS_LIST); break;
		case 'f': case 'F': M_BrowserClick(SBUI_FAVORITE, browser_selected); break;
		case 'r': case 'R': M_Refresh(0); break;
		case 's': case 'S': CV_AddValue(&cv_serversort, 1); break;
		case '/': M_BrowserClick(SBUI_SEARCH, 0); break;
		case KEY_ESCAPE: case KEY_BACKSPACE: M_SetupNextMenu(&MP_MainDef); break;
	}
}

static void Command_ServerBrowser_f(void)
{
	if (Playing() || dedicated) { CONS_Printf("End the current game before browsing servers.\n"); return; }
	M_StartControlPanel();
	if (COM_Argc() > 1) strlcpy(browser_search, COM_Argv(1), sizeof browser_search);
	else browser_search[0] = '\0';
	M_ConnectMenu(-1);
	M_BrowserApplyLookup();
}

static void Command_ServerFavorite_f(void)
{
	if (COM_Argc() != 2 || !SB_ValidAddress(COM_Argv(1)))
	{ CONS_Printf("serverfavorite <address:port>: save a server, including an offline one.\n"); return; }
	if (SB_AddFavorite(COM_Argv(1)) >= 0) CONS_Printf("Server favorite saved.\n");
}

static void Command_ServerBrowserStatus_f(void)
{
	for (size_t i = 0; i < SB_Count(); i++)
	{
		const sb_server_t *entry = SB_Server(i);
		CONS_Printf("Browser: %s | %s | %s | favorite=%d | players=%d/%d | modified=%d cheats=%d\n",
			entry->address, entry->info.servername, SB_Status(entry), entry->favorite,
			SB_Online(entry) ? entry->info.numberofplayer : -1, SB_Online(entry) ? entry->info.maxplayer : -1,
			SB_Online(entry) ? entry->info.modifiedgame : -1, SB_Online(entry) ? entry->info.cheatsenabled : -1);
	}
	CONS_Printf("Browser selection: %s | addons=%u complete=%d failed=%d clean_required=%d\n",
		browser_selected < 0 ? "<none>" : SB_Server(browser_selected)->address,
		(UINT32)SB_AddonCount(), SB_AddonsComplete(), SB_AddonsFailed(), SB_NeedsCleanSession());
	CONS_Printf("Browser keyboard focus: %s\n", browser_focus_names[browser_focus]);
	if (browser_detail == BDETAIL_PLAYERS)
	{
		CONS_Printf("Browser players: %s (%u)\n", SB_PlayerStatus(), (UINT32)SB_PlayerCount());
		for (size_t i = 0; i < SB_PlayerCount(); i++)
			CONS_Printf("  %s\n", SB_Player(i)->name);
	}
}
