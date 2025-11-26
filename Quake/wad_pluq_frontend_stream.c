/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2010-2014 QuakeSpasm developers
Copyright (C) 2024 PluQuake developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// wad_pluq_frontend_stream.c -- WAD streaming via PluQ IPC
// This variant of wad.c streams resources from backend via IPC instead of loading locally

#include "quakedef.h"
#include "pluq_frontend.h"

int				wad_numlumps = 0;
lumpinfo_t		*wad_lumps = NULL;
byte			*wad_base = NULL;

void SwapPic (qpic_t *pic);

/*
==================
W_CleanupName

Lowercases name and pads with spaces and a terminating 0 to the length of
lumpinfo_t->name.
Used so lumpname lookups can proceed rapidly by comparing 4 chars at a time
Space padding is so names can be printed nicely in tables.
Can safely be performed in place.
==================
*/
void W_CleanupName (const char *in, char *out)
{
	int		i;
	int		c;

	for (i=0 ; i<16 ; i++ )
	{
		c = in[i];
		if (!c)
			break;

		if (c >= 'A' && c <= 'Z')
			c += ('a' - 'A');
		out[i] = c;
	}

	for ( ; i< 16 ; i++ )
		out[i] = 0;
}

/*
====================
W_LoadWadFile
====================
*/
void W_LoadWadFile (void)
{
	// PluQ Frontend Streaming variant: No local WAD file loading
	// Resources are streamed from backend on demand
	Con_Printf("W_LoadWadFile: Using PluQ resource streaming (no local WAD)\n");

	wad_base = NULL;
	wad_numlumps = 0;
	wad_lumps = NULL;
}

/*
==================
W_GetLumpinfo
==================
*/
lumpinfo_t *W_GetLumpinfo (const char *name)
{
	// Not used in streaming mode - resources are fetched on demand
	Con_DPrintf("W_GetLumpinfo: %s - using IPC streaming\n", name);
	return NULL;
}

/*
==================
W_GetLumpName

Streaming variant: Request resource from backend via IPC
==================
*/
void *W_GetLumpName (const char *name, lumpinfo_t **out_info)
{
	void *data = NULL;
	size_t size = 0;

	Con_DPrintf("W_GetLumpName: Requesting '%s' from backend via IPC\n", name);

	// Request resource from backend
	if (!PluQ_Frontend_RequestResource(PluQ_ResourceType_Texture, 0, name, &data, &size))
	{
		Con_SafePrintf("W_GetLumpName: Failed to fetch '%s' from backend\n", name);
		if (out_info)
			*out_info = NULL;
		return NULL;
	}

	Con_DPrintf("W_GetLumpName: Received '%s' (%zu bytes) from backend\n", name, size);

	// Create a temporary lumpinfo for compatibility
	// Note: This memory is leaked but it's minimal (20 bytes per texture)
	if (out_info)
	{
		lumpinfo_t *info = (lumpinfo_t *)malloc(sizeof(lumpinfo_t));
		if (info)
		{
			memset(info, 0, sizeof(lumpinfo_t));
			W_CleanupName(name, info->name);
			info->size = (int)size;
			info->filepos = 0;  // Not used in streaming mode
			info->type = 0;
			*out_info = info;
		}
		else
		{
			*out_info = NULL;
		}
	}

	return data;
}

void *W_GetLumpNum (int num)
{
	// Not supported in streaming mode
	Con_Printf("W_GetLumpNum: Lump number access not supported in streaming mode\n");
	return NULL;
}

/*
=============================================================================

automatic byte swapping

=============================================================================
*/

void SwapPic (qpic_t *pic)
{
	pic->width = LittleLong(pic->width);
	pic->height = LittleLong(pic->height);
}
