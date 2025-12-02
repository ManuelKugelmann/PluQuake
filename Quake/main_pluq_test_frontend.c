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
#include <errno.h>
#include <poll.h>

// Import from host_pluq_frontend.c
extern void Host_Init_PluQ_Frontend(void);
extern void Host_Shutdown_PluQ_Frontend(void);

#define DEFAULT_MEMORY (64 * 1024 * 1024) // Minimal heap for test

static quakeparms_t parms;

// Input state
static qboolean stdin_eof = false;

/*
==================
Input_CheckStdin

Check if stdin has data available (non-blocking)
Uses poll() which properly distinguishes data vs EOF
==================
*/
static qboolean Input_CheckStdin(void)
{
	struct pollfd pfd;

	if (stdin_eof)
		return false;

	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	pfd.revents = 0;

	if (poll(&pfd, 1, 0) <= 0)
		return false;

	// POLLIN set means data OR EOF - we need to try reading to know which
	return (pfd.revents & POLLIN) != 0;
}

/*
==================
Input_ReadStdin

Read a line from stdin (non-blocking)
Returns true if a complete line was read
==================
*/
static qboolean Input_ReadStdin(char *buffer, size_t bufsize)
{
	static char line_buf[1024];
	static int line_pos = 0;
	char c;
	ssize_t n;

	if (stdin_eof)
		return false;

	while (Input_CheckStdin())
	{
		n = read(STDIN_FILENO, &c, 1);

		if (n == 0)
		{
			// EOF - stdin closed, stop trying to read
			stdin_eof = true;
			return false;
		}
		else if (n < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return false; // No data right now
			// Other error - treat as EOF
			stdin_eof = true;
			return false;
		}

		// Got a character
		if (c == '\n' || c == '\r')
		{
			if (line_pos > 0)
			{
				line_buf[line_pos] = '\0';
				strncpy(buffer, line_buf, bufsize - 1);
				buffer[bufsize - 1] = '\0';
				line_pos = 0;
				return true;
			}
		}
		else if (line_pos < (int)sizeof(line_buf) - 1)
		{
			line_buf[line_pos++] = c;
		}
	}

	return false;
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

	// Read commands from stdin (non-blocking)
	// Commands are forwarded to backend automatically via cmd.c when PLUQ_FRONTEND is defined
	while (Input_ReadStdin(input_line, sizeof(input_line)))
	{
		printf("[Frontend] → Backend: %s\n", input_line);
		fflush(stdout);

		// Add to command buffer - cmd.c will forward to backend via IPC
		Cbuf_AddText(input_line);
		Cbuf_AddText("\n");
	}

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

	// Set stdin to non-blocking for command input
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);

	printf("\n[Type commands, or pipe: echo 'map e1m1' | %s]\n", argv[0]);
	printf("Examples: status, map e1m1, god\n\n");

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
