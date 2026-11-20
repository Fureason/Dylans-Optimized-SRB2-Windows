// Shared Windows, Linux and Android controller setup, GPLv2.
#include "m_gamepadcapture.h"
static void M_ResetMenuMouse(void);
static void M_MenuMouseButton(INT32 item, INT32 key, INT32 x, INT32 y, INT32 width, const char *label);
static boolean M_MenuMouseInput(event_t *ev, INT32 *key);

enum { GP_MOVE = 1, GP_CAMERA, GP_JUMP, GP_SPIN, GP_FIRE, GP_NORMALFIRE };
enum { GP_UNKNOWN, GP_LEFT_H, GP_LEFT_V, GP_RIGHT_H, GP_RIGHT_V, GP_LEFT_TRIGGER, GP_RIGHT_TRIGGER };
static struct
{
	boolean valid;
	SDL_JoystickID id;
	INT32 roles[GP_CAPTURE_AXES], center[GP_CAPTURE_AXES];
	INT32 button_roles[JOYBUTTONS];
	UINT32 sticks, all, peers[GP_CAPTURE_AXES];
} gp_info;
static struct
{
	boolean active, secondary;
	INT32 action;
	INT32 button;
	SDL_JoystickID device;
	tic_t deadline;
	menu_t *menu;
	gp_capture_t input;
} gp_capture;
static const char *gp_status;
static tic_t gp_status_until;

static SDL_Joystick *M_GPDevice(boolean secondary)
{
	return secondary ? JoyInfo2.dev : JoyInfo.dev;
}

static void M_GPRefresh(SDL_Joystick *device)
{
	INT32 i, j, count, index;
	SDL_GameController *controller = NULL;
	static const SDL_GameControllerAxis axes[] = {
		SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
		SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY,
		SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT
	};
	if (!device || !SDL_JoystickGetAttached(device)) { gp_info.valid = false; return; }
	if (gp_info.valid && gp_info.id == SDL_JoystickInstanceID(device)) return;
	memset(&gp_info, 0, sizeof gp_info);
	gp_info.id = SDL_JoystickInstanceID(device);
	gp_info.valid = true;
	count = min(GP_CAPTURE_AXES, SDL_JoystickNumAxes(device));
	index = I_GetJoystickDeviceIndex(device);
	if (index >= 0 && SDL_IsGameController(index)) controller = SDL_GameControllerOpen(index);
	if (controller)
	{
		for (i = 0; i < 6; i++)
		{
			SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForAxis(controller, axes[i]);
			if (bind.bindType == SDL_CONTROLLER_BINDTYPE_AXIS && bind.value.axis >= 0 && bind.value.axis < count)
				gp_info.roles[bind.value.axis] = i + 1;
		}
		for (i = 0; i < 15; i++)
		{
			SDL_GameControllerButtonBind bind = SDL_GameControllerGetBindForButton(controller, (SDL_GameControllerButton)i);
			if (bind.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON)
			{
				INT32 key = I_JoystickButtonKey(bind.value.button, false)-KEY_JOY1;
				if (key >= 0 && key < JOYBUTTONS) gp_info.button_roles[key] = i+1;
			}
		}
		SDL_GameControllerClose(controller);
	}
#ifdef __ANDROID__
	/* Android's SDL driver sorts X/Y/Z/RZ before trigger axes. This also
	 * names sticks on older SDL builds lacking a mapping for new controllers. */
	if (count >= 4 && !gp_info.roles[0] && !gp_info.roles[1]
		&& !gp_info.roles[2] && !gp_info.roles[3])
		for (i = 0; i < 4; i++) gp_info.roles[i] = i + 1;
#endif
	for (i = 0; i < count; i++)
	{
		Sint16 initial = 0;
		gp_info.all |= 1u << i;
		if (!SDL_JoystickGetAxisInitialState(device, i, &initial)) initial = SDL_JoystickGetAxis(device, i);
		if (gp_info.roles[i] >= GP_LEFT_H && gp_info.roles[i] <= GP_RIGHT_V)
			gp_info.sticks |= 1u << i;
		else if (gp_info.roles[i] == GP_UNKNOWN && abs(initial) < 24000)
			gp_info.sticks |= 1u << i;
		if (!(gp_info.sticks & (1u << i))) gp_info.center[i] = initial < -24000 ? -32768 : initial > 24000 ? 32767 : 0;
		for (j = 0; j < count; j++)
			if ((gp_info.roles[i] == GP_LEFT_H || gp_info.roles[i] == GP_LEFT_V)
				? (gp_info.roles[j] == GP_LEFT_H || gp_info.roles[j] == GP_LEFT_V)
				: (gp_info.roles[i] == GP_RIGHT_H || gp_info.roles[i] == GP_RIGHT_V)
				&& (gp_info.roles[j] == GP_RIGHT_H || gp_info.roles[j] == GP_RIGHT_V))
				gp_info.peers[i] |= 1u << j;
	}
}

static consvar_t *M_GPBinding(INT32 action, boolean secondary)
{
	switch (action)
	{
		case GP_JUMP: return secondary ? &cv_jumpaxis2 : &cv_jumpaxis;
		case GP_SPIN: return secondary ? &cv_spinaxis2 : &cv_spinaxis;
		case GP_FIRE: return secondary ? &cv_fireaxis2 : &cv_fireaxis;
		default: return secondary ? &cv_firenaxis2 : &cv_firenaxis;
	}
}

static const char *M_GPStickName(INT32 horizontal, INT32 vertical)
{
	INT32 h = abs(horizontal) - 1, v = abs(vertical) - 1;
	if (!horizontal && !vertical) return "None";
	if (h < 0 || v < 0 || h >= GP_CAPTURE_AXES || v >= GP_CAPTURE_AXES) return "Custom Stick";
	if (gp_info.valid && gp_info.roles[h] == GP_LEFT_H && gp_info.roles[v] == GP_LEFT_V) return "Left Stick";
	if (gp_info.valid && gp_info.roles[h] == GP_RIGHT_H && gp_info.roles[v] == GP_RIGHT_V) return "Right Stick";
	return "Custom Stick";
}

static const char *M_GPInputName(INT32 value)
{
	INT32 axis = abs(value) - 1;
	if (!value) return "None";
	if (!gp_info.valid || axis >= GP_CAPTURE_AXES) return "Custom Input";
	switch (gp_info.roles[axis])
	{
		case GP_LEFT_H: return value > 0 ? "Left Stick Right" : "Left Stick Left";
		case GP_LEFT_V: return value > 0 ? "Left Stick Down" : "Left Stick Up";
		case GP_RIGHT_H: return value > 0 ? "Right Stick Right" : "Right Stick Left";
		case GP_RIGHT_V: return value > 0 ? "Right Stick Down" : "Right Stick Up";
		case GP_LEFT_TRIGGER: return "Left Trigger";
		case GP_RIGHT_TRIGGER: return "Right Trigger";
		default: return "Custom Input";
	}
}

static INT32 M_GPControl(INT32 action)
{
	switch (action)
	{
		case GP_JUMP: return GC_JUMP;
		case GP_SPIN: return GC_SPIN;
		case GP_FIRE: return GC_FIRE;
		default: return GC_FIRENORMAL;
	}
}

static boolean M_GPButtonKey(INT32 key, boolean secondary)
{
	INT32 first = secondary ? KEY_2JOY1 : KEY_JOY1;
	return key >= first && key < first+JOYBUTTONS+JOYHATS*4;
}

static const char *M_GPButtonName(INT32 key, boolean secondary)
{
	static const char *names[] = {"Cross / A", "Circle / B", "Square / X", "Triangle / Y",
		"Share / View", "Home", "Options / Menu", "L3", "R3", "L1 / LB", "R1 / RB",
		"D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right"};
	INT32 index = key-(secondary ? KEY_2JOY1 : KEY_JOY1);
	if (gp_info.valid && index >= 0 && index < JOYBUTTONS && gp_info.button_roles[index])
		return names[gp_info.button_roles[index]-1];
	if (index >= JOYBUTTONS && index < JOYBUTTONS+4) return names[11+index-JOYBUTTONS];
	return va("Button %d", index+1);
}

static const char *M_GPActionName(INT32 action, boolean secondary)
{
	INT32 value = M_GPBinding(action, secondary)->value;
	INT32 (*controls)[2] = secondary ? gamecontrolbis : gamecontrol;
	if (value) return M_GPInputName(value);
	for (INT32 slot = 0; slot < 2; slot++)
		if (M_GPButtonKey(controls[M_GPControl(action)][slot], secondary))
			return M_GPButtonName(controls[M_GPControl(action)][slot], secondary);
	return "None";
}

static void M_GPFinish(boolean apply, boolean clear, const char *status)
{
	boolean secondary = gp_capture.secondary;
	if (apply)
	{
		if (gp_capture.action == GP_MOVE)
		{
			CV_SetValue(secondary ? &cv_moveaxis2 : &cv_moveaxis, clear ? 0 : -gp_capture.input.vertical);
			CV_SetValue(secondary ? &cv_sideaxis2 : &cv_sideaxis, clear ? 0 : gp_capture.input.horizontal);
		}
		else if (gp_capture.action == GP_CAMERA)
		{
			CV_SetValue(secondary ? &cv_lookaxis2 : &cv_lookaxis, clear ? 0 : gp_capture.input.vertical);
			CV_SetValue(secondary ? &cv_turnaxis2 : &cv_turnaxis, clear ? 0 : gp_capture.input.horizontal);
		}
		else
		{
			INT32 control = M_GPControl(gp_capture.action), slot;
			INT32 (*controls)[2] = secondary ? gamecontrolbis : gamecontrol;
			for (slot = 0; slot < 2; slot++)
				if (M_GPButtonKey(controls[control][slot], secondary)) controls[control][slot] = KEY_NULL;
			CV_SetValue(M_GPBinding(gp_capture.action, secondary), clear || gp_capture.button ? 0 : gp_capture.input.binding);
			if (!clear && gp_capture.button)
			{
				(void)G_CheckDoubleUsage(gp_capture.button, true);
				slot = controls[control][0] == KEY_NULL ? 0 : 1;
				controls[control][slot] = gp_capture.button;
			}
		}
	}
	gp_capture.active = false;
	gp_status = status;
	gp_status_until = I_GetTime() + 3*TICRATE;
	S_StartSound(NULL, sfx_menu1);
}

static void M_GPCapture(INT32 choice)
{
	boolean secondary = currentMenu == &OP_Joystick2Def;
	SDL_Joystick *device = M_GPDevice(secondary);
	UINT32 mask;
	M_GPRefresh(device);
	if (!gp_info.valid)
	{
		gp_status = "Connect and select a controller first.";
		gp_status_until = I_GetTime() + 4*TICRATE;
		return;
	}
	memset(&gp_capture, 0, sizeof gp_capture);
	gp_capture.action = choice;
	gp_capture.secondary = secondary;
	gp_capture.menu = currentMenu;
	gp_capture.device = gp_info.id;
	gp_capture.deadline = I_GetTime() + 30*TICRATE;
	mask = choice <= GP_CAMERA ? gp_info.sticks : gp_info.all;
	if (!mask) return;
	M_GPCaptureInit(&gp_capture.input, choice <= GP_CAMERA, mask, gp_info.center, gp_info.peers);
	gp_capture.active = true;
}

static boolean M_GPCaptureResponder(event_t *event)
{
	INT32 pointerkey = -1;
	if (!gp_capture.active) return false;
	if (event->type == ev_mouseabsolute || event->type == ev_mouse
		|| ((event->type == ev_keydown || event->type == ev_keyup)
			&& event->key >= KEY_MOUSE1 && event->key < KEY_JOY1))
	{
		M_MenuMouseInput(event, &pointerkey);
		if (pointerkey == KEY_ESCAPE) M_GPFinish(false, false, "Setup canceled. Previous binding kept.");
		return true;
	}
	if (event->type == ev_keydown)
	{
		INT32 key = event->key;
		if (gp_capture.action >= GP_JUMP && M_GPButtonKey(key, gp_capture.secondary))
		{
			if (gp_capture.input.phase == GP_MOVE_UP)
			{
				gp_capture.button = key;
				gp_capture.input.phase = GP_RELEASE_END;
				gp_capture.input.stable = 0;
			}
		}
		else if (key == KEY_ESCAPE || (gp_capture.input.paired && (key == KEY_JOY1+1 || key == KEY_2JOY1+1)))
			M_GPFinish(false, false, "Setup canceled. Previous binding kept.");
		else if (key == KEY_BACKSPACE || (gp_capture.input.paired && (key == KEY_JOY1+2 || key == KEY_2JOY1+2)))
			M_GPFinish(true, true, "Input cleared.");
	}
	return true;
}

static void M_GPCaptureTicker(void)
{
	INT32 i, values[GP_CAPTURE_AXES] = {0};
	SDL_Joystick *device;
	if (!gp_capture.active) return;
	if (!menuactive || currentMenu != gp_capture.menu) { gp_capture.active = false; return; }
	device = M_GPDevice(gp_capture.secondary);
	if (!device || !SDL_JoystickGetAttached(device) || SDL_JoystickInstanceID(device) != gp_capture.device)
	{ M_GPFinish(false, false, "Controller disconnected. Binding kept."); return; }
	if (I_GetTime() >= gp_capture.deadline)
	{ M_GPFinish(false, false, "Setup timed out. Previous binding kept."); return; }
	if (gp_capture.input.phase == GP_RELEASE_START || gp_capture.input.phase == GP_RELEASE_END)
	{
		for (i = 0; i < SDL_JoystickNumButtons(device); i++)
			if (SDL_JoystickGetButton(device, i)) { gp_capture.input.stable = 0; return; }
		for (i = 0; i < SDL_JoystickNumHats(device); i++)
			if (SDL_JoystickGetHat(device, i)) { gp_capture.input.stable = 0; return; }
	}
	for (i = 0; i < min(GP_CAPTURE_AXES, SDL_JoystickNumAxes(device)); i++) values[i] = SDL_JoystickGetAxis(device, i);
	M_GPCaptureSample(&gp_capture.input, values);
	if (gp_capture.input.phase == GP_CAPTURE_DONE) M_GPFinish(true, false, "Controller input saved.");
}

static void M_GPResetSticks(INT32 choice)
{
	boolean secondary = currentMenu == &OP_Joystick2Def;
	consvar_t *vars[] = {secondary ? &cv_sideaxis2 : &cv_sideaxis, secondary ? &cv_moveaxis2 : &cv_moveaxis,
		secondary ? &cv_turnaxis2 : &cv_turnaxis, secondary ? &cv_lookaxis2 : &cv_lookaxis};
	INT32 role, axis;
	(void)choice;
	M_GPRefresh(M_GPDevice(secondary));
	for (role = GP_LEFT_H; role <= GP_RIGHT_V; role++)
	{
		for (axis = 0; axis < GP_CAPTURE_AXES; axis++) if (gp_info.valid && gp_info.roles[axis] == role) break;
		if (axis < GP_CAPTURE_AXES) CV_SetValue(vars[role-1], role == GP_RIGHT_V ? -axis-1 : axis+1);
		else CV_Set(vars[role-1], vars[role-1]->defaultvalue);
	}
	gp_status = "Movement and camera sticks reset.";
	gp_status_until = I_GetTime() + 3*TICRATE;
}

static void M_DrawGamepadOptions(void)
{
	INT32 action;
	boolean secondary = currentMenu == &OP_Joystick2Def;
	M_GPRefresh(M_GPDevice(secondary));
	M_DrawGenericMenu();
	V_DrawRightAlignedString(BASEVIDWIDTH-currentMenu->x, currentMenu->y+22, V_YELLOWMAP,
		M_GPStickName((secondary ? cv_sideaxis2 : cv_sideaxis).value, (secondary ? cv_moveaxis2 : cv_moveaxis).value));
	V_DrawRightAlignedString(BASEVIDWIDTH-currentMenu->x, currentMenu->y+32, V_YELLOWMAP,
		M_GPStickName((secondary ? cv_turnaxis2 : cv_turnaxis).value, (secondary ? cv_lookaxis2 : cv_lookaxis).value));
	for (action = GP_JUMP; action <= GP_NORMALFIRE; action++)
		V_DrawRightAlignedSmallString(BASEVIDWIDTH-currentMenu->x, currentMenu->y+52+(action-GP_JUMP)*10+2,
			V_YELLOWMAP, M_GPActionName(action, secondary));
	if (!gp_capture.active)
	{
		V_DrawCenteredSmallString(BASEVIDWIDTH/2, 195, 0,
			gp_status && I_GetTime() < gp_status_until ? gp_status : "Select an input, then press a button or move a stick.");
		return;
	}
	V_DrawFill(16, 49, 288, 105, 159);
	M_ResetMenuMouse();
	V_DrawCenteredString(BASEVIDWIDTH/2, 59, V_YELLOWMAP,
		gp_capture.action == GP_MOVE ? "Movement Stick" : gp_capture.action == GP_CAMERA ? "Camera Stick" : "Record Controller Input");
	switch (gp_capture.input.phase)
	{
		case GP_RELEASE_START: case GP_RELEASE_MIDDLE: case GP_RELEASE_END:
			V_DrawCenteredString(BASEVIDWIDTH/2, 83, 0, "Release the controls."); break;
		case GP_MOVE_UP:
			V_DrawCenteredString(BASEVIDWIDTH/2, 83, 0, gp_capture.input.paired ? "Move the stick UP." : "Press a button, trigger or stick."); break;
		case GP_MOVE_RIGHT:
			V_DrawCenteredString(BASEVIDWIDTH/2, 83, 0, "Move the SAME stick RIGHT."); break;
		default: break;
	}
	V_DrawCenteredSmallString(BASEVIDWIDTH/2, 107, 0, gp_capture.input.paired
		? "B / Esc: cancel     Square / X / Backspace: clear"
		: "Esc / Back: cancel     Backspace: clear");
	V_DrawCenteredSmallString(BASEVIDWIDTH/2, 119, 0, "Current bindings stay unchanged until setup finishes.");
	M_MenuMouseButton(-1, KEY_ESCAPE, 120, 134, 80, "Cancel");
}

#define GP_MENU_ITEMS(secondary) \
	{IT_STRING|IT_CALL,NULL,"Select Controller...",secondary ? M_Setup2PJoystickMenu : M_Setup1PJoystickMenu,6}, \
	{IT_STRING|IT_CALL,NULL,"Movement Stick",M_GPCapture,22}, \
	{IT_STRING|IT_CALL,NULL,"Camera Stick",M_GPCapture,32}, \
	{IT_STRING|IT_CALL,NULL,"Jump Input",M_GPCapture,52}, \
	{IT_STRING|IT_CALL,NULL,"Spin Input",M_GPCapture,62}, \
	{IT_STRING|IT_CALL,NULL,"Fire Input",M_GPCapture,72}, \
	{IT_STRING|IT_CALL,NULL,"Normal Fire Input",M_GPCapture,82}, \
	{IT_STRING|IT_CVAR,NULL,"First-person Up/Down",secondary ? &cv_alwaysfreelook2 : &cv_alwaysfreelook,102}, \
	{IT_STRING|IT_CVAR,NULL,"Third-person Up/Down",secondary ? &cv_chasefreelook2 : &cv_chasefreelook,112}, \
	{IT_STRING|IT_CVAR|IT_CV_FLOATSLIDER,NULL,"Stick Deadzone",secondary ? &cv_deadzone2 : &cv_deadzone,132}, \
	{IT_STRING|IT_CVAR|IT_CV_FLOATSLIDER,NULL,"Action Deadzone",secondary ? &cv_digitaldeadzone2 : &cv_digitaldeadzone,142}, \
	{IT_STRING|IT_CALL,NULL,"Reset Stick Bindings",M_GPResetSticks,152}
static menuitem_t OP_Joystick1Menu[] = {GP_MENU_ITEMS(false)};
static menuitem_t OP_Joystick2Menu[] = {GP_MENU_ITEMS(true)};
#undef GP_MENU_ITEMS
