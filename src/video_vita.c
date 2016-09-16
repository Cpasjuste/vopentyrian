/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "keyboard.h"
#include "opentyr.h"
#include "palette.h"
#include "video.h"
#include "video_scale.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <psp2/power.h>
#include <vita2d.h>
#define SCR_W 960
#define SCR_H 544

#undef DIRECTDRAW

const char* scaling_mode_names[ScalingMode_MAX] = {
	"Integer",
	"Fit",
	"FullScreen",
};

int fullscreen_display;
ScalingMode scaling_mode = SCALE_FIT;
static SDL_Rect last_output_rect = { 0, 0, vga_width, vga_height };

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;
SDL_PixelFormat* main_window_tex_format;
SDL_Window* main_window; // "used" by keyboard.c

void show_splash();
vita2d_texture *vitaTexture;
Uint8 *vitaData;
#ifdef DIRECTDRAW
void *sdlpixels;
#endif

// fps
double  t, t0, fps;
int frames=0;
void fpsinit() {
	t0 = (double)SDL_GetTicks()/1000.0;
}
void fpsrun() {
	t = (double)SDL_GetTicks()/1000.0;
	if( (t-t0) > 1.0 || frames == 0 ) {
		fps = (double)frames / (t-t0);
		printf( "%.1f FPS\n", fps );
		t0 = t;
		frames = 0;
	}
	++frames;
}
// fps

void c8_16(SDL_Surface *src_surface, Uint8 *dst)
{
	Uint8 *src = src_surface->pixels, *src_temp;
	Uint8 *dst_temp;

	int src_pitch = src_surface->pitch;
	int dst_pitch = 320*2;

	const int dst_Bpp = 2;  
	int dst_width = 320;
	
	const int height = vga_height, 
	          width = vga_width, 
	          scale = 1;
	
	for (int y = height; y > 0; y--)
	{
		src_temp = src;
		dst_temp = dst;
		
		for (int x = width; x > 0; x--)
		{
			for (int z = scale; z > 0; z--)
			{
				*(Uint16 *)dst = rgb_palette[*src];
				dst += dst_Bpp;
			}
			src++;
		}
		
		src = src_temp + src_pitch;
		dst = dst_temp + dst_pitch;
		
		for (int z = scale; z > 1; z--)
		{
			memcpy(dst, dst_temp, dst_width * dst_Bpp);
			dst += dst_pitch;
		}
	}
}

void init_video( void )
{
	// Create the software surfaces that the game renders to. These are all 320x200x8 regardless
	// of the window size or monitor resolution.
	VGAScreen = VGAScreenSeg = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
	VGAScreen2 = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
	game_screen = SDL_CreateRGBSurface(0, vga_width, vga_height, 8, 0, 0, 0, 0);
	scaler = 0;

	// get max speed :/
	scePowerSetGpuClockFrequency(222);
	scePowerSetArmClockFrequency(444);
	// use vita texture for rendering
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
#ifdef DIRECTDRAW
	vitaTexture = vita2d_create_empty_texture_format(320, 200, SCE_GXM_TEXTURE_FORMAT_U8_RRRR);
#else
	vitaTexture = vita2d_create_empty_texture_format(320, 200, SCE_GXM_TEXTURE_FORMAT_R5G6B5);
#endif
	vitaData = (Uint8 *)vita2d_texture_get_datap(vitaTexture);
#ifdef DIRECTDRAW
	sdlpixels = VGAScreenSeg->pixels;
	VGAScreenSeg->pixels = (void*)vitaData;
#endif
	
	// The game code writes to surface->pixels directly without locking, so make sure that we
	// indeed created software surfaces that support this.
	assert(!SDL_MUSTLOCK(VGAScreen));
	assert(!SDL_MUSTLOCK(VGAScreen2));
	assert(!SDL_MUSTLOCK(game_screen));

	JE_clr256(VGAScreen);

#ifdef DIRECTDRAW
	main_window_tex_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB444);
#else
	main_window_tex_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
#endif

#ifdef VDEBUG
	fpsinit();
#endif

	init_scaler(1);
	
	show_splash();
}

void JE_clr256( SDL_Surface * screen)
{
	//SDL_FillRect(screen, NULL, 0);
}

void JE_showVGA( void ) 
{
	scale_and_flip(VGAScreen); 
}

bool init_scaler( unsigned int new_scaler ) 
{ 
	scaler = new_scaler;
	return true; 
}

bool set_scaling_mode_by_name( const char* name ) 
{
	return true; 
}

void scale_and_flip( SDL_Surface *src_surface )
{
#ifndef DIRECTDRAW
	c8_16(src_surface, vitaData);
#endif

	int x = 0, y = 0, w = 320, h = 200;
	float sx = 1, sy = 1;
	float ratio = (float)320/(float)200;
	
	if(scaling_mode == SCALE_FIT) {
		h = SCR_H; w = (float)h*ratio;
		x = (SCR_W-w)/2; y = 0;
	} else if(scaling_mode == SCALE_FULLSCREEN) {
		h = SCR_H; w = SCR_W;
		x = 0; y = 0;
	} else if(scaling_mode == SCALE_INTEGER) {
		h = scalers[scaler].height; w = scalers[scaler].width;
		x = (SCR_W-w)/2; y = (SCR_H-h)/2;
	}
	sx = (float)w/(float)320;
	sy = (float)h/(float)200;

	vita2d_start_drawing();
	vita2d_clear_screen();
	vita2d_draw_texture_scale(vitaTexture, x, y, sx, sy);
	vita2d_end_drawing();
	vita2d_swap_buffers();

#ifdef VDEBUG
	//fpsrun();
#endif         
	//last_output_rect = dst_rect;
}

void deinit_video( void )
{
#ifdef DIRECTDRAW
	VGAScreenSeg->pixels = sdlpixels;
#endif
	vita2d_free_texture(vitaTexture);
	
	SDL_FreeSurface(VGAScreenSeg);
	SDL_FreeSurface(VGAScreen2);
	SDL_FreeSurface(game_screen);
}

void video_on_win_resize() {}
void toggle_fullscreen( void ) {}
void reinit_fullscreen( int new_display ) {}

/** Converts the given point from the game screen coordinates to the window
 * coordinates, after scaling. */
void map_screen_to_window_pos(int* inout_x, int* inout_y) {
	*inout_x = (*inout_x * last_output_rect.w / VGAScreen->w) + last_output_rect.x;
	*inout_y = (*inout_y * last_output_rect.h / VGAScreen->h) + last_output_rect.y;
}

/** Converts the given point from window coordinates (after scaling) to game
 * screen coordinates. */
void map_window_to_screen_pos(int* inout_x, int* inout_y) {
	*inout_x = (*inout_x - last_output_rect.x) * VGAScreen->w / last_output_rect.w;
	*inout_y = (*inout_y - last_output_rect.y) * VGAScreen->h / last_output_rect.h;
}

void show_splash() {
	
	int alpha = 1;
	int done = 0;
	
	vita2d_texture *splash = 
		vita2d_load_PNG_file("app0:data/gekihen-splash.png");
		
	if(splash == NULL)
		return;
		
	while(true) {
		
		if(alpha < 255 && !done)
			alpha+=2;
		else {
			if(!done) {
				SDL_Delay(1000*3);
			}
			done = 1;
			alpha-=2;
		}
		
		if(alpha <= 0)
			break;
		
		vita2d_start_drawing();
		vita2d_clear_screen();
		vita2d_draw_texture_tint(splash, 0, 0, RGBA8(255,255,255,alpha));
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	
	//vita2d_free_texture(splash);
}
