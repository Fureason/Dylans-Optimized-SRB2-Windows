/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "testutils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#ifndef SDL_JOYSTICK_DISABLED

#define SCREEN_WIDTH    512
#define SCREEN_HEIGHT   320

#define BUTTON_SIZE     50
#define AXIS_SIZE       50

#define BUTTON_SIZE 50
#define AXIS_SIZE   50

 
static const struct
{
    int x;
    int y;
} button_positions[] = {
    { 387, 167 },  
    { 431, 132 },  
    { 342, 132 },  
    { 389, 101 },  
    { 174, 132 },  
    { 232, 128 },  
    { 289, 132 },  
    { 75, 154 },   
    { 305, 230 },  
    { 77, 40 },    
    { 396, 36 },   
    { 154, 188 },  
    { 154, 249 },  
    { 116, 217 },  
    { 186, 217 },  
    { 232, 174 },  
    { 132, 135 },  
    { 330, 135 },  
    { 132, 175 },  
    { 330, 175 },  
    { 0, 0 },      
};
SDL_COMPILE_TIME_ASSERT(button_positions, SDL_arraysize(button_positions) == SDL_CONTROLLER_BUTTON_MAX);

 
static const struct
{
    int x;
    int y;
    double angle;
} axis_positions[] = {
    { 74, 153, 270.0 },   
    { 74, 153, 0.0 },     
    { 306, 231, 270.0 },  
    { 306, 231, 0.0 },    
    { 91, -20, 0.0 },     
    { 375, -20, 0.0 },    
};
SDL_COMPILE_TIME_ASSERT(axis_positions, SDL_arraysize(axis_positions) == SDL_CONTROLLER_AXIS_MAX);

 
static const char *power_level_strings[] = {
    "unknown",  
    "empty",    
    "low",      
    "medium",   
    "full",     
    "wired",    
};
SDL_COMPILE_TIME_ASSERT(power_level_strings, SDL_arraysize(power_level_strings) == SDL_JOYSTICK_POWER_MAX + 1);

static SDL_Window *window = NULL;
static SDL_Renderer *screen = NULL;
static SDL_bool retval = SDL_FALSE;
static SDL_bool done = SDL_FALSE;
static SDL_bool set_LED = SDL_FALSE;
static int trigger_effect = 0;
static SDL_Texture *background_front, *background_back, *button_texture, *axis_texture;
static SDL_GameController *gamecontroller;
static SDL_GameController **gamecontrollers;
static int num_controllers = 0;
static SDL_Joystick *virtual_joystick = NULL;
static SDL_GameControllerAxis virtual_axis_active = SDL_CONTROLLER_AXIS_INVALID;
static int virtual_axis_start_x;
static int virtual_axis_start_y;
static SDL_GameControllerButton virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;

static void UpdateWindowTitle()
{
    if (window == NULL) {
        return;
    }

    if (gamecontroller) {
        const char *name = SDL_GameControllerName(gamecontroller);
        const char *serial = SDL_GameControllerGetSerial(gamecontroller);
        const char *basetitle = "Game Controller Test: ";
        const size_t titlelen = SDL_strlen(basetitle) + (name ? SDL_strlen(name) : 0) + (serial ? 3 + SDL_strlen(serial) : 0) + 1;
        char *title = (char *)SDL_malloc(titlelen);

        retval = SDL_FALSE;
        done = SDL_FALSE;

        if (title) {
            SDL_strlcpy(title, basetitle, titlelen);
            if (name) {
                SDL_strlcat(title, name, titlelen);
            }
            if (serial) {
                SDL_strlcat(title, " (", titlelen);
                SDL_strlcat(title, serial, titlelen);
                SDL_strlcat(title, ")", titlelen);
            }
            SDL_SetWindowTitle(window, title);
            SDL_free(title);
        }
    } else {
        SDL_SetWindowTitle(window, "Waiting for controller...");
    }
}

static const char *GetSensorName(SDL_SensorType sensor)
{
    switch (sensor) {
    case SDL_SENSOR_ACCEL:
        return "accelerometer";
    case SDL_SENSOR_GYRO:
        return "gyro";
    case SDL_SENSOR_ACCEL_L:
        return "accelerometer (L)";
    case SDL_SENSOR_GYRO_L:
        return "gyro (L)";
    case SDL_SENSOR_ACCEL_R:
        return "accelerometer (R)";
    case SDL_SENSOR_GYRO_R:
        return "gyro (R)";
    default:
        return "UNKNOWN";
    }
}

static int FindController(SDL_JoystickID controller_id)
{
    int i;

    for (i = 0; i < num_controllers; ++i) {
        if (controller_id == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamecontrollers[i]))) {
            return i;
        }
    }
    return -1;
}

static void AddController(int device_index, SDL_bool verbose)
{
    SDL_JoystickID controller_id = SDL_JoystickGetDeviceInstanceID(device_index);
    SDL_GameController *controller;
    SDL_GameController **controllers;
    Uint16 firmware_version;
    SDL_SensorType sensors[] = {
        SDL_SENSOR_ACCEL,
        SDL_SENSOR_GYRO,
        SDL_SENSOR_ACCEL_L,
        SDL_SENSOR_GYRO_L,
        SDL_SENSOR_ACCEL_R,
        SDL_SENSOR_GYRO_R
    };
    unsigned int i;

    controller_id = SDL_JoystickGetDeviceInstanceID(device_index);
    if (controller_id < 0) {
        SDL_Log("Couldn't get controller ID: %s\n", SDL_GetError());
        return;
    }

    if (FindController(controller_id) >= 0) {
         
        return;
    }

    controller = SDL_GameControllerOpen(device_index);
    if (controller == NULL) {
        SDL_Log("Couldn't open controller: %s\n", SDL_GetError());
        return;
    }

    controllers = (SDL_GameController **)SDL_realloc(gamecontrollers, (num_controllers + 1) * sizeof(*controllers));
    if (controllers == NULL) {
        SDL_GameControllerClose(controller);
        return;
    }

    controllers[num_controllers++] = controller;
    gamecontrollers = controllers;
    gamecontroller = controller;
    trigger_effect = 0;

    if (verbose) {
        const char *name = SDL_GameControllerName(gamecontroller);
        const char *path = SDL_GameControllerPath(gamecontroller);
        SDL_Log("Opened game controller %s%s%s\n", name, path ? ", " : "", path ? path : "");
    }

    firmware_version = SDL_GameControllerGetFirmwareVersion(gamecontroller);
    if (firmware_version) {
        if (verbose) {
            SDL_Log("Firmware version: 0x%x (%d)\n", firmware_version, firmware_version);
        }
    }

    for (i = 0; i < SDL_arraysize(sensors); ++i) {
        SDL_SensorType sensor = sensors[i];

        if (SDL_GameControllerHasSensor(gamecontroller, sensor)) {
            if (verbose) {
                SDL_Log("Enabling %s at %.2f Hz\n", GetSensorName(sensor), SDL_GameControllerGetSensorDataRate(gamecontroller, sensor));
            }
            SDL_GameControllerSetSensorEnabled(gamecontroller, sensor, SDL_TRUE);
        }
    }

    if (SDL_GameControllerHasRumble(gamecontroller)) {
        SDL_Log("Rumble supported");
    }

    if (SDL_GameControllerHasRumbleTriggers(gamecontroller)) {
        SDL_Log("Trigger rumble supported");
    }

    UpdateWindowTitle();
}

static void SetController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0) {
        return;
    }

    if (gamecontroller != gamecontrollers[i]) {
        gamecontroller = gamecontrollers[i];
        UpdateWindowTitle();
    }
}

static void DelController(SDL_JoystickID controller)
{
    int i = FindController(controller);

    if (i < 0) {
        return;
    }

    SDL_GameControllerClose(gamecontrollers[i]);

    --num_controllers;
    if (i < num_controllers) {
        SDL_memcpy(&gamecontrollers[i], &gamecontrollers[i + 1], (num_controllers - i) * sizeof(*gamecontrollers));
    }

    if (num_controllers > 0) {
        gamecontroller = gamecontrollers[0];
    } else {
        gamecontroller = NULL;
    }
    UpdateWindowTitle();
}

static Uint16 ConvertAxisToRumble(Sint16 axisval)
{
     
    const Sint16 half_axis = (Sint16)SDL_ceil(SDL_JOYSTICK_AXIS_MAX / 2.0f);
    if (axisval > half_axis) {
        return (Uint16)(axisval - half_axis) * 4;
    } else {
        return 0;
    }
}

                                    
                                                                       
  
typedef struct
{
    Uint8 ucEnableBits1;               
    Uint8 ucEnableBits2;               
    Uint8 ucRumbleRight;               
    Uint8 ucRumbleLeft;                
    Uint8 ucHeadphoneVolume;           
    Uint8 ucSpeakerVolume;             
    Uint8 ucMicrophoneVolume;          
    Uint8 ucAudioEnableBits;           
    Uint8 ucMicLightMode;              
    Uint8 ucAudioMuteBits;             
    Uint8 rgucRightTriggerEffect[11];  
    Uint8 rgucLeftTriggerEffect[11];   
    Uint8 rgucUnknown1[6];             
    Uint8 ucLedFlags;                  
    Uint8 rgucUnknown2[2];             
    Uint8 ucLedAnim;                   
    Uint8 ucLedBrightness;             
    Uint8 ucPadLights;                 
    Uint8 ucLedRed;                    
    Uint8 ucLedGreen;                  
    Uint8 ucLedBlue;                   
} DS5EffectsState_t;

static void CyclePS5TriggerEffect()
{
    DS5EffectsState_t state;

    Uint8 effects[3][11] = {
         
        { 0x05, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
         
        { 0x01, 0, 110, 0, 0, 0, 0, 0, 0, 0, 0 },
         
        { 0x06, 15, 63, 128, 0, 0, 0, 0, 0, 0, 0 },
    };

    trigger_effect = (trigger_effect + 1) % SDL_arraysize(effects);

    SDL_zero(state);
    state.ucEnableBits1 |= (0x04 | 0x08);  
    SDL_memcpy(state.rgucRightTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_memcpy(state.rgucLeftTriggerEffect, effects[trigger_effect], sizeof(effects[trigger_effect]));
    SDL_GameControllerSendEffect(gamecontroller, &state, sizeof(state));
}

static SDL_bool ShowingFront()
{
    SDL_bool showing_front = SDL_TRUE;
    int i;

    if (gamecontroller) {
         
        for (i = SDL_CONTROLLER_BUTTON_PADDLE1; i <= SDL_CONTROLLER_BUTTON_PADDLE4; ++i) {
            if (SDL_GameControllerGetButton(gamecontroller, (SDL_GameControllerButton)i) == SDL_PRESSED) {
                showing_front = SDL_FALSE;
                break;
            }
        }
    }
    if ((SDL_GetModState() & KMOD_SHIFT) != 0) {
        showing_front = SDL_FALSE;
    }
    return showing_front;
}

static void SDLCALL VirtualControllerSetPlayerIndex(void *userdata, int player_index)
{
    SDL_Log("Virtual Controller: player index set to %d\n", player_index);
}

static int SDLCALL VirtualControllerRumble(void *userdata, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble)
{
    SDL_Log("Virtual Controller: rumble set to %d/%d\n", low_frequency_rumble, high_frequency_rumble);
    return 0;
}

static int SDLCALL VirtualControllerRumbleTriggers(void *userdata, Uint16 left_rumble, Uint16 right_rumble)
{
    SDL_Log("Virtual Controller: trigger rumble set to %d/%d\n", left_rumble, right_rumble);
    return 0;
}

static int SDLCALL VirtualControllerSetLED(void *userdata, Uint8 red, Uint8 green, Uint8 blue)
{
    SDL_Log("Virtual Controller: LED set to RGB %d,%d,%d\n", red, green, blue);
    return 0;
}

static void OpenVirtualController()
{
    SDL_VirtualJoystickDesc desc;
    int virtual_index;

    SDL_zero(desc);
    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.SetPlayerIndex = VirtualControllerSetPlayerIndex;
    desc.Rumble = VirtualControllerRumble;
    desc.RumbleTriggers = VirtualControllerRumbleTriggers;
    desc.SetLED = VirtualControllerSetLED;

    virtual_index = SDL_JoystickAttachVirtualEx(&desc);
    if (virtual_index < 0) {
        SDL_Log("Couldn't open virtual device: %s\n", SDL_GetError());
    } else {
        virtual_joystick = SDL_JoystickOpen(virtual_index);
        if (virtual_joystick == NULL) {
            SDL_Log("Couldn't open virtual device: %s\n", SDL_GetError());
        }
    }
}

static void CloseVirtualController()
{
    int i;

    for (i = SDL_NumJoysticks(); i--;) {
        if (SDL_JoystickIsVirtual(i)) {
            SDL_JoystickDetachVirtual(i);
        }
    }

    if (virtual_joystick) {
        SDL_JoystickClose(virtual_joystick);
        virtual_joystick = NULL;
    }
}

static SDL_GameControllerButton FindButtonAtPosition(int x, int y)
{
    SDL_Point point;
    int i;
    SDL_bool showing_front = ShowingFront();

    point.x = x;
    point.y = y;
    for (i = 0; i < SDL_CONTROLLER_BUTTON_TOUCHPAD; ++i) {
        SDL_bool on_front = (i < SDL_CONTROLLER_BUTTON_PADDLE1 || i > SDL_CONTROLLER_BUTTON_PADDLE4);
        if (on_front == showing_front) {
            SDL_Rect rect;
            rect.x = button_positions[i].x;
            rect.y = button_positions[i].y;
            rect.w = BUTTON_SIZE;
            rect.h = BUTTON_SIZE;
            if (SDL_PointInRect(&point, &rect)) {
                return (SDL_GameControllerButton)i;
            }
        }
    }
    return SDL_CONTROLLER_BUTTON_INVALID;
}

static SDL_GameControllerAxis FindAxisAtPosition(int x, int y)
{
    SDL_Point point;
    int i;
    SDL_bool showing_front = ShowingFront();

    point.x = x;
    point.y = y;
    for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        if (showing_front) {
            SDL_Rect rect;
            rect.x = axis_positions[i].x;
            rect.y = axis_positions[i].y;
            rect.w = AXIS_SIZE;
            rect.h = AXIS_SIZE;
            if (SDL_PointInRect(&point, &rect)) {
                return (SDL_GameControllerAxis)i;
            }
        }
    }
    return SDL_CONTROLLER_AXIS_INVALID;
}

static void VirtualControllerMouseMotion(int x, int y)
{
    if (virtual_button_active != SDL_CONTROLLER_BUTTON_INVALID) {
        if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID) {
            const int MOVING_DISTANCE = 2;
            if (SDL_abs(x - virtual_axis_start_x) >= MOVING_DISTANCE ||
                SDL_abs(y - virtual_axis_start_y) >= MOVING_DISTANCE) {
                SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_RELEASED);
                virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;
            }
        }
    }

    if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID) {
        if (virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
            virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
            int range = (SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN);
            float distance = SDL_clamp(((float)y - virtual_axis_start_y) / AXIS_SIZE, 0.0f, 1.0f);
            Sint16 value = (Sint16)(SDL_JOYSTICK_AXIS_MIN + (distance * range));
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, value);
        } else {
            float distanceX = SDL_clamp(((float)x - virtual_axis_start_x) / AXIS_SIZE, -1.0f, 1.0f);
            float distanceY = SDL_clamp(((float)y - virtual_axis_start_y) / AXIS_SIZE, -1.0f, 1.0f);
            Sint16 valueX, valueY;

            if (distanceX >= 0) {
                valueX = (Sint16)(distanceX * SDL_JOYSTICK_AXIS_MAX);
            } else {
                valueX = (Sint16)(distanceX * -SDL_JOYSTICK_AXIS_MIN);
            }
            if (distanceY >= 0) {
                valueY = (Sint16)(distanceY * SDL_JOYSTICK_AXIS_MAX);
            } else {
                valueY = (Sint16)(distanceY * -SDL_JOYSTICK_AXIS_MIN);
            }
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, valueX);
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active + 1, valueY);
        }
    }
}

static void VirtualControllerMouseDown(int x, int y)
{
    SDL_GameControllerButton button;
    SDL_GameControllerAxis axis;

    button = FindButtonAtPosition(x, y);
    if (button != SDL_CONTROLLER_BUTTON_INVALID) {
        virtual_button_active = button;
        SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_PRESSED);
    }

    axis = FindAxisAtPosition(x, y);
    if (axis != SDL_CONTROLLER_AXIS_INVALID) {
        virtual_axis_active = axis;
        virtual_axis_start_x = x;
        virtual_axis_start_y = y;
    }
}

static void VirtualControllerMouseUp(int x, int y)
{
    if (virtual_button_active != SDL_CONTROLLER_BUTTON_INVALID) {
        SDL_JoystickSetVirtualButton(virtual_joystick, virtual_button_active, SDL_RELEASED);
        virtual_button_active = SDL_CONTROLLER_BUTTON_INVALID;
    }

    if (virtual_axis_active != SDL_CONTROLLER_AXIS_INVALID) {
        if (virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
            virtual_axis_active == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, SDL_JOYSTICK_AXIS_MIN);
        } else {
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active, 0);
            SDL_JoystickSetVirtualAxis(virtual_joystick, virtual_axis_active + 1, 0);
        }
        virtual_axis_active = SDL_CONTROLLER_AXIS_INVALID;
    }
}

void loop(void *arg)
{
    SDL_Event event;
    int i;
    SDL_bool showing_front;

     
    SDL_PumpEvents();

     
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1) {
        switch (event.type) {
        case SDL_CONTROLLERDEVICEADDED:
            SDL_Log("Game controller device %d added.\n", (int)SDL_JoystickGetDeviceInstanceID(event.cdevice.which));
            AddController(event.cdevice.which, SDL_TRUE);
            break;

        case SDL_CONTROLLERDEVICEREMOVED:
            SDL_Log("Game controller device %d removed.\n", (int)event.cdevice.which);
            DelController(event.cdevice.which);
            break;

        case SDL_CONTROLLERTOUCHPADDOWN:
        case SDL_CONTROLLERTOUCHPADMOTION:
        case SDL_CONTROLLERTOUCHPADUP:
            SDL_Log("Controller %" SDL_PRIs32 " touchpad %" SDL_PRIs32 " finger %" SDL_PRIs32 " %s %.2f, %.2f, %.2f\n",
                    event.ctouchpad.which,
                    event.ctouchpad.touchpad,
                    event.ctouchpad.finger,
                    (event.type == SDL_CONTROLLERTOUCHPADDOWN ? "pressed at" : (event.type == SDL_CONTROLLERTOUCHPADUP ? "released at" : "moved to")),
                    event.ctouchpad.x,
                    event.ctouchpad.y,
                    event.ctouchpad.pressure);
            break;

#define VERBOSE_SENSORS
#ifdef VERBOSE_SENSORS
        case SDL_CONTROLLERSENSORUPDATE:
            SDL_Log("Controller %" SDL_PRIs32 " sensor %s: %.2f, %.2f, %.2f (%" SDL_PRIu64 ")\n",
                    event.csensor.which,
                    GetSensorName((SDL_SensorType)event.csensor.sensor),
                    event.csensor.data[0],
                    event.csensor.data[1],
                    event.csensor.data[2],
                    event.csensor.timestamp_us);
            break;
#endif  

#define VERBOSE_AXES
#ifdef VERBOSE_AXES
        case SDL_CONTROLLERAXISMOTION:
            if (event.caxis.value <= (-SDL_JOYSTICK_AXIS_MAX / 2) || event.caxis.value >= (SDL_JOYSTICK_AXIS_MAX / 2)) {
                SetController(event.caxis.which);
            }
            SDL_Log("Controller %" SDL_PRIs32 " axis %s changed to %d\n", event.caxis.which, SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)event.caxis.axis), event.caxis.value);
            break;
#endif  

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                SetController(event.cbutton.which);
            }
            SDL_Log("Controller %" SDL_PRIs32 " button %s %s\n", event.cbutton.which, SDL_GameControllerGetStringForButton((SDL_GameControllerButton)event.cbutton.button), event.cbutton.state ? "pressed" : "released");

             
            if (event.type == SDL_CONTROLLERBUTTONDOWN &&
                event.cbutton.button == SDL_CONTROLLER_BUTTON_MISC1 &&
                SDL_GameControllerGetType(gamecontroller) == SDL_CONTROLLER_TYPE_PS5) {
                CyclePS5TriggerEffect();
            }
            break;

        case SDL_JOYBATTERYUPDATED:
            SDL_Log("Controller %" SDL_PRIs32 " battery state changed to %s\n", event.jbattery.which, power_level_strings[event.jbattery.level + 1]);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (virtual_joystick) {
                VirtualControllerMouseDown(event.button.x, event.button.y);
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (virtual_joystick) {
                VirtualControllerMouseUp(event.button.x, event.button.y);
            }
            break;

        case SDL_MOUSEMOTION:
            if (virtual_joystick) {
                VirtualControllerMouseMotion(event.motion.x, event.motion.y);
            }
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9) {
                if (gamecontroller) {
                    int player_index = (event.key.keysym.sym - SDLK_0);

                    SDL_GameControllerSetPlayerIndex(gamecontroller, player_index);
                }
                break;
            }
            if (event.key.keysym.sym == SDLK_a) {
                OpenVirtualController();
                break;
            }
            if (event.key.keysym.sym == SDLK_d) {
                CloseVirtualController();
                break;
            }
            if (event.key.keysym.sym != SDLK_ESCAPE) {
                break;
            }
            SDL_FALLTHROUGH;
        case SDL_QUIT:
            done = SDL_TRUE;
            break;
        default:
            break;
        }
    }

    showing_front = ShowingFront();

     
    SDL_SetRenderDrawColor(screen, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(screen);
    SDL_RenderCopy(screen, showing_front ? background_front : background_back, NULL, NULL);

    if (gamecontroller) {
         
        for (i = 0; i < SDL_CONTROLLER_BUTTON_TOUCHPAD; ++i) {
            if (SDL_GameControllerGetButton(gamecontroller, (SDL_GameControllerButton)i) == SDL_PRESSED) {
                SDL_bool on_front = (i < SDL_CONTROLLER_BUTTON_PADDLE1 || i > SDL_CONTROLLER_BUTTON_PADDLE4);
                if (on_front == showing_front) {
                    SDL_Rect dst;
                    dst.x = button_positions[i].x;
                    dst.y = button_positions[i].y;
                    dst.w = BUTTON_SIZE;
                    dst.h = BUTTON_SIZE;
                    SDL_RenderCopyEx(screen, button_texture, NULL, &dst, 0, NULL, SDL_FLIP_NONE);
                }
            }
        }

        if (showing_front) {
            for (i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
                const Sint16 deadzone = 8000;  
                const Sint16 value = SDL_GameControllerGetAxis(gamecontroller, (SDL_GameControllerAxis)(i));
                if (value < -deadzone) {
                    const double angle = axis_positions[i].angle;
                    SDL_Rect dst;
                    dst.x = axis_positions[i].x;
                    dst.y = axis_positions[i].y;
                    dst.w = AXIS_SIZE;
                    dst.h = AXIS_SIZE;
                    SDL_RenderCopyEx(screen, axis_texture, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
                } else if (value > deadzone) {
                    const double angle = axis_positions[i].angle + 180.0;
                    SDL_Rect dst;
                    dst.x = axis_positions[i].x;
                    dst.y = axis_positions[i].y;
                    dst.w = AXIS_SIZE;
                    dst.h = AXIS_SIZE;
                    SDL_RenderCopyEx(screen, axis_texture, NULL, &dst, angle, NULL, SDL_FLIP_NONE);
                }
            }
        }

         
        {
            Sint16 x = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 y = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTY);

            if (!set_LED) {
                set_LED = (x < -8000 || x > 8000 || y > 8000);
            }
            if (set_LED) {
                Uint8 r, g, b;

                if (x < 0) {
                    r = (Uint8)(((~x) * 255) / 32767);
                    b = 0;
                } else {
                    r = 0;
                    b = (Uint8)(((int)(x)*255) / 32767);
                }
                if (y > 0) {
                    g = (Uint8)(((int)(y)*255) / 32767);
                } else {
                    g = 0;
                }

                SDL_GameControllerSetLED(gamecontroller, r, g, b);
            }
        }

        if (trigger_effect == 0) {
             
            {
                Sint16 left = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
                Sint16 right = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
                Uint16 low_frequency_rumble = ConvertAxisToRumble(left);
                Uint16 high_frequency_rumble = ConvertAxisToRumble(right);
                SDL_GameControllerRumble(gamecontroller, low_frequency_rumble, high_frequency_rumble, 250);
            }

             
            {
                Sint16 left = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_LEFTY);
                Sint16 right = SDL_GameControllerGetAxis(gamecontroller, SDL_CONTROLLER_AXIS_RIGHTY);
                Uint16 left_rumble = ConvertAxisToRumble(~left);
                Uint16 right_rumble = ConvertAxisToRumble(~right);

                SDL_GameControllerRumbleTriggers(gamecontroller, left_rumble, right_rumble, 250);
            }
        }
    }
    SDL_Delay(16);
    SDL_RenderPresent(screen);

#ifdef __EMSCRIPTEN__
    if (done) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char *argv[])
{
    int i;
    int controller_count = 0;
    int controller_index = 0;
    char guid[64];

    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_STEAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ROG_CHAKRAM, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_LINUX_JOYSTICK_DEADZONES, "1");

     
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

     
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

     
    if (argv[1] && SDL_strcmp(argv[1], "--mappings") == 0) {
        SDL_Log("Supported mappings:\n");
        for (i = 0; i < SDL_GameControllerNumMappings(); ++i) {
            char *mapping = SDL_GameControllerMappingForIndex(i);
            if (mapping) {
                SDL_Log("\t%s\n", mapping);
                SDL_free(mapping);
            }
        }
        SDL_Log("\n");
    }

     
    for (i = 0; i < SDL_NumJoysticks(); ++i) {
        const char *name;
        const char *path;
        const char *description;

        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),
                                  guid, sizeof(guid));

        if (SDL_IsGameController(i)) {
            controller_count++;
            name = SDL_GameControllerNameForIndex(i);
            path = SDL_GameControllerPathForIndex(i);
            switch (SDL_GameControllerTypeForIndex(i)) {
            case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
                description = "Amazon Luna Controller";
                break;
            case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
                description = "Google Stadia Controller";
                break;
            case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
            case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
            case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
                description = "Nintendo Switch Joy-Con";
                break;
            case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
                description = "Nintendo Switch Pro Controller";
                break;
            case SDL_CONTROLLER_TYPE_PS3:
                description = "PS3 Controller";
                break;
            case SDL_CONTROLLER_TYPE_PS4:
                description = "PS4 Controller";
                break;
            case SDL_CONTROLLER_TYPE_PS5:
                description = "PS5 Controller";
                break;
            case SDL_CONTROLLER_TYPE_XBOX360:
                description = "XBox 360 Controller";
                break;
            case SDL_CONTROLLER_TYPE_XBOXONE:
                description = "XBox One Controller";
                break;
            case SDL_CONTROLLER_TYPE_VIRTUAL:
                description = "Virtual Game Controller";
                break;
            default:
                description = "Game Controller";
                break;
            }
            AddController(i, SDL_FALSE);
        } else {
            name = SDL_JoystickNameForIndex(i);
            path = SDL_JoystickPathForIndex(i);
            description = "Joystick";
        }
        SDL_Log("%s %d: %s%s%s (guid %s, VID 0x%.4x, PID 0x%.4x, player index = %d)\n",
                description, i, name ? name : "Unknown", path ? ", " : "", path ? path : "", guid,
                SDL_JoystickGetDeviceVendor(i), SDL_JoystickGetDeviceProduct(i), SDL_JoystickGetDevicePlayerIndex(i));
    }
    SDL_Log("There are %d game controller(s) attached (%d joystick(s))\n", controller_count, SDL_NumJoysticks());

     
    window = SDL_CreateWindow("Game Controller Test", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, 0);
    if (window == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s\n", SDL_GetError());
        return 2;
    }

    screen = SDL_CreateRenderer(window, -1, 0);
    if (screen == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return 2;
    }

    SDL_SetRenderDrawColor(screen, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(screen);
    SDL_RenderPresent(screen);

     
    SDL_RenderSetLogicalSize(screen, SCREEN_WIDTH, SCREEN_HEIGHT);

    background_front = LoadTexture(screen, "controllermap.bmp", SDL_FALSE, NULL, NULL);
    background_back = LoadTexture(screen, "controllermap_back.bmp", SDL_FALSE, NULL, NULL);
    button_texture = LoadTexture(screen, "button.bmp", SDL_TRUE, NULL, NULL);
    axis_texture = LoadTexture(screen, "axis.bmp", SDL_TRUE, NULL, NULL);

    if (background_front == NULL || background_back == NULL || button_texture == NULL || axis_texture == NULL) {
        SDL_DestroyRenderer(screen);
        SDL_DestroyWindow(window);
        return 2;
    }
    SDL_SetTextureColorMod(button_texture, 10, 255, 21);
    SDL_SetTextureColorMod(axis_texture, 10, 255, 21);

     
     

    for (i = 1; i < argc; ++i) {
        if (SDL_strcmp(argv[i], "--virtual") == 0) {
            OpenVirtualController();
        }
        if (argv[i] && *argv[i] != '-') {
            controller_index = SDL_atoi(argv[i]);
            break;
        }
    }
    if (controller_index < num_controllers) {
        gamecontroller = gamecontrollers[controller_index];
    } else {
        gamecontroller = NULL;
    }
    UpdateWindowTitle();

     
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(loop, NULL, 0, 1);
#else
    while (!done) {
        loop(NULL);
    }
#endif

     
    if (trigger_effect != 0) {
        trigger_effect = -1;
        CyclePS5TriggerEffect();
    }

    CloseVirtualController();
    SDL_DestroyRenderer(screen);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

    return 0;
}

#else

int
main(int argc, char *argv[])
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL compiled without Joystick support.\n");
    return 1;
}

#endif

 
