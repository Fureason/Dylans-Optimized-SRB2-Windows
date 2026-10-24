/*---------------------------------------------------------------------------

   rpng - simple PNG display program                             rpng-win.c

   This program decodes and displays PNG images, with gamma correction and
   optionally with a user-specified background color (in case the image has
   transparency).  It is very nearly the most basic PNG viewer possible.
   This version is for 32-bit Windows; it may compile under 16-bit Windows
   with a little tweaking (or maybe not).

   to do:
    - handle quoted command-line args (especially filenames with spaces)
    - have minimum window width:  oh well
    - use %.1023s to simplify truncation of title-bar string?

  ---------------------------------------------------------------------------

   Changelog:
    - 1.00:  initial public release
    - 1.01:  modified to allow abbreviated options; fixed long/ulong mis-
              match; switched to png_jmpbuf() macro
    - 1.02:  added extra set of parentheses to png_jmpbuf() macro; fixed
              command-line parsing bug
    - 1.10:  enabled "message window"/console (thanks to David Geldreich)
    - 2.00:  dual-licensed (added GNU GPL)
    - 2.01:  fixed improper display of usage screen on PNG error(s)
    - 2.02:  check for integer overflow (Glenn R-P)

  ---------------------------------------------------------------------------

      Copyright (c) 1998-2008, 2017 Greg Roelofs.  All rights reserved.

      This software is provided "as is," without warranty of any kind,
      express or implied.  In no event shall the author or contributors
      be held liable for any damages arising in any way from the use of
      this software.

      The contents of this file are DUAL-LICENSED.  You may modify and/or
      redistribute this software according to the terms of one of the
      following two licenses (at your option):


      LICENSE 1 ("BSD-like with advertising clause"):

      Permission is granted to anyone to use this software for any purpose,
      including commercial applications, and to alter it and redistribute
      it freely, subject to the following restrictions:

      1. Redistributions of source code must retain the above copyright
         notice, disclaimer, and this list of conditions.
      2. Redistributions in binary form must reproduce the above copyright
         notice, disclaimer, and this list of conditions in the documenta-
         tion and/or other materials provided with the distribution.
      3. All advertising materials mentioning features or use of this
         software must display the following acknowledgment:

            This product includes software developed by Greg Roelofs
            and contributors for the book, "PNG: The Definitive Guide,"
            published by O'Reilly and Associates.


      LICENSE 2 (GNU GPL v2 or later):

      This program is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 2 of the License, or
      (at your option) any later version.

      This program is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.

      You should have received a copy of the GNU General Public License
      along with this program; if not, write to the Free Software Foundation,
      Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  ---------------------------------------------------------------------------*/

#define PROGNAME  "rpng-win"
#define LONGNAME  "Simple PNG Viewer for Windows"
#define VERSION   "2.01 of 16 March 2008"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#ifdef __CYGWIN__
                                                           
                                                         
                          
   
#include <unistd.h>
#include <termio.h>
#include <sys/ioctl.h>
int repl_getch( void )
{
  char ch;
  int fd = fileno(stdin);
  struct termio old_tty, new_tty;

  ioctl(fd, TCGETA, &old_tty);
  new_tty = old_tty;
  new_tty.c_lflag &= ~(ICANON | ECHO | ISIG);
  ioctl(fd, TCSETA, &new_tty);
  fread(&ch, 1, sizeof(ch), stdin);
  ioctl(fd, TCSETA, &old_tty);

  return ch;
}
#define _getch repl_getch
#else
#include <conio.h>       
#endif

 

#include "readpng.h"     


                                                                     
                                                                        
                                                             
                             

#define alpha_composite(composite, fg, alpha, bg) {               \
    ush temp = ((ush)(fg)*(ush)(alpha) +                          \
                (ush)(bg)*(ush)(255 - (ush)(alpha)) + (ush)128);  \
    (composite) = (uch)((temp + (temp >> 8)) >> 8);               \
}


 
static int        rpng_win_create_window(HINSTANCE hInst, int showmode);
static int        rpng_win_display_image(void);
static void       rpng_win_cleanup(void);
LRESULT CALLBACK  rpng_win_wndproc(HWND, UINT, WPARAM, LPARAM);


static char titlebar[1024];
static char *progname = PROGNAME;
static char *appname = LONGNAME;
static char *filename;
static FILE *infile;

static char *bgstr;
static uch bg_red=0, bg_green=0, bg_blue=0;

static double display_exponent;

static ulg image_width, image_height, image_rowbytes;
static int image_channels;
static uch *image_data;

 
static ulg wimage_rowbytes;
static uch *dib;
static uch *wimage_data;
static BITMAPINFOHEADER *bmih;

static HWND global_hwnd;




int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, PSTR cmd, int showmode)
{
    char *args[1024];                  
    char *p, *q, **argv = args;
    int argc = 0;
    int rc, alen, flen;
    int error = 0;
    int have_bg = FALSE;
    double LUT_exponent;               
    double CRT_exponent = 2.2;         
    double default_display_exponent;   
    MSG msg;


    filename = (char *)NULL;

#ifndef __CYGWIN__
                                                                           
                                                                        
                                                                               
                      

    AllocConsole();
    freopen("CONOUT$", "a", stderr);
    freopen("CONOUT$", "a", stdout);
#endif


                                                                        
                                                                        
                                                                     
                                                                       
                                                                        
                                                                         

#if defined(NeXT)
    LUT_exponent = 1.0 / 2.2;
      
                                                           
                                        
       
#elif defined(sgi)
    LUT_exponent = 1.0 / 1.7;
                                                                  
                                                 
    infile = fopen("/etc/config/system.glGammaVal", "r");
    if (infile) {
        double sgi_gamma;

        fgets(tmpline, 80, infile);
        fclose(infile);
        sgi_gamma = atof(tmpline);
        if (sgi_gamma > 0.0)
            LUT_exponent = 1.0 / sgi_gamma;
    }
#elif defined(Macintosh)
    LUT_exponent = 1.8 / 2.61;
      
                                                         
                                        
       
#else
    LUT_exponent = 1.0;    
#endif

     
    default_display_exponent = LUT_exponent * CRT_exponent;


                                                                             
                                                                              
                                                                          
                                                  

    if ((p = getenv("SCREEN_GAMMA")) != NULL)
        display_exponent = atof(p);
    else
        display_exponent = default_display_exponent;


                                                                             
                                                                          
                                       

    argv[argc++] = PROGNAME;
    p = cmd;
    for (;;) {
        if (*p == ' ')
            while (*++p == ' ')
                ;
         
        if (*p == '\0')
            break;     
        argv[argc++] = q = p;
        while (*q && *q != ' ')
            ++q;
         
        if (*q == '\0')
            break;     
        *q = '\0';     
        p = q + 1;
    }
    argv[argc] = NULL;    


     

    while (*++argv && !error) {
        if (!strncmp(*argv, "-gamma", 2)) {
            if (!*++argv)
                ++error;
            else {
                display_exponent = atof(*argv);
                if (display_exponent <= 0.0)
                    ++error;
            }
        } else if (!strncmp(*argv, "-bgcolor", 2)) {
            if (!*++argv)
                ++error;
            else {
                bgstr = *argv;
                if (strlen(bgstr) != 7 || bgstr[0] != '#')
                    ++error;
                else
                    have_bg = TRUE;
            }
        } else {
            if (**argv != '-') {
                filename = *argv;
                if (argv[1])    
                    ++error;
            } else
                ++error;    
        }
    }

    if (!filename)
        ++error;


     

    if (error) {
#ifndef __CYGWIN__
        int ch;
#endif

        fprintf(stderr, "\n%s %s:  %s\n\n", PROGNAME, VERSION, appname);
        readpng_version_info();
        fprintf(stderr, "\n"
          "Usage:  %s [-gamma exp] [-bgcolor bg] file.png\n"
          "    exp \ttransfer-function exponent (``gamma'') of the display\n"
          "\t\t  system in floating-point format (e.g., ``%.1f''); equal\n"
          "\t\t  to the product of the lookup-table exponent (varies)\n"
          "\t\t  and the CRT exponent (usually 2.2); must be positive\n"
          "    bg  \tdesired background color in 7-character hex RGB format\n"
          "\t\t  (e.g., ``#ff7700'' for orange:  same as HTML colors);\n"
          "\t\t  used with transparent images\n"
          "\nPress Q, Esc or mouse button 1 after image is displayed to quit.\n"
#ifndef __CYGWIN__
          "Press Q or Esc to quit this usage screen.\n"
#endif
          "\n", PROGNAME, default_display_exponent);
#ifndef __CYGWIN__
        do
            ch = _getch();
        while (ch != 'q' && ch != 'Q' && ch != 0x1B);
#endif
        exit(1);
    }


    if (!(infile = fopen(filename, "rb"))) {
        fprintf(stderr, PROGNAME ":  can't open PNG file [%s]\n", filename);
        ++error;
    } else {
        if ((rc = readpng_init(infile, &image_width, &image_height)) != 0) {
            switch (rc) {
                case 1:
                    fprintf(stderr, PROGNAME
                      ":  [%s] is not a PNG file: incorrect signature\n",
                      filename);
                    break;
                case 2:
                    fprintf(stderr, PROGNAME
                      ":  [%s] has bad IHDR (libpng longjmp)\n", filename);
                    break;
                case 4:
                    fprintf(stderr, PROGNAME ":  insufficient memory\n");
                    break;
                default:
                    fprintf(stderr, PROGNAME
                      ":  unknown readpng_init() error\n");
                    break;
            }
            ++error;
        }
        if (error)
            fclose(infile);
    }


    if (error) {
#ifndef __CYGWIN__
        int ch;
#endif

        fprintf(stderr, PROGNAME ":  aborting.\n");
#ifndef __CYGWIN__
        do
            ch = _getch();
        while (ch != 'q' && ch != 'Q' && ch != 0x1B);
#endif
        exit(2);
    } else {
        fprintf(stderr, "\n%s %s:  %s\n", PROGNAME, VERSION, appname);
#ifndef __CYGWIN__
        fprintf(stderr,
          "\n   [console window:  closing this window will terminate %s]\n\n",
          PROGNAME);
#endif
    }


     

    alen = strlen(appname);
    flen = strlen(filename);
    if (alen + flen + 3 > 1023)
        sprintf(titlebar, "%s:  ...%s", appname, filename+(alen+flen+6-1023));
    else
        sprintf(titlebar, "%s:  %s", appname, filename);


                                                                         
                                                                         
                              

    if (have_bg) {
        unsigned r, g, b;    

        sscanf(bgstr+1, "%2x%2x%2x", &r, &g, &b);
        bg_red   = (uch)r;
        bg_green = (uch)g;
        bg_blue  = (uch)b;
    } else if (readpng_get_bgcolor(&bg_red, &bg_green, &bg_blue) > 1) {
        readpng_cleanup(TRUE);
        fprintf(stderr, PROGNAME
          ":  libpng error while checking for background color\n");
        exit(2);
    }


                                                                             
                                   

    if (rpng_win_create_window(hInst, showmode))
        exit(2);


     

    Trace((stderr, "calling readpng_get_image()\n"))
    image_data = readpng_get_image(display_exponent, &image_channels,
      &image_rowbytes);
    Trace((stderr, "done with readpng_get_image()\n"))


                                                                           
                           

    readpng_cleanup(FALSE);
    fclose(infile);

    if (!image_data) {
        fprintf(stderr, PROGNAME ":  unable to decode PNG image\n");
        exit(3);
    }


     

    Trace((stderr, "calling rpng_win_display_image()\n"))
    if (rpng_win_display_image()) {
        free(image_data);
        exit(4);
    }
    Trace((stderr, "done with rpng_win_display_image()\n"))


     

    printf(
#ifndef __CYGWIN__
      "Done.  Press Q, Esc or mouse button 1 (within image window) to quit.\n"
#else
      "Done.  Press mouse button 1 (within image window) to quit.\n"
#endif
    );
    fflush(stdout);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


     

    rpng_win_cleanup();

    return msg.wParam;
}





static int rpng_win_create_window(HINSTANCE hInst, int showmode)
{
    uch *dest;
    int extra_width, extra_height;
    ulg i, j;
    WNDCLASSEX wndclass;


                                                                             
                                                                           
                                      
                                                                               

    wimage_rowbytes = ((3*image_width + 3L) >> 2) << 2;

     
    if (image_height > ((size_t)(-1))/wimage_rowbytes) {
        fprintf(stderr, PROGNAME ":  image_data buffer would be too large\n");
        return 4;    
    }

    if (!(dib = (uch *)malloc(sizeof(BITMAPINFOHEADER) +
                              wimage_rowbytes*image_height)))
    {
        return 4;    
    }

                                                                             
                                                                           
                                                                            
                                                                           
                                                                
                                                                               

    memset(dib, 0, sizeof(BITMAPINFOHEADER));
    bmih = (BITMAPINFOHEADER *)dib;
    bmih->biSize = sizeof(BITMAPINFOHEADER);
    bmih->biWidth = image_width;
    bmih->biHeight = -((long)image_height);
    bmih->biPlanes = 1;
    bmih->biBitCount = 24;
    bmih->biCompression = 0;
    wimage_data = dib + sizeof(BITMAPINFOHEADER);

                                                                             
                                                                       
                                                                               

    for (j = 0;  j < image_height;  ++j) {
        dest = wimage_data + j*wimage_rowbytes;
        for (i = image_width;  i > 0;  --i) {
            *dest++ = bg_blue;
            *dest++ = bg_green;
            *dest++ = bg_red;
        }
    }

                                                                             
                              
                                                                               

    memset(&wndclass, 0, sizeof(wndclass));

    wndclass.cbSize = sizeof(wndclass);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = rpng_win_wndproc;
    wndclass.hInstance = hInst;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = progname;
    wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wndclass);

                                                                             
                               
                                                                               

    extra_width  = 2*(GetSystemMetrics(SM_CXBORDER) +
                      GetSystemMetrics(SM_CXDLGFRAME));
    extra_height = 2*(GetSystemMetrics(SM_CYBORDER) +
                      GetSystemMetrics(SM_CYDLGFRAME)) +
                      GetSystemMetrics(SM_CYCAPTION);

    global_hwnd = CreateWindow(progname, titlebar, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, image_width+extra_width,
      image_height+extra_height, NULL, NULL, hInst, NULL);

    ShowWindow(global_hwnd, showmode);
    UpdateWindow(global_hwnd);

    return 0;

}  





static int rpng_win_display_image()
{
    uch *src, *dest;
    uch r, g, b, a;
    ulg i, row, lastrow;
    RECT rect;


    Trace((stderr, "beginning display loop (image_channels == %d)\n",
      image_channels))
    Trace((stderr, "(width = %ld, rowbytes = %ld, wimage_rowbytes = %d)\n",
      image_width, image_rowbytes, wimage_rowbytes))


                                                                             
                                                                          
                                                                           
              
                                                                               

    for (lastrow = row = 0;  row < image_height;  ++row) {
        src = image_data + row*image_rowbytes;
        dest = wimage_data + row*wimage_rowbytes;
        if (image_channels == 3) {
            for (i = image_width;  i > 0;  --i) {
                r = *src++;
                g = *src++;
                b = *src++;
                *dest++ = b;
                *dest++ = g;    
                *dest++ = r;
            }
        } else   {
            for (i = image_width;  i > 0;  --i) {
                r = *src++;
                g = *src++;
                b = *src++;
                a = *src++;
                if (a == 255) {
                    *dest++ = b;
                    *dest++ = g;
                    *dest++ = r;
                } else if (a == 0) {
                    *dest++ = bg_blue;
                    *dest++ = bg_green;
                    *dest++ = bg_red;
                } else {
                                                                    
                                                                    
                                                                   
                                                              
                    alpha_composite(*dest++, b, a, bg_blue);
                    alpha_composite(*dest++, g, a, bg_green);
                    alpha_composite(*dest++, r, a, bg_red);
                }
            }
        }
         
        if (((row+1) & 0xf) == 0) {
            rect.left = 0L;
            rect.top = (LONG)lastrow;
            rect.right = (LONG)image_width;       
            rect.bottom = (LONG)lastrow + 16L;    
            InvalidateRect(global_hwnd, &rect, FALSE);
            UpdateWindow(global_hwnd);      
            lastrow = row + 1;
        }
    }

    Trace((stderr, "calling final image-flush routine\n"))
    if (lastrow < image_height) {
        rect.left = 0L;
        rect.top = (LONG)lastrow;
        rect.right = (LONG)image_width;       
        rect.bottom = (LONG)image_height;     
        InvalidateRect(global_hwnd, &rect, FALSE);
        UpdateWindow(global_hwnd);      
    }

  
                                                                         
                                            
                              
   

    return 0;
}





static void rpng_win_cleanup()
{
    if (image_data) {
        free(image_data);
        image_data = NULL;
    }

    if (dib) {
        free(dib);
        dib = NULL;
    }
}





LRESULT CALLBACK rpng_win_wndproc(HWND hwnd, UINT iMsg, WPARAM wP, LPARAM lP)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    int rc;

    switch (iMsg) {
        case WM_CREATE:
             
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
                     
            rc = StretchDIBits(hdc, 0, 0, image_width, image_height,
                     
                                    0, 0, image_width, image_height,
                                    wimage_data, (BITMAPINFO *)bmih,
                     
                                    0, SRCCOPY);
            EndPaint(hwnd, &ps);
            return 0;

         
        case WM_CHAR:
            switch (wP) {       
                case 'q':
                case 'Q':
                case 0x1B:      
                    PostQuitMessage(0);
            }
            return 0;

        case WM_LBUTTONDOWN:    
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, iMsg, wP, lP);
}
