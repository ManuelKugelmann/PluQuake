/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// main_pluq_test_frontend.c -- Headless PluQ Frontend for IPC Testing
// Reuses host_pluq_frontend.c initialization, replaces only the main loop

#include "quakedef.h"
#include "pluq.h"
#include "pluq_frontend.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Import from host_pluq_frontend.c
extern void Host_Init_PluQ_Frontend(void);
extern void Host_Shutdown_PluQ_Frontend(void);

#define DEFAULT_MEMORY (64 * 1024 * 1024) // Minimal heap for test

static quakeparms_t parms;

/*
==================
Sys_CheckStdinAvailable
==================
*/
static qboolean Sys_CheckStdinAvailable(void)
{
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
}

/*
==================
Host_Frame_TestFrontend

Simplified frame: stdin → IPC → backend, backend → IPC → stdout
==================
*/
void Host_Frame_TestFrontend(double time)
{
	static char input_line[1024];
	static int input_pos = 0;
	static uint32_t frame_count = 0;
	static double last_status_time = 0;
	static qboolean connected = false;

	frame_count++;

	// Debug first few frames
	if (frame_count <= 3)
	{
		printf("[HOST_FRAME] Frame %u - starting\n", frame_count);
		fflush(stdout);
	}

	// Stdin input disabled - was blocking the main loop
#if 0
	// Read stdin commands
	while (Sys_CheckStdinAvailable())
	{
		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
		{
			if (c == '\n' || c == '\r')
			{
				if (input_pos > 0)
				{
					input_line[input_pos] = 0;
					printf("[Frontend] → Backend: %s\n", input_line);
					fflush(stdout);

					// Forward to backend
					PluQ_Frontend_SendCommand(input_line);

					// Also execute locally
					Cbuf_AddText(input_line);
					Cbuf_AddText("\n");

					input_pos = 0;
				}
			}
			else if (input_pos < (int)sizeof(input_line) - 1)
			{
				input_line[input_pos++] = c;
			}
		}
	}
#endif

	Cbuf_Execute();

	// Receive world state from backend
	qboolean got_frame = PluQ_Frontend_ReceiveWorldState();

	if (got_frame)
	{
		uint32_t received = PluQ_Frontend_GetFramesReceived();

		// Log connection establishment
		if (!connected)
		{
			printf("[Frontend] Connected to backend! First frame received.\n");
			fflush(stdout);
			connected = true;
		}

		// Periodic status (every 60 frames = ~1 second at 60fps)
		if (received % 60 == 0)
		{
			printf("[Frontend] Receiving: %u frames (frame %u)\n", received, frame_count);
			fflush(stdout);
		}

		PluQ_Frontend_ApplyReceivedState();
	}

	// Status output while waiting for connection
	double now = Sys_DoubleTime();
	if (!connected && now > last_status_time + 2.0)
	{
		printf("[Frontend] Waiting for backend... (poll %u)\n", frame_count);
		fflush(stdout);
		last_status_time = now;
	}
}

int main(int argc, char *argv[])
{
	double time, oldtime, newtime;

	host_parms = &parms;
	parms.basedir = ".";
	parms.argc = argc;
	parms.argv = argv;
	parms.errstate = 0;

	COM_InitArgv(parms.argc, parms.argv);

	Sys_Init();

	// Set stdout to unbuffered for immediate output
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("======================================\n");
	printf("PluQ Test Frontend (Headless)\n");
	printf("======================================\n");

	parms.memsize = DEFAULT_MEMORY;
	parms.membase = malloc(parms.memsize);

	if (!parms.membase)
		Sys_Error("Not enough memory\n");

	// Reuse existing frontend initialization
	Host_Init_PluQ_Frontend();

	printf("\n[Type commands and press Enter]\n");
	printf("Examples: map e1m1, skill 2, god\n\n");

	// Set stdin to non-blocking
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

	oldtime = Sys_DoubleTime();

	printf("[Frontend] Entering main loop (waiting for backend)...\n");
	fflush(stdout);

	// Main loop - simplified version of Host_Frame_PluQ_Frontend
	while (1)
	{
		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		if (time < 0.016)
		{
			usleep(1000);
			continue;
		}

		Host_Frame_TestFrontend(time);
		oldtime = newtime;
	}

	Host_Shutdown_PluQ_Frontend();

	return 0;
}
