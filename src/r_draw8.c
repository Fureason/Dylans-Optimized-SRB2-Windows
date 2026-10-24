// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------












                                      
                                                                  
  
void R_DrawColumn_8(void)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif


	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		register const UINT8 *source = dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		if (dc_texheight & heightmask)    
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				*dest = colormap[source[frac>>FRACBITS]];
				dest += vid.width;


				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
		}
	}
}

   
                                                  
  
                                                                             
                                                                               
                                                                             
                       
   
void R_DrawSkyColumn_8(void)
{
	INT32 count = dc_yh - dc_yl;
	UINT8 *restrict dest;
	const UINT8 *restrict source = dc_source;
	const lighttable_t *restrict colormap = dc_colormap;
	intptr_t frac;
	const intptr_t fracstep = dc_iscale;
	const intptr_t texheight = dc_texheight;

	if (count < 0 || texheight <= 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];
	frac = dc_texturemid
		+ FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	do
	{
		intptr_t index = frac >> FRACBITS;

		if (index < 0)
			index = 0;
		else if (index >= texheight)
			index = texheight - 1;

		*dest = colormap[source[index]];
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

                                             
                                                                                                    
  
void R_DrawColumnClamped_8(void)
{
	INT32 count;
	UINT8 *dest;
	fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif


	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		const UINT8 *source = dc_source;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight-1;
		INT32 idx;
		if (dc_texheight & heightmask)    
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;


				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
			}
		}
	}
}

void R_Draw2sMultiPatchColumn_8(void)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif


	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		register const UINT8 *source = dc_source;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
		if (dc_texheight & heightmask)    
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];

				dest += vid.width;


				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
			}
		}
	}
}

void R_Draw2sMultiPatchTranslucentColumn_8(void)
{
	INT32 count;
	register UINT8 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif


	dest = &topleft[dc_yl*vid.width + dc_x];

	count++;


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		register const UINT8 *source = dc_source;
		register const UINT8 *transmap = dc_transmap;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
		if (dc_texheight & heightmask)    
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));

				dest += vid.width;


				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = *(transmap + (colormap[val]<<8) + (*dest));
			}
		}
	}
}

                                           
                                                                  
  
void R_DrawShadeColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;


	if ((dc_yl < 0) || (dc_x >= vid.width))
		return;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawShadeColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);


	do
	{
		*dest = colormaps[(dc_source[frac>>FRACBITS] <<8) + (*dest)];
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

                                                 
                                                                     
                                                                     
                     
  
void R_DrawTranslucentColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		register const UINT8 *source = dc_source;
		register const UINT8 *transmap = dc_transmap;
		register const lighttable_t *colormap = dc_colormap;
		register INT32 heightmask = dc_texheight - 1;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				*dest = *(transmap + (colormap[source[frac>>FRACBITS]]<<8) + (*dest));
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = *(transmap + (colormap[source[(frac>>FRACBITS)&heightmask]]<<8) + (*dest));
		}
	}
}

                                                        
                                                                                                               
  
void R_DrawTranslucentColumnClamped_8(void)
{
	INT32 count;
	UINT8 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0)  
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumnClamped_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		const UINT8 *source = dc_source;
		const UINT8 *transmap = dc_transmap;
		const lighttable_t *colormap = dc_colormap;
		INT32 heightmask = dc_texheight - 1;
		INT32 idx;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{



				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = *(transmap + (colormap[source[idx]]<<8) + (*dest));
			}
		}
	}
}






void R_DrawDropShadowColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;

	count = dc_yh - dc_yl + 1;

	if (count <= 0)  
		return;

	dest = &topleft[dc_yl*vid.width + dc_x];

	{
#define DSCOLOR 31  
		register const UINT8 *transmap_offset = dc_transmap + (dc_colormap[DSCOLOR] << 8);
#undef DSCOLOR
		while ((count -= 2) >= 0)
		{
			*dest = *(transmap_offset + (*dest));
			dest += vid.width;
			*dest = *(transmap_offset + (*dest));
			dest += vid.width;
		}
		if (count & 1)
			*dest = *(transmap_offset + (*dest));
	}
}

                                                           
                                                                                    
                             
  
void R_DrawTranslatedTranslucentColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0)  
		return;

	dest = &topleft[dc_yl*vid.width + dc_x];


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);



	{
		register INT32 heightmask = dc_texheight - 1;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{




				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]<<8) + (*dest));

				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0)  
			{
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = *(dc_transmap + (dc_colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]]<<8) + (*dest));
		}
	}
}

                                                
                                                                     

                                    
  
void R_DrawTranslatedColumn_8(void)
{
	register INT32 count;
	register UINT8 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &topleft[dc_yl*vid.width + dc_x];


	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);


	do
	{





		*dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];

		dest += vid.width;

		frac += fracstep;
	} while (count--);
}





#define SPANSIZE 16
#define INVSPAN 0.0625f

                                    
                       
  
void R_DrawSpan_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{



		dest[0] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		*dest++ = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;
	}
}

                                          
                         
  
void R_DrawTiltedSpan_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;


#if 0	 

	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

                                                     
                                     
  
void R_DrawTiltedTranslucentSpan_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;


#if 0	 

	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dest);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

                                               
                                              
  
void R_DrawTiltedWaterSpan_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;


#if 0	 

	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				*dest = *(ds_transmap + (colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]] << 8) + *dsrc++);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;


#if 0	 

	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];

		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedTranslucentSplat_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = ds_source;


#if 0	 

	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);

		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val != TRANSPARENTPIXEL)
					*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

                                     
                                                      
  
void R_DrawSplat_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{






		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[7] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

                                                
                                             
  
void R_DrawTranslucentSplat_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{



		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[0] = *(ds_transmap + (colormap[val] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[1] = *(ds_transmap + (colormap[val] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[2] = *(ds_transmap + (colormap[val] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[3] = *(ds_transmap + (colormap[val] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[4] = *(ds_transmap + (colormap[val] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[5] = *(ds_transmap + (colormap[val] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[6] = *(ds_transmap + (colormap[val] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[7] = *(ds_transmap + (colormap[val] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = *(ds_transmap + (colormap[val] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

                                           
                                                
  
void R_DrawFloorSprite_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{



		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[0] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[1] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[2] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[3] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[4] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[5] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[6] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[7] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			*dest = colormap[translation[val & 0xFF]];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

                                                     
                                                   
  
void R_DrawTranslucentFloorSprite_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{



		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[0] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[1] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[2] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[3] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[4] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[5] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[6] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[7] = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

                                                 
                             
  
void R_DrawTiltedFloorSprite_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

                                                            
                                           
  
void R_DrawTiltedTranslucentFloorSprite_8(void)
{

	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	UINT8 *colormap;
	UINT8 *translation;
	UINT8 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &topleft[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = ds_colormap;
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;

	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val & 0xFF00)
					*dest = *(ds_transmap + (colormap[translation[val & 0xFF]] << 8) + *dest);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

                                               
                                         
  
void R_DrawTranslucentSpan_8 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;








	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{



		dest[0] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[0]);
		xposition += xstep;
		yposition += ystep;

		dest[1] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[1]);
		xposition += xstep;
		yposition += ystep;

		dest[2] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[2]);
		xposition += xstep;
		yposition += ystep;

		dest[3] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[3]);
		xposition += xstep;
		yposition += ystep;

		dest[4] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[4]);
		xposition += xstep;
		yposition += ystep;

		dest[5] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[5]);
		xposition += xstep;
		yposition += ystep;

		dest[6] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[6]);
		xposition += xstep;
		yposition += ystep;

		dest[7] = *(ds_transmap + (colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]] << 8) + dest[7]);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		*dest = *(ds_transmap + (colormap[source[val]] << 8) + *dest);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawWaterSpan_8(void)
{
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	UINT8 *colormap;
	UINT8 *dest;
	UINT8 *dsrc;

	size_t count;








	xposition = ds_xfrac << nflatshiftup; yposition = (ds_yfrac + ds_waterofs) << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = ds_colormap;
	dest = &topleft[ds_y*vid.width + ds_x1];
	dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{



		dest[0] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest++ = colormap[*(ds_transmap + (source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)] << 8) + *dsrc++)];
		xposition += xstep;
		yposition += ystep;
	}
}

                                       
                                    
  
void R_DrawFogSpan_8(void)
{
	UINT8 *colormap;
	UINT8 *dest;

	size_t count;

	colormap = ds_colormap;
	dest = &topleft[ds_y *vid.width + ds_x1];

	count = ds_x2 - ds_x1 + 1;

	while (count >= 4)
	{
		dest[0] = colormap[dest[0]];
		dest[1] = colormap[dest[1]];
		dest[2] = colormap[dest[2]];
		dest[3] = colormap[dest[3]];

		dest += 4;
		count -= 4;
	}

	while (count--)
	{
		*dest = colormap[*dest];
		dest++;
	}
}

                                             
                                  
  
void R_DrawTiltedFogSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = colormap[*dest];
		dest++;
	} while (--width >= 0);
}

                                              
                          
  
void R_DrawSolidColorSpan_8(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT8 source = ds_colormap[ds_source[0]];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	memset(dest, source, count);
}

                                                   
                                      
  
void R_DrawTransSolidColorSpan_8(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT8 source = ds_colormap[ds_source[0]];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = *(ds_transmap + (source << 8) + *dest);
		dest++;
	}
}

                                                    
                                 
  
void R_DrawTiltedSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest++ = colormap[source];
	} while (--width >= 0);
}

                                                         
                                                 
  
void R_DrawTiltedTransSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest = *(ds_transmap + (colormap[source] << 8) + *dest);
		dest++;
	} while (--width >= 0);
}

                                                   
                                
  
void R_DrawWaterSolidColorSpan_8(void)
{
	UINT8 source = ds_source[0];
	UINT8 *colormap = ds_colormap;
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];
	UINT8 *dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	size_t count = (ds_x2 - ds_x1 + 1);
	const UINT8 *deststop = screens[0] + vid.rowbytes * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = colormap[*(ds_transmap + (source << 8) + *dsrc++)];
		dest++;
	}
}

                                                         
                                       
  
void R_DrawTiltedWaterSolidColorSpan_8(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT8 *dest = &topleft[ds_y*vid.width + ds_x1];
	UINT8 *dsrc = screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	R_CalcSlopeLight();

	do
	{
		UINT8 *colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		*dest++ = *(ds_transmap + (colormap[source] << 8) + *dsrc++);
	} while (--width >= 0);
}

                                         
          
  
void R_DrawFogColumn_8(void)
{
	INT32 count;
	UINT8 *dest;

	count = dc_yh - dc_yl;


	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawFogColumn_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif


	dest = &topleft[dc_yl*vid.width + dc_x];


	do
	{

		*dest = dc_colormap[*dest];
		dest += vid.width;
	} while (count--);
}

                                           
                                                  

                                                                             
  
void R_DrawColumnShadowed_8(void)
{
	INT32 count, realyh, i, height, bheight = 0, solid = 0;

	realyh = dc_yh;

	count = dc_yh - dc_yl;


	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawColumnShadowed_8: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif


	for (i = 0; i < dc_numlights; i++)
	{


		solid = dc_lightlist[i].flags & FOF_CUTSOLIDS;

		height = dc_lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
		{
			bheight = dc_lightlist[i].botheight >> LIGHTSCALESHIFT;
			if (bheight < height)
			{




				INT32 temp = height;
				height = bheight;
				bheight = temp;
			}
		}
		if (height <= dc_yl)
		{
			dc_colormap = dc_lightlist[i].rcolormap;
			if (solid && dc_yl < bheight)
				dc_yl = bheight;
			continue;
		}

		dc_yh = height;

		if (dc_yh > realyh)
			dc_yh = realyh;
		(colfuncs[BASEDRAWFUNC])();		 
		if (solid)
			dc_yl = bheight;
		else
			dc_yl = dc_yh + 1;

		dc_colormap = dc_lightlist[i].rcolormap;
	}
	dc_yh = realyh;
	if (dc_yl <= realyh)
		(colfuncs[BASEDRAWFUNC])();		 
}
