// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------



#ifndef __HWR_MAIN_H__
#define __HWR_MAIN_H__

#include "hw_data.h"
#include "hw_defs.h"

#include "../am_map.h"
#include "../d_player.h"
#include "../r_defs.h"

#include "../m_perfstats.h"


void HWR_Startup(void);
void HWR_Switch(void);
void HWR_Shutdown(void);

void HWR_drawAMline(const fline_t *fl, INT32 color);
void HWR_BeginAMLineBatch(void);
void HWR_EndAMLineBatch(void);
void HWR_FadeScreenMenuBack(UINT16 color, UINT8 strength);
void HWR_DrawConsoleBack(UINT32 color, INT32 height);
void HWR_DrawTutorialBack(UINT32 color, INT32 boxheight);
void HWR_RenderSkyboxView(INT32 viewnumber, player_t *player);
void HWR_RenderPlayerView(INT32 viewnumber, player_t *player);
void HWR_ClearSkyDome(void);
void HWR_BuildSkyDome(void);
void HWR_DrawFlatFill(INT32 x, INT32 y, INT32 w, INT32 h, lumpnum_t flatlumpnum);
void HWR_SetViewSize(void);
void HWR_DrawStretchyFixedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap);
void HWR_DrawCroppedPatch(patch_t *gpatch, fixed_t x, fixed_t y, fixed_t pscale, fixed_t vscale, INT32 option, const UINT8 *colormap, fixed_t sx, fixed_t sy, fixed_t w, fixed_t h);
void HWR_MakePatch(patch_t *patch, GLPatch_t *grPatch, GLMipmap_t *grMipmap, boolean makebitmap);
void HWR_CreatePlanePolygons(INT32 bspnum);
void HWR_CreateStaticLightmaps(INT32 bspnum);
void HWR_DrawFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color);
void HWR_DrawFadeFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT16 actualcolor, UINT8 strength);
void HWR_DrawConsoleFill(INT32 x, INT32 y, INT32 w, INT32 h, INT32 color, UINT32 actualcolor);	 

UINT8 *HWR_GetScreenshot(void);
void HWR_SetMultisample(boolean enabled);
boolean HWR_Screenshot(const char *pathname);

void HWR_AddCommands(void);
void transform(float *cx, float *cy, float *cz);
UINT32 HWR_GetTextureUsed(void);
void HWR_DoPostProcessor(player_t *player);
void HWR_StartScreenWipe(void);
void HWR_EndScreenWipe(void);
void HWR_DrawIntermissionBG(void);
void HWR_DoWipe(UINT8 wipenum, UINT8 scrnnum);
void HWR_MakeScreenFinalTexture(void);
void HWR_DrawScreenFinalTexture(int width, int height);


boolean HWR_UseShader(void);
void HWR_Lighting(FSurfaceInfo *Surface, INT32 light_level, extracolormap_t *colormap);
UINT8 HWR_FogBlockAlpha(INT32 light, extracolormap_t *colormap);  

UINT8 HWR_GetTranstableAlpha(INT32 transtablenum);
FBITFIELD HWR_GetBlendModeFlag(INT32 style);
FBITFIELD HWR_SurfaceBlend(INT32 style, INT32 transtablenum, FSurfaceInfo *pSurf);
FBITFIELD HWR_TranstableToAlpha(INT32 transtablenum, FSurfaceInfo *pSurf);

boolean HWR_ShouldUsePaletteRendering(void);

extern CV_PossibleValue_t glanisotropicmode_cons_t[];

#ifdef ALAM_LIGHTING
extern consvar_t cv_gldynamiclighting;
extern consvar_t cv_glstaticlighting;
extern consvar_t cv_glcoronas;
extern consvar_t cv_glcoronasize;
#endif

extern consvar_t cv_glshaders, cv_glallowshaders;
extern consvar_t cv_glmodels;
extern consvar_t cv_glmodelinterpolation;
extern consvar_t cv_glmodellighting;
extern consvar_t cv_glfiltermode;
extern consvar_t cv_glspritefiltermode;
extern consvar_t cv_glanisotropicmode;
extern consvar_t cv_glsolvetjoin;
extern consvar_t cv_glshearing;
extern consvar_t cv_glspritebillboarding;
extern consvar_t cv_glskydome;
extern consvar_t cv_glfakecontrast;
extern consvar_t cv_glvisuportals;
extern consvar_t cv_glslopecontrast;
extern consvar_t cv_glbatching;
extern consvar_t cv_glretaintextures;
extern consvar_t cv_gltruecolorpngs;
extern consvar_t cv_glpaletterendering;
extern consvar_t cv_glpalettedepth;

extern consvar_t cv_glwireframe;


extern fixed_t *hwbbox;
extern FTransform atransform;
extern float gl_viewsin, gl_viewcos;


extern ps_metric_t ps_hw_skyboxtime;
extern ps_metric_t ps_hw_portaltime;
extern ps_metric_t ps_hw_numportals;
extern ps_metric_t ps_hw_nodesorttime;
extern ps_metric_t ps_hw_nodedrawtime;
extern ps_metric_t ps_hw_spritesorttime;
extern ps_metric_t ps_hw_spritedrawtime;
extern ps_metric_t ps_hw_textureuploadtime;
extern ps_metric_t ps_hw_textureuploads;
extern ps_metric_t ps_hw_batchedsprites;


extern ps_metric_t ps_hw_numpolys;
extern ps_metric_t ps_hw_numverts;
extern ps_metric_t ps_hw_numcalls;
extern ps_metric_t ps_hw_numshaders;
extern ps_metric_t ps_hw_numtextures;
extern ps_metric_t ps_hw_numpolyflags;
extern ps_metric_t ps_hw_numcolors;
extern ps_metric_t ps_hw_batchsorttime;
extern ps_metric_t ps_hw_batchdrawtime;

extern boolean gl_init;
extern boolean gl_maploaded;
extern boolean gl_maptexturesloaded;
extern boolean gl_sessioncommandsadded;
extern boolean gl_shadersavailable;

#endif
