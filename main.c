/*

    Dodgin' Diamond 2, a shot'em up arcade
    Copyright (C) 2003,2004 Juan J. Martinez <jjm@usebox.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 2 as
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <pspsdk.h>
#include <psppower.h>

#include<stdlib.h>
#include<stdio.h>
#include"main.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include"menu.h"

/* Define the module info section */
PSP_MODULE_INFO("dd2_psp", 0, 1, 1);

/* Define the main thread's attribute value (optional) */
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define APP_NAME	"Dodgin' Diamond ]["

#define FPS	60

#include "control.h"
#include "SDL_plus.h"
#include "engine.h"
#include "cfg.h"

#ifdef WIN32
static const char COPYRIGHT[]="Dodgin' Diamond 2 - Copyright (c) 2003,2004 Juan J. Martinez <jjm@usebox.net> This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License Version 2 as published by the Free Software Foundation (www.fsf.org).";
#endif

#ifdef GP32
   #include <x_gp32.h>
#endif
 
SDL_Surface *screen, *gfx;
extern pDesc player[2];
SDL_Joystick *joy[2]={ NULL, NULL };
SDL_Event event;
Uint32 tick, ntick;
float scroll=0,scroll2=0;

bool pause;
Uint32 pause_tick;

extern bool boss;

cfg conf;
score hiscore[10];

Mix_Chunk *efx[8]={ NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
Mix_Music *bgm=NULL, *bgm_boss=NULL;
int sound;
bool done;

/* load all the sound stuff */
void
soundLoad()
{
    int i;
    char buffer[512];

	sprintf(buffer,"%s/bgm1.xm","data");
	bgm=Mix_LoadMUS(buffer);
	if(!bgm)
		fprintf(stderr,"Unable load bgm: %s\n", SDL_GetError());

	sprintf(buffer,"%s/bgm2.xm","data");
	bgm_boss=Mix_LoadMUS(buffer);
	if(!bgm_boss)
		fprintf(stderr,"Unable load bgm_boss: %s\n", SDL_GetError());

	for(i=0;i<NUM_EFX;i++) {
		sprintf(buffer,"%s/efx%i.wav","data",i+1);
		efx[i]=Mix_LoadWAV(buffer);
		if(!efx[i]) {
			fprintf(stderr,"Unable load efx: %s\n", SDL_GetError());
		} else
			Mix_VolumeChunk(efx[i],MIX_MAX_VOLUME/2);
	}

}

void
gameLoop()
{
	int afterdeath=0;

	for(done=false,tick=SDL_GetTicks();!done && afterdeath<400;) {

		while(SDL_PollEvent(&event)) {
			if (event.type==SDL_QUIT)
				done=true;

			/* joystick control for the menu */
			if(player[0].joy && joy[0])
			{
				SDL_JoystickUpdate();

				if(SDL_JoystickGetButton(joy[0], 1))
				{
					event.type=SDL_KEYDOWN;
					event.key.keysym.sym=SDLK_SPACE;
				}
			}
			else
				if(player[1].joy && joy[1])
				{
					SDL_JoystickUpdate();

					if(SDL_JoystickGetButton(joy[1], 1))
					{
						event.type=SDL_KEYDOWN;
						event.key.keysym.sym=SDLK_SPACE;
					}
				}

			if(event.type==SDL_KEYDOWN) {
				if(event.key.keysym.sym==SDLK_TAB||event.key.keysym.sym==SDLK_ESCAPE) {
					done=true;
					continue;
				}
				else
				{
					if(event.key.keysym.sym==SDLK_SPACE && pause_tick<SDL_GetTicks())
					{
						writeCString(gfx, screen, 98, 20, "game paused", 0);
						SDL_Flip(screen);
						pause=pause ? false : true;
						pause_tick=SDL_GetTicks()+200;
						continue;
					}
					else
						if(event.key.keysym.sym==SDLK_F12)
							SDL_SaveBMP(screen,"scnshot.bmp");
				}
			}
		}

		/* player control */
		if(player[0].shield) {
			if(joy[0] && player[0].joy)
				control_player_joy(joy[0],&player[0]);
			else
				control_player(&player[0]);
		}

		if(player[1].shield) {
			if(joy[1] && player[1].joy)
				control_player_joy(joy[1],&player[1]);
			else
				control_player(&player[1]);
		}

		if(pause)
			continue;
		
		/* frame rate calculation */
		ntick=SDL_GetTicks();
		if(ntick-tick>=1000/FPS) {
			tick=ntick;

				/* scroll here */
			{
				SDL_Rect a,b;

				if(scroll>0)
					scroll-=0.5;
				else
					scroll=SCREENH;

				b.x=1;
				b.w=SCREENW;
				a.x=0;

				if(!scroll) {
					a.y=0;
					b.y=204;
					b.h=SCREENH;
					SDL_BlitSurface(gfx, &b, screen, &a);
				} else {
					a.y=0;
					b.y=204+(int)scroll;
					b.h=SCREENH-(int)scroll;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll;
					b.y=204;
					b.h=(int)scroll;
					SDL_BlitSurface(gfx, &b, screen, &a);
				}

				/* scroll parallax here */

				if(scroll2>0)
					scroll2-=2;
				else
					scroll2=SCREENH;

				b.x=324;
				b.w=25;
				a.x=0;

				if(!scroll2) {
					a.y=0;
					b.y=204;
					b.h=SCREENH;
					SDL_BlitSurface(gfx, &b, screen, &a);

					b.x=358;
					a.x=SCREENW-25;
					SDL_BlitSurface(gfx, &b, screen, &a);		  		
				} else {
					a.y=0;
					b.y=204+(int)scroll2;
					b.h=SCREENH-(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll2;
					b.y=204;
					b.h=(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);

					b.x=358;
					a.x=SCREENW-25;
					a.y=0;
					b.y=204+(int)scroll2;
					b.h=SCREENH-(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);
					a.y=SCREENH-(int)scroll2;
					b.y=204;
					b.h=(int)scroll2;
					SDL_BlitSurface(gfx, &b, screen, &a);	
				}
			}
			/* enemy here */
			engine_enemy();

			/* fire here */
			engine_fire();

			/* character here */
			if(player[0].shield)
				engine_player(&player[0]);

			if(player[1].shield)
				engine_player(&player[1]);

			if(!(player[0].shield | player[1].shield))
				afterdeath++;
				
			engine_obj();

			engine_vefx();

			/* panel */
			drawPanel(gfx,screen,player);

			SDL_Flip(screen);
		}
	}
}

int
main (int argc, char *argv[])
{
    int i,j,k;
    char buffer[512];
	SDL_VideoInfo *vi;
	unsigned char bpp=8;

#ifdef GP32
	x_gp32_SetCPUSpeed_133();
#endif

	scePowerSetClockFrequency(333,333,166);
 
#ifdef OTHER
	if(argc==2)
		if(argv[1][0]=='-' && argv[1][1]=='v') {
			printf("%s v%s\nCopyright (c) 2003,2004 Juan J. Martinez <jjm@usebox.net>\n", PACKAGE, VERSION);
			printf("This is free software, and you are welcome\nto redistribute it"
                   " under certain conditions; read COPYING for details.\n");
			return 1;
		}

    /* try local configuration */
	sprintf(buffer,"%.500s/.dd2rc",getenv("HOME"));
	if(!loadCFG(buffer,&conf)) {
		/* if there's no local, use global */
		sprintf(buffer,"%s/dd2.cfg","data");
		if(!loadCFG(buffer,&conf))
			fprintf(stderr,"unable to read configuration, using defaults\n");
	}
#else
	sprintf(buffer,"%s/dd2.cfg","data");
	if(!loadCFG(buffer,&conf))
		fprintf(stderr,"unable to read configuration, using defaults : %s\n", buffer);
#endif

	/* read hi-scores */
	sprintf(buffer,"%s/hiscore","data");
	if(!loadScore(buffer,hiscore))
		fprintf(stderr,"unable to read hi-scores, using defaults\n");

	/* don't init sound if it's not needed */
	if(conf.sound!=NO_SOUND)
		i=SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK);
	else
		i=SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK);
	if(i<0) {
		fprintf(stderr,"Unable to init SDL: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);

	sound=SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO;

	/* no sound, 16000, 22050, 44100 */
	if(sound && conf.sound!=NO_SOUND) {
		switch(conf.sound) {
			default:
			case SOUND_HI:
				i=44100;
			break;
			case SOUND_MED:
				i=22050;
			break;
			case SOUND_LOW:
				i=16000;
			break;
		}
		if(Mix_OpenAudio(i, MIX_DEFAULT_FORMAT, 2, 2048)<0) {
			fprintf(stderr, "Unable to set audio: %s\n", SDL_GetError());
			sound=0;
		} else
			soundLoad();
	}

	vi=(SDL_VideoInfo *)SDL_GetVideoInfo();
	if(vi)
		bpp=vi->vfmt->BitsPerPixel;

	if(conf.fullscreen)
		screen=SDL_SetVideoMode(SCREENW, SCREENH, bpp, SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF|SDL_FULLSCREEN);
	else
		screen=SDL_SetVideoMode(SCREENW, SCREENH, bpp, SDL_HWSURFACE|SDL_HWPALETTE|SDL_DOUBLEBUF);

	if(!screen) {
		fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
		return 1;
	}

	/* init the joystick */
	if(SDL_WasInit(SDL_INIT_JOYSTICK) & SDL_INIT_JOYSTICK)
		if(SDL_NumJoysticks()>=1)
		{
			joy[0]=SDL_JoystickOpen(0);
			if(SDL_NumJoysticks()>1)
				joy[1]=SDL_JoystickOpen(1);
		}

	/* hide the mouse */
	SDL_ShowCursor(SDL_DISABLE);

	/* set the caption */
	SDL_WM_SetCaption(APP_NAME,NULL);

	/* load console gfx */
	sprintf(buffer,"%s/gfx.bmp","data");
	gfx=loadBMP(buffer);
	if(!gfx) {
		fprintf(stderr,"Unable load gfx: %s\n", SDL_GetError());
		return 1;
	}
	/* set transparent color */
	if(SDL_SetColorKey(gfx, SDL_SRCCOLORKEY, SDL_MapRGB(gfx->format, 255, 0, 255))<0) {
			fprintf(stderr,"Unable to setup gfx: %s\n", SDL_GetError());
			return 1;
	}

	/* main LOOP */
	while(menu()) {

		/* init the engine */
		engine_init();

		SDL_FillRect(screen,NULL,SDL_MapRGB(screen->format,0,0,0));
		SDL_Flip(screen);

		if(sound && bgm) {
			Mix_VolumeMusic(MIX_MAX_VOLUME);
			Mix_FadeInMusic(bgm,-1,2000);
			SDL_Delay(2000);
		}

		player[0].joy=(int)conf.control[0]==JOYSTICK;
		player[1].joy=(int)conf.control[1]==JOYSTICK;

		pause=0;
		pause_tick=0;
		boss=0;
		gameLoop();

		if(sound && bgm) {
			Mix_FadeOutMusic(2000);
			SDL_Delay(3000);
		}

		for(i=0;i<2;i++) {
			/* check if there's a place for this score */
			for(j=9;j>=0 && hiscore[j].score<player[i].score;j--);

			/* the player will be in the hall of fame? */
			if(j<9) {
				for(k=8;k>j;k--)
					hiscore[k+1]=hiscore[k];

				/* put the new score */
				hiscore[j+1].score=player[i].score;
				hiscore[j+1].stage=player[i].stage;

				hiscore[j+1].name[0]=0;
				if(!getName(hiscore[j+1].name, j+2,i+1))
					break; /* probably a problem if the user closes the window */

				/* show the hall of fame */
				hiscores();
			}
		}
	}

	if(sound) {
		if(bgm)
			Mix_FreeMusic(bgm);
		if(bgm_boss)
			Mix_FreeMusic(bgm_boss);

		for(i=0;i<NUM_EFX;i++)
			if(efx[i])
				Mix_FreeChunk(efx[i]);

		Mix_CloseAudio();
	}

	/* release the joystick */
	if(SDL_JoystickOpened(0))
		SDL_JoystickClose(joy[0]);
	if(SDL_JoystickOpened(1))
		SDL_JoystickClose(joy[1]);

	/* free all! */
	SDL_FreeSurface(gfx);

	/* free engine memory */
	engine_release();

	/* now update conf changes */

	/* update cnf file */
	#ifndef GP32
	sprintf(buffer,"%s/dd2.cfg","data");
	saveCFG(buffer,&conf);
	#endif

	/* save hi-scores */
	sprintf(buffer,"%s/hiscore","data");
	if(!saveScore(buffer,hiscore))
		fprintf(stderr,"unable to save hi-scores\ndo you have permissions to write into %s?\n"
			,buffer);

	return 0;
}
