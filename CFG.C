/* Read an AVRIL-style CFG file */

/* 

   Copyright 1994, 1995, 1996, 1997, 1998, 1999  by Bernie Roehl 

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA,
   02111-1307, USA.

   For more information, contact Bernie Roehl <broehl@uwaterloo.ca> 
   or at Bernie Roehl, 68 Margaret Avenue North, Waterloo, Ontario, 
   N2J 3P7, Canada

*/

#include "avril.h"
#include "avrildrv.h"
#include <stdlib.h>   /* strtoul(), atof() */
#include <string.h>
#include <ctype.h>

/* These next few externs should really be in avrildrv.h */

extern vrl_DisplayDriverFunction vrl_DisplayDriverDefault;
extern vrl_DisplayDriverFunction vrl_DisplayDriverModeY;

static int show_compass = 0, show_position = 0, show_framerate = 0;

static int getline(char *buff, int maxbytes, FILE *in)
	{
	char *p;
	if (fgets(buff, maxbytes, in) == NULL) return 0;
	if ((p = strchr(buff, '\n')) != NULL) *p = '\0';
	return 1;
	}

static int lookup(char *token, char *table[], int n)
	{
	int i;
	for (i = 0; i < n; ++i)
		if (!stricmp(token, table[i]))
			return i;
	return -1;
	}

static int parse(char *buff, char *seps, char *argv[])
	{
	char *p;
	int argc = 0;
	if ((p = strchr(buff, '#')) != NULL) *p = '\0';
	p = strtok(buff, seps);
	while (p)
		{
		argv[argc++] = p;
		p = strtok(NULL, seps);
		}
	return argc;
	}

static char *statement_table[] =
	{
	"version", "loadpath", "include",
	"device", "devconfig", "displaydriver", "videodriver", "sounddriver",
	"compass", "framerate", "position", "cursor",
	"stereoparams", "stereoleft", "stereoright", "stereotype"
	};

static enum statements
	{
	st_version, st_loadpath, st_include,
	st_device, st_devconfig, st_displaydriver, st_videodriver, st_sounddriver,
	st_compass, st_framerate, st_position, st_cursor,
	st_stereoparams, st_stereoleft, st_stereoright, st_stereotype,
	ncmds
	};

static char *stereotypes[] =
	{
	"NONE", "SEQUENTIAL",
	"ANAGLYPH_SEQUENTIAL", "ANAGLYPH_WIRE_ALTERNATE",
	"ENIGMA", "FRESNEL",
	"CYBERSCOPE", "CRYSTALEYES",
	"CHROMADEPTH", "SIRDS",
	"TWOCARDS", "ANAGLYPH_SOLID_ALTERNATE"
	};

static int nstereotypes = 12;

static int driver_setup(int argc, char *argv[])
	{
	int mode = 0, serial = 1, buffsize = 2000, irq = 3;
	unsigned int address = 0x2F8;
	char *nickname = "No name device", *drivername = "Unknown device";
	vrl_SerialPort *port = NULL;
	vrl_DeviceDriverFunction *fn;
	vrl_Device *device;
	switch (argc)
		{
		case 1: return -1;    /* bad syntax */
		default:
		case 7: buffsize = strtoul(argv[6], NULL, 0);
		case 6: irq = strtoul(argv[5], NULL, 0);
		case 5: address = strtoul(argv[4], NULL, 0);
		case 4: mode = strtoul(argv[3], NULL, 0);
		case 3: drivername = strdup(argv[2]);
		case 2: nickname = strdup(argv[1]);
		}
	if (!stricmp(drivername, "mouse")) { fn = vrl_MouseDevice; serial = 0; }
	else if (!stricmp(drivername, "GDC")) { fn = vrl_GlobalDevice; serial = 1; }
	else if (!stricmp(drivername, "Cyberman")) { fn = vrl_CybermanDevice; serial = 1; }
	else if (!stricmp(drivername, "Spaceball")) { fn = vrl_SpaceballDevice; serial = 1; }
	else if (!stricmp(drivername, "RedBaron")) { fn = vrl_RedbaronDevice; serial = 1; }
	else if (!stricmp(drivername, "CTM")) { fn = vrl_CTMDevice; serial = 1; }
	else if (!stricmp(drivername, "Isotrak")) { fn = vrl_IsotrakDevice; serial = 1; }
	else if (!stricmp(drivername, "VIO")) { fn = vrl_VIODevice; serial = 1; }
#ifdef VRL_PC_COMPATABLE
	/* these next few only exist on PC's */
	else if (!stricmp(drivername, "keypad")) { fn = vrl_KeypadDevice; serial = 0; }
	else if (!stricmp(drivername, "joystick")) { fn = vrl_JoystickDevice; serial = 0; }
	else if (!stricmp(drivername, "Pad")) { fn = vrl_PadDevice; serial = 0; }
	else if (!stricmp(drivername, "CyberWand")) { fn = vrl_CyberwandDevice; serial = 0; }
	else if (!stricmp(drivername, "7thSense")) { fn = vrl_7thSenseDevice; serial = 0; }
#endif
	/* add other devices in here, as well as making entries in avrildrv.h */
	else return -2;  /* unknown device */
	if (serial)
		{
		port = vrl_SerialOpen(address, irq, buffsize);
		if (port == NULL)
			return -3;  /* couldn't open serial port */
		}
	device = vrl_DeviceOpen(fn, port);
	if (device == NULL)
		return -4;   /* couldn't open device */
	vrl_DeviceSetNickname(device, nickname);
	vrl_DeviceSetMode(device, mode);
	return 0;	
	}

int vrl_ReadCFGProcessLine(char *buff)
	{
	char buffcopy[256];
	int argc;
	char *argv[20];
	vrl_StereoConfiguration *conf;
	strcpy(buffcopy, buff);  /* unparsed version */
	argc = parse(buff, " \t,", argv);
	if (argc < 1) return 0;  /* ignore blank lines */
	if (argc < 2) return 0;  /* all statements currently have at least one parameter */
	switch (lookup(argv[0], statement_table, ncmds))
		{
		st_version: break;
		case st_loadpath: vrl_FileSetLoadpath(argv[1]); break;
		case st_include:
			{
			FILE *new = fopen(vrl_FileFixupFilename(argv[1]), "r");
			if (new)
				{
				vrl_ReadCFG(new);
				fclose(new);
				}
			}
			break;
		case st_device: driver_setup(argc, argv); break;
		case st_cursor: if (stricmp(argv[1], "on")) vrl_VideoCursorHide(); break;
		case st_compass: show_compass = !stricmp(argv[1], "on"); break;
		case st_position: show_position = !stricmp(argv[1], "on"); break;
		case st_framerate: show_framerate = !stricmp(argv[1], "on"); break;
		case st_displaydriver:
			if (argc < 2) break;
			if (!stricmp(argv[1], "ModeY"))
				vrl_DisplaySetDriver(vrl_DisplayDriverModeY);
			else
				vrl_DisplaySetDriver(vrl_DisplayDriverDefault);
			vrl_DisplayInit(NULL);
			break;
		case st_videodriver:
			{
			int mode = 0;
			if (argc < 2) break;
			if (argc > 2) mode = strtoul(argv[2], NULL, 0);
			vrl_VideoShutdown();
			if (!stricmp(argv[1], "Mode13"))
				vrl_VideoSetDriver(vrl_VideoDriverMode13);
			else if (!stricmp(argv[1], "ModeY"))
				vrl_VideoSetDriver(vrl_VideoDriverModeY);
			else if (!stricmp(argv[1], "7thSense"))
				vrl_VideoSetDriver(vrl_VideoDriver7thSense);
			vrl_VideoSetup(mode);
			vrl_DisplayInit(NULL);
			vrl_MouseReset();
			}
			break;
		case st_devconfig:
			{
			vrl_Device *device;
			int channel;
			if (argc < 3) break;
			device = vrl_DeviceFind(argv[1]);
			if (device == NULL) break;
			if (isdigit(*argv[2]))
				channel = atoi(argv[2]);
			else
				channel = toupper(*argv[2]) - 'X' + ((toupper(argv[2][1]) == 'R') ? 3 : 0);
			if (argc > 3)
				{
				if (toupper(*argv[3]) == 'A')
					vrl_DeviceSetScale(device, channel, float2angle(atof(&argv[3][1])));
				else
					vrl_DeviceSetScale(device, channel, float2scalar(atof(argv[3])));
				}
			if (argc > 4)
				vrl_DeviceSetDeadzone(device, channel, float2scalar(atof(argv[4])));
			}
			break;
		case st_stereotype:
			conf = vrl_WorldGetStereoConfiguration();
			if (conf == NULL)
				vrl_WorldSetStereoConfiguration(conf = vrl_StereoCreateConfiguration());
			if (conf && argc > 1)
				{
				int n = lookup(argv[1], stereotypes, nstereotypes);
				if (n >= 0)
					vrl_StereoSetType(conf, n);
				vrl_WorldSetStereo(n);  /* NONE (==0) is FALSE, non-zero is TRUE */
				}
			break;
		case st_stereoparams:
			conf = vrl_WorldGetStereoConfiguration();
			if (conf == NULL)
				vrl_WorldSetStereoConfiguration(conf = vrl_StereoCreateConfiguration());
			if (conf == NULL)
				break;
			if (vrl_StereoGetType(conf) == VRL_STEREOTYPE_CHROMADEPTH)
				{					
				if (argc > 2)
					vrl_StereoSetChromaFar(conf, float2scalar(atof(argv[2])));
				if (argc > 1)
					vrl_StereoSetChromaNear(conf, float2scalar(atof(argv[1])));
				}
			else
				{
				if (argc > 2)
					vrl_StereoSetConvergence(conf, atof(argv[2]));
				if (argc > 1)
					vrl_StereoSetEyespacing(conf, atof(argv[1]));
				}
			break;
		case st_stereoleft:
			conf = vrl_WorldGetStereoConfiguration();
			if (conf == NULL)
				vrl_WorldSetStereoConfiguration(conf = vrl_StereoCreateConfiguration());
			if (conf && argc > 2)
				vrl_StereoSetLeftEyeRotation(conf, float2angle(atof(argv[2])));
			if (conf && argc > 1)
				vrl_StereoSetLeftEyeShift(conf, float2angle(atof(argv[1])));
			break;
		case st_stereoright:
			conf = vrl_WorldGetStereoConfiguration();
			if (conf == NULL)
				vrl_WorldSetStereoConfiguration(conf = vrl_StereoCreateConfiguration());
			if (conf && argc > 2)
				vrl_StereoSetRightEyeRotation(conf, float2angle(atof(argv[2])));
			if (conf && argc > 1)
				vrl_StereoSetRightEyeShift(conf, float2angle(atof(argv[1])));
			break;
		default:
			vrl_ReadWLDfeature(argc, argv, buffcopy);
			break;
		}
	return 0;
	}

int vrl_ReadCFG(FILE *in)
	{
	char buff[256];
	while (getline(buff, sizeof(buff), in))
		vrl_ReadCFGProcessLine(buff);
	return 0;
	}

void vrl_ConfigSetCompassDisplay(vrl_Boolean flag)
	{
	show_compass = flag;
	}

vrl_Boolean vrl_ConfigGetCompassDisplay(void)
	{
	return show_compass;
	}

void vrl_ConfigToggleCompassDisplay(void)
	{
	show_compass = !show_compass;
	}

void vrl_ConfigSetPositionDisplay(vrl_Boolean flag)
	{
	show_position = flag;
	}

vrl_Boolean vrl_ConfigGetPositionDisplay(void)
	{
	return show_position;
	}

void vrl_ConfigTogglePositionDisplay(void)
	{
	show_position = !show_position;
	}

void vrl_ConfigSetFramerateDisplay(vrl_Boolean flag)
	{
	show_framerate = flag;
	}

vrl_Boolean vrl_ConfigGetFramerateDisplay(void)
	{
	return show_framerate;
	}

void vrl_ConfigToggleFramerateDisplay(void)
	{
	show_framerate = !show_framerate;
	}

void vrl_ConfigStartup(char *filename)
	{
	vrl_SystemStartup();
	vrl_ReadCFGfile(filename);
	}

int vrl_ReadCFGfile(char *filename)
	{
	FILE *cfgfile;
	cfgfile = fopen(vrl_FileFixupFilename(filename ? filename : "avril.cfg"), "r");
	if (cfgfile)
		{
		vrl_ReadCFG(cfgfile);
		fclose(cfgfile);
		return 0;
		}
	return -1;
	}
