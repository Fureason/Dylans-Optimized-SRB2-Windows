/*
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>
  Copyright (C) 2020-2022 Collabora Ltd.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.
*/

#include "../src/SDL_internal.h"

#include <stdio.h>
#include <string.h>

static int run_test(void);

#if HAVE_LIBUDEV_H || defined(SDL_JOYSTICK_LINUX)

#include <stdint.h>

#include "SDL_stdinc.h"
#include "SDL_endian.h"
#include "../src/core/linux/SDL_evdev_capabilities.h"
#include "../src/core/linux/SDL_evdev_capabilities.c"

static const struct
{
    int code;
    const char *name;
} device_classes[] = {
#define CLS(x)                  \
    {                           \
        SDL_UDEV_DEVICE_##x, #x \
    }
    CLS(MOUSE),
    CLS(KEYBOARD),
    CLS(JOYSTICK),
    CLS(SOUND),
    CLS(TOUCHSCREEN),
    CLS(ACCELEROMETER),
    CLS(TOUCHPAD),
#undef CLS
    { 0, NULL }
};

typedef struct
{
    const char *name;
    uint16_t bus_type;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t version;
    uint8_t ev[(EV_MAX + 1) / 8];
    uint8_t keys[(KEY_MAX + 1) / 8];
    uint8_t abs[(ABS_MAX + 1) / 8];
    uint8_t rel[(REL_MAX + 1) / 8];
    uint8_t ff[(FF_MAX + 1) / 8];
    uint8_t props[INPUT_PROP_MAX / 8];
    int expected;
} GuessTest;

  
                                                               
  
                                                        
                                  
  
                                                                     
                                 
   
#define ZEROx4 0, 0, 0, 0
#define ZEROx8 ZEROx4, ZEROx4
#define FFx4   0xff, 0xff, 0xff, 0xff
#define FFx8   FFx4, FFx4

 
   
static const GuessTest guess_tests[] =
{
    {
      .name = "Xbox 360 wired USB controller",
      .bus_type = 0x0003,
      .vendor_id = 0x045e,
      .product_id = 0x028e,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b, 0x00, 0x20 },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xdb, 0x7c,
      },
    },
    {
      .name = "X-Box One Elite",
      .bus_type = 0x0003,
      .vendor_id = 0x045e,
      .product_id = 0x02e3,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xdb, 0x7c,
      },
    },
    {
      .name = "X-Box One S via Bluetooth",
      .bus_type = 0x0005,
      .vendor_id = 0x045e,
      .product_id = 0x02e0,
      .version = 0x1130,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xdb, 0x7c,
      },
    },
    {
      .name = "X-Box One S wired",
      .bus_type = 0x0003,
      .vendor_id = 0x045e,
      .product_id = 0x02ea,
      .version = 0x0301,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xdb, 0x7c,
      },
    },
    {
      .name = "DualShock 4 - gamepad",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x09cc,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
                                                              
                                   
      .ev = { 0x1b, 0x00, 0x20 },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                              
            ZEROx4, 0x00, 0x00, 0xdb, 0x7f,
      },
    },
    {
      .name = "DualShock 4 - gamepad via Bluetooth",
      .bus_type = 0x0005,
      .vendor_id = 0x054c,
      .product_id = 0x09cc,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                              
            ZEROx4, 0x00, 0x00, 0xdb, 0x7f,
      },
    },
    {
      .name = "DualShock 4 - touchpad",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x09cc,
      .expected = SDL_UDEV_DEVICE_TOUCHPAD,
       
      .ev = { 0x0b },
       
      .abs = { 0x03, 0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x02 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            0x00, 0x00, 0x01, 0x00, ZEROx4,
           
            0x20, 0x24, 0x00, 0x00
      },
       
      .props = { 0x05 },
    },
    {
      .name = "DualShock 4 - accelerometer",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x09cc,
      .expected = SDL_UDEV_DEVICE_ACCELEROMETER,
       
      .ev = { 0x19 },
       
      .abs = { 0x3f },
       
      .props = { 0x40 },
    },
    {
      .name = "DualShock 4 via USB dongle",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x0ba0,
      .version = 0x8111,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                              
            ZEROx4, 0x00, 0x00, 0xdb, 0x7f,
      },
    },
    {
      .name = "DualShock 3 - gamepad",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x0268,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x1b, 0x00, 0x20 },
       
      .abs = { 0x3f },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                              
            ZEROx4, 0x00, 0x00, 0xdb, 0x7f,
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            ZEROx4, 0x0f, 0x00, 0x00, 0x00,
      },
    },
    {
      .name = "DualShock 3 - accelerometer",
      .bus_type = 0x0003,
      .vendor_id = 0x054c,
      .product_id = 0x0268,
      .expected = SDL_UDEV_DEVICE_ACCELEROMETER,
       
      .ev = { 0x09 },
       
      .abs = { 0x07 },
       
      .props = { 0x40 },
    },
    {
      .name = "Steam Controller - gamepad",
      .bus_type = 0x0003,
      .vendor_id = 0x28de,
      .product_id = 0x1142,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x1b, 0x00, 0x33 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                                                                
            ZEROx4, 0x06, 0x00, 0xdb, 0x7f,
           
            0x00, 0x00, 0x03, 0x00, ZEROx4,
            ZEROx8,
            ZEROx8,
           
            ZEROx4, 0x0f, 0x00, 0x00, 0x00,
      },
    },
    {
                                                                    
                        
      .name = "Steam Controller - dongle",
      .bus_type = 0x0003,
      .vendor_id = 0x28de,
      .product_id = 0x1142,
      .expected = (SDL_UDEV_DEVICE_KEYBOARD
                   | SDL_UDEV_DEVICE_MOUSE),
       
      .ev = { 0x17, 0x00, 0x12 },
       
      .rel = { 0x03, 0x09 },
      .keys = {
            0xfe, 0xff, 0xff, 0xff, FFx4,
            0xff, 0xff, 0xcf, 0x01, 0xdf, 0xff, 0x80, 0xe0,
            ZEROx8,
            ZEROx8,
            0x00, 0x00, 0x1f, 0x00, ZEROx4,
      },
    },
    {
      .name = "Guitar Hero for PS3",
      .bus_type = 0x0003,
      .vendor_id = 0x12ba,
      .product_id = 0x0100,
      .version = 0x0110,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x27, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xff, 0x1f,
      },
    },
    {
      .name = "G27 Racing Wheel, 0003:046d:c29b v0111",
      .bus_type = 0x0003,
      .vendor_id = 0x046d,
      .product_id = 0xc29b,
      .version = 0x0111,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x27, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                                                                        
            ZEROx4, 0xff, 0xff, 0x00, 0x00,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            0x7f,
      },
    },
    {
      .name = "Logitech Driving Force, 0003:046d:c294 v0100",
      .bus_type = 0x0003,
      .vendor_id = 0x046d,
      .product_id = 0xc294,
      .version = 0x0100,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x23, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                            
            ZEROx4, 0xff, 0x0f, 0x00, 0x00,
      },
    },
    {
      .name = "Logitech Dual Action",
      .bus_type = 0x0003,
      .vendor_id = 0x046d,
      .product_id = 0xc216,
      .version = 0x0110,
                                                                    
                                                                    
                     
       
       
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x27, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                            
            ZEROx4, 0xff, 0x0f, 0x00, 0x00,
      },
    },
    {
      .name = "Saitek ST290 Pro flight stick",
      .bus_type = 0x0003,
      .vendor_id = 0x06a3,
      .product_id = 0x0160,    
      .version = 0x0100,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x1b },
       
      .abs = { 0x27, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x3f, 0x00, 0x00, 0x00,
      },
    },
    {
      .name = "Saitek X52 Pro Flight Control System",
      .bus_type = 0x0003,
      .vendor_id = 0x06a3,
      .product_id = 0x0762,
      .version = 0x0111,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
      .ev = { 0x0b },
       
      .abs = { 0x7f, 0x00, 0x03, 0x00, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                                                                        
            ZEROx4, 0xff, 0xff, 0x00, 0x00,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            0xff, 0xff, 0x7f,
      },
    },
    {
      .name = "Logitech Extreme 3D",
      .bus_type = 0x0003,
      .vendor_id = 0x046d,
      .product_id = 0xc215,
      .version = 0x0110,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x63, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                            
            ZEROx4, 0xff, 0x0f, 0x00, 0x00,
      },
    },
    {
      .name = "Hori Real Arcade Pro VX-SA",
      .bus_type = 0x0003,
      .vendor_id = 0x24c6,
      .product_id = 0x5501,
      .version = 0x0533,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0xdb, 0x7c,
      },
    },
    {
      .name = "Switch Pro Controller via Bluetooth",
      .bus_type = 0x0005,
      .vendor_id = 0x057e,
      .product_id = 0x2009,
      .version = 0x0001,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x1b, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                         
                                                                        
            ZEROx4, 0xff, 0xff, 0x00, 0x00,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            0x03,
      },
    },
    {
      .name = "Switch Pro Controller via USB",
      .bus_type = 0x0003,
      .vendor_id = 0x057e,
      .product_id = 0x2009,
      .version = 0x0111,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x27, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
      },
    },
    {
      .name = "Thrustmaster Racing Wheel FFB",
                                                                 
                                                      
                                    
      .bus_type = 0x0003,
      .vendor_id = 0x044f,
      .product_id = 0xb66d,
      .version = 0x0110,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
      .ev = { 0x0b },
       
      .abs = { 0x3f, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                             
                      
            ZEROx4, 0x00, 0x00, 0xff, 0x3f,
      },
    },
    {
      .name = "Thrustmaster T.Flight Hotas X",
      .bus_type = 0x0003,
      .vendor_id = 0x044f,
      .product_id = 0xb108,
      .version = 0x0100,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
      .ev = { 0x0b },
       
      .abs = { 0x67, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                             
                            
            ZEROx4, 0xff, 0x0f
      },
    },
    {
      .name = "8BitDo N30 Pro 2",
      .bus_type = 0x0003,
      .vendor_id = 0x2dc8,
      .product_id = 0x9015,
      .version = 0x0111,
       
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
      .ev = { 0x0b },
       
      .abs = { 0x27, 0x06, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                                     
                      
            ZEROx4, 0x00, 0x00, 0xff, 0x7f,
      },
    },
    {
      .name = "Retro Power SNES-style controller, 0003:0079:0011 v0110",
      .bus_type = 0x0003,
      .vendor_id = 0x0079,
      .product_id = 0x0011,
      .version = 0x0110,
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
      .ev = { 0x0b },
       
      .abs = { 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                             
                            
            ZEROx4, 0xff, 0x03, 0x00, 0x00,
      },
    },
    {
      .name = "Wiimote - buttons",
      .bus_type = 0x0005,
      .vendor_id = 0x057e,
      .product_id = 0x0306,
      .version = 0x8600,
                                                                       
                                                                     
                                                 
       
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x03 },
      .keys = {
            ZEROx8,
           
            ZEROx4, 0x80, 0x16, 0x00, 0x00,
            ZEROx8,
            ZEROx8,
           
            0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10,
            ZEROx8,
           
            0x00, 0x00, 0x80, 0x10, ZEROx4,
      },
    },
    {
      .name = "Wiimote - Motion Plus or accelerometer",
      .bus_type = 0x0005,
      .vendor_id = 0x057e,
      .product_id = 0x0306,
      .version = 0x8600,
      .expected = SDL_UDEV_DEVICE_ACCELEROMETER,
       
      .ev = { 0x09 },
       
      .abs = { 0x38 },
    },
    {
      .name = "Wiimote - IR positioning",
      .bus_type = 0x0005,
      .vendor_id = 0x057e,
      .product_id = 0x0306,
      .version = 0x8600,
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x09 },
       
      .abs = { 0x00, 0x1f },
    },
    {
      .name = "Wiimote - Nunchuck",
      .bus_type = 0x0005,
      .vendor_id = 0x057e,
      .product_id = 0x0306,
      .version = 0x8600,
       
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x0b },
       
      .abs = { 0x38, 0x00, 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
          
            ZEROx4, 0x00, 0x00, 0x24, 0x00,
      },
    },
    {
       
      .name = "Wiimote - Classic Controller",
                                                                   
                                                        
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x0b },
       
      .abs = { 0x00, 0x1c },
      .keys = {
            ZEROx8,
           
            ZEROx4, 0x80, 0x16, 0x00, 0x00,
            ZEROx8,
            ZEROx8,
           
            ZEROx4, 0x00, 0x13, 0xdb, 0x10,
            ZEROx8,
           
            0x00, 0x00, 0x80, 0x10, ZEROx4,
      },
    },
    {
       
      .name = "Wiimote - Balance Board",
       
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x0b },
       
      .abs = { 0x00, 0x0f },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0x01, 0x00,
      },
    },
    {
       
      .name = "Wiimote - Wii U Pro Controller",
      .expected = SDL_UDEV_DEVICE_JOYSTICK,
       
      .ev = { 0x0b },
       
      .abs = { 0x1b },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
                                                               
                              
            ZEROx4, 0x00, 0x00, 0xdb, 0x7f,
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            ZEROx4, 0x0f, 0x00, 0x00, 0x00,
      },
    },
    {
      .name = "Synaptics TM3381-002 (Thinkpad X280 trackpad)",
      .bus_type = 0x001d,    
      .vendor_id = 0x06cb,
      .product_id = 0x0000,
      .version = 0x0000,
      .expected = SDL_UDEV_DEVICE_TOUCHPAD,
       
      .ev = { 0x0b },
       
      .abs = { 0x03, 0x00, 0x00, 0x01, 0x00, 0x80, 0xf3, 0x06 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            0x00, 0x00, 0x01, 0x00, ZEROx4,
           
            0x20, 0xe5
      },
       
      .props = { 0x05 },
    },
    {
      .name = "TPPS/2 Elan TrackPoint (Thinkpad X280)",
      .bus_type = 0x0011,    
      .vendor_id = 0x0002,
      .product_id = 0x000a,
      .version = 0x0000,
      .expected = SDL_UDEV_DEVICE_MOUSE,
       
      .ev = { 0x07 },
       
      .rel = { 0x03 },
      .keys = {
            ZEROx8, ZEROx8, ZEROx8, ZEROx8,
           
            0x00, 0x00, 0x07,
      },
       
      .props = { 0x21 },
    },
    {
      .name = "Thinkpad ACPI buttons",
       
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x33 },
      .keys = {
            ZEROx8,
            ZEROx4, 0x00, 0x00, 0x0e, 0x01,
            0x00, 0x50, 0x11, 0x51, 0x00, 0x28, 0x00, 0xc0,
            0x04, 0x20, 0x10, 0x02, 0x1b, 0x70, 0x01, 0x00,
            ZEROx8,
            ZEROx4, 0x00, 0x00, 0x50, 0x00,
            ZEROx8,
            0x00, 0x00, 0x04, 0x18, ZEROx4,
            ZEROx8,
            0x40, 0x00, 0x01, 0x00, ZEROx4,
      },
    },
    {
      .name = "PC speaker",
      .bus_type = 0x0010,    
      .vendor_id = 0x001f,
      .product_id = 0x0001,
      .version = 0x0100,
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x01, 0x00, 0x04 },
    },
    {
      .name = "ALSA headphone detection, etc.",
      .bus_type = 0x0000,
      .vendor_id = 0x0000,
      .product_id = 0x0000,
      .version = 0x0000,
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x21 },
    },
    {
       
      .name = "Thinkpad T520 and X280 keyboards",
      .bus_type = 0x0011,    
      .vendor_id = 0x0001,
      .product_id = 0x0001,
      .version = 0xab54,
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x13, 0x00, 0x12 },
      .keys = {
            0xfe, 0xff, 0xff, 0xff, FFx4,
            0xff, 0xff, 0xef, 0xff, 0xdf, 0xff, 0xff, 0xfe,
            0x01, 0xd0, 0x00, 0xf8, 0x78, 0x30, 0x80, 0x03,
            0x00, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00,
      },
    },
    {
      .name = "Thinkpad X280 sleep button",
      .bus_type = 0x0019,    
      .vendor_id = 0x0000,
      .product_id = 0x0003,
      .version = 0x0000,
       
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x03 },
      .keys = {
            ZEROx8,
            ZEROx8,
           
            0x00, 0x40,
      },
    },
    {
      .name = "Thinkpad X280 lid switch",
      .bus_type = 0x0019,    
      .vendor_id = 0x0000,
      .product_id = 0x0005,
      .version = 0x0000,
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
       
      .ev = { 0x21 },
    },
    {
      .name = "Thinkpad X280 power button",
      .bus_type = 0x0019,    
      .vendor_id = 0x0000,
      .product_id = 0x0001,
      .version = 0x0000,
       
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x03 },
      .keys = {
            ZEROx8,
           
            ZEROx4, 0x00, 0x00, 0x10, 0x00,
      },
    },
    {
      .name = "Thinkpad X280 video bus",
      .bus_type = 0x0019,    
      .vendor_id = 0x0000,
      .product_id = 0x0006,
      .version = 0x0000,
       
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x03 },
      .keys = {
            ZEROx8,
            ZEROx8,
            ZEROx8,
           
            ZEROx4, 0x0b, 0x00, 0x3e, 0x00,
      },
    },
    {
      .name = "Thinkpad X280 extra buttons",
      .bus_type = 0x0019,    
      .vendor_id = 0x17aa,
      .product_id = 0x5054,
      .version = 0x4101,
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x03 },
      .keys = {
            ZEROx8,
            ZEROx4, 0x00, 0x00, 0x0e, 0x01,
            0x00, 0x50, 0x11, 0x51, 0x00, 0x28, 0x00, 0xc0,
            0x04, 0x20, 0x10, 0x02, 0x1b, 0x70, 0x01, 0x00,
            ZEROx8,
            ZEROx4, 0x00, 0x00, 0x50, 0x00,
            ZEROx8,
            0x00, 0x00, 0x04, 0x18, ZEROx4,
            ZEROx8,
            0x40, 0x00, 0x01, 0x00, ZEROx4,
      },
    },
    {
      .name = "Thinkpad USB keyboard with Trackpoint - keyboard",
      .bus_type = 0x0003,
      .vendor_id = 0x17ef,
      .product_id = 0x6009,
      .expected = SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x13, 0x00, 0x12 },
      .keys = {
            0xfe, 0xff, 0xff, 0xff, FFx4,
            0xff, 0xff, 0xef, 0xff, 0xdf, 0xff, 0xbe, 0xfe,
            0xff, 0x57, 0x40, 0xc1, 0x7a, 0x20, 0x9f, 0xff,
            0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      },
    },
    {
      .name = "Thinkpad USB keyboard with Trackpoint - Trackpoint",
      .bus_type = 0x0003,
      .vendor_id = 0x17ef,
      .product_id = 0x6009,
                                                                   
                                                               
                                                                               
      .expected = SDL_UDEV_DEVICE_MOUSE | SDL_UDEV_DEVICE_KEYBOARD,
       
      .ev = { 0x17, 0x00, 0x02 },
       
      .rel = { 0x03 },
      .keys = {
            ZEROx8,
            ZEROx4, 0x00, 0x00, 0x1e, 0x00,
            0x00, 0xcc, 0x11, 0x01, 0x78, 0x40, 0x00, 0xc0,
            0x00, 0x20, 0x10, 0x00, 0x0b, 0x50, 0x00, 0x00,
           
            0x00, 0x00, 0x87, 0x68, ZEROx4,
            ZEROx4, 0x00, 0x00, 0x10, 0x00,
            ZEROx4, 0x00, 0x00, 0x40, 0x00,
      },
    },
    {
      .name = "No information",
      .expected = SDL_UDEV_DEVICE_UNKNOWN,
    }
};
   

                                                                     
                                                                          
                                                                  
                                                                      
                                                                     
                                                                           
                           
  
                                                                        
                                                                           
                                                                          
                                                                           
                                                                            
SDL_COMPILE_TIME_ASSERT(sizeof_long, sizeof(unsigned long) == 4 || sizeof(unsigned long) == 8);
#define SwapLongLE(X) \
    ((sizeof(unsigned long) == 4) ? SDL_SwapLE32(X) : SDL_SwapLE64(X))

static int
run_test(void)
{
    int success = 1;
    size_t i;

    for (i = 0; i < SDL_arraysize(guess_tests); i++) {
        const GuessTest *t = &guess_tests[i];
        size_t j;
        int actual;
        struct
        {
            unsigned long ev[NBITS(EV_MAX)];
            unsigned long abs[NBITS(ABS_MAX)];
            unsigned long keys[NBITS(KEY_MAX)];
            unsigned long rel[NBITS(REL_MAX)];
        } caps = {};

        printf("%s...\n", t->name);

        memset(&caps, '\0', sizeof(caps));
        memcpy(caps.ev, t->ev, sizeof(t->ev));
        memcpy(caps.keys, t->keys, sizeof(t->keys));
        memcpy(caps.abs, t->abs, sizeof(t->abs));
        memcpy(caps.rel, t->rel, sizeof(t->rel));

        for (j = 0; j < SDL_arraysize(caps.ev); j++) {
            caps.ev[j] = SwapLongLE(caps.ev[j]);
        }

        for (j = 0; j < SDL_arraysize(caps.keys); j++) {
            caps.keys[j] = SwapLongLE(caps.keys[j]);
        }

        for (j = 0; j < SDL_arraysize(caps.abs); j++) {
            caps.abs[j] = SwapLongLE(caps.abs[j]);
        }

        for (j = 0; j < SDL_arraysize(caps.rel); j++) {
            caps.rel[j] = SwapLongLE(caps.rel[j]);
        }

        actual = SDL_EVDEV_GuessDeviceClass(caps.ev, caps.abs, caps.keys,
                                            caps.rel);

        if (actual == t->expected) {
            printf("\tOK\n");
        } else {
            printf("\tExpected 0x%08x\n", t->expected);

            for (j = 0; device_classes[j].code != 0; j++) {
                if (t->expected & device_classes[j].code) {
                    printf("\t\t%s\n", device_classes[j].name);
                }
            }

            printf("\tGot      0x%08x\n", actual);

            for (j = 0; device_classes[j].code != 0; j++) {
                if (actual & device_classes[j].code) {
                    printf("\t\t%s\n", device_classes[j].name);
                }
            }

            success = 0;
        }
    }

    return success;
}

#else  

static int
run_test(void)
{
    printf("SDL compiled without evdev capability check.\n");
    return 1;
}

#endif

int main(int argc, char *argv[])
{
    return run_test() ? 0 : 1;
}
