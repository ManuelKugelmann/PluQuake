/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_frontend.c -- PLQ Frontend IPC Implementation
// Frontend binary only - connects to backend, receives world state, sends input

#include "pluq_frontend.h"
#include <string.h>

// ============================================================================
// FRONTEND CONTEXT
// ============================================================================

typedef struct
{
	nng_socket resources_req;
	nng_socket gameplay_sub;
	nng_socket input_push;
	nng_dialer resources_dialer;
	nng_dialer gameplay_dialer;
	nng_dialer input_dialer;
	qboolean initialized;
	qboolean is_frontend;
} pluq_frontend_context_t;

static pluq_frontend_context_t frontend_ctx;
static qboolean frontend_initialized = false;
static uint32_t last_received_frame = 0;
static uint32_t frames_received = 0;

// Received frame state storage
typedef struct
{
	// View state
	vec3_t viewangles;
	vec3_t velocity;
	vec3_t punchangle;
	float viewheight;
	double time;
	int viewentity;
	uint16_t flags;

	// Stats
	int health;
	int frags;
	int weapon;
	int ammo;
	int armor;
	int weaponframe;
	int shells, nails, rockets, cells;
	int activeweapon;
	int totalsecrets, totalmonsters;
	int secrets, monsters;
	unsigned int items;

	// State flags
	qboolean valid;
} received_frame_state_t;

static received_frame_state_t received_state = {0};

// ============================================================================
// FRONTEND INITIALIZATION / SHUTDOWN
// ============================================================================

qboolean PluQ_Frontend_Init(void)
{
	int rv;

	if (frontend_initialized)
	{
		Con_Printf("PLQ Frontend already initialized\n");
		return true;
	}

	Con_Printf("Initializing PLQ Frontend IPC sockets (nng+FlatBuffers)...\n");

	// Note: nng library already initialized by PluQ_Init()

	memset(&frontend_ctx, 0, sizeof(frontend_ctx));
	frontend_ctx.is_frontend = true;

	// Initialize frontend sockets (REQ, SUB, PUSH)

	// Resources channel (REQ/REP) - Frontend connects with REQ
	if ((rv = nng_req0_open(&frontend_ctx.resources_req)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create resources REQ socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.resources_dialer, frontend_ctx.resources_req, PLUQ_URL_RESOURCES)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.resources_dialer, 0)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}

	// Gameplay channel (PUB/SUB) - Frontend connects with SUB
	if ((rv = nng_sub0_open(&frontend_ctx.gameplay_sub)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create gameplay SUB socket: %s\n", nng_strerror(rv));
		goto error;
	}
	// Subscribe to all topics (empty string = all)
	if ((rv = nng_sub0_socket_subscribe(frontend_ctx.gameplay_sub, "", 0)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to subscribe to gameplay channel: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.gameplay_dialer, frontend_ctx.gameplay_sub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.gameplay_dialer, 0)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}

	// Input channel (PUSH/PULL) - Frontend connects with PUSH
	if ((rv = nng_push0_open(&frontend_ctx.input_push)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create input PUSH socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.input_dialer, frontend_ctx.input_push, PLUQ_URL_INPUT)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.input_dialer, 0)) != 0)
	{
		Con_Printf("PLQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}

	Con_Printf("PLQ Frontend: IPC sockets initialized successfully\n");
	Con_Printf("PLQ Frontend: Connected to backend on ports 9001-9003\n");

	frontend_ctx.initialized = true;
	frontend_initialized = true;
	return true;

error:
	PluQ_Frontend_Shutdown();
	return false;
}

void PluQ_Frontend_Shutdown(void)
{
	if (!frontend_initialized)
		return;

	Con_Printf("PLQ Frontend: Shutting down (received %u frames)\n", frames_received);

	// Close frontend sockets
	nng_socket_close(frontend_ctx.resources_req);
	nng_socket_close(frontend_ctx.gameplay_sub);
	nng_socket_close(frontend_ctx.input_push);

	memset(&frontend_ctx, 0, sizeof(frontend_ctx));
	frontend_initialized = false;
}

// ============================================================================
// FRONTEND TRANSPORT LAYER
// ============================================================================

qboolean PluQ_Frontend_ReceiveFrame(nng_msg **msg_out)
{
	int rv;
	static int first_frame_received = 0;
	static int poll_count = 0;

	poll_count++;

	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
	{
		printf("[ReceiveFrame] Not initialized! init=%d is_fe=%d\n",
			frontend_ctx.initialized, frontend_ctx.is_frontend);
		fflush(stdout);
		return false;
	}

	// Debug: Show first few polls
	if (poll_count <= 3)
	{
		printf("[ReceiveFrame] Poll %d, calling nng_recvmsg...\n", poll_count);
		fflush(stdout);
	}

	if ((rv = nng_recvmsg(frontend_ctx.gameplay_sub, msg_out, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (poll_count <= 3)
		{
			printf("[ReceiveFrame] Poll %d: rv=%d (%s)\n", poll_count, rv, nng_strerror(rv));
			fflush(stdout);
		}
		if (rv != NNG_EAGAIN)
			Con_Printf("PLQ Frontend: recvmsg error: %s\n", nng_strerror(rv));
		return false;
	}

	// Log first frame
	if (!first_frame_received)
	{
		printf("[ReceiveFrame] FIRST FRAME! (%zu bytes)\n", nng_msg_len(*msg_out));
		fflush(stdout);
		Con_Printf("PLQ Frontend: First frame received! (%zu bytes)\n", nng_msg_len(*msg_out));
		first_frame_received = 1;
	}

	return true;
}

qboolean PluQ_Frontend_SendInput(const void *flatbuf, size_t size)
{
	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	int rv = nng_send(frontend_ctx.input_push, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PLQ Frontend: Failed to send input command: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

// ============================================================================
// FRONTEND HIGH-LEVEL API
// ============================================================================

void PluQ_Frontend_SendCommand(const char *cmd_text)
{
	if (!frontend_initialized || !cmd_text || !cmd_text[0])
		return;

	// Build plq_command_t FlatBuffer wrapped in plq_message_t
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Create command table
	PLQ_plq_command_t_start(&builder);
	PLQ_plq_command_t_text_create_str(&builder, cmd_text);
	PLQ_plq_command_t_ref_t cmd_ref = PLQ_plq_command_t_end(&builder);

	// Wrap in message
	PLQ_plq_message_t_start_as_root(&builder);
	PLQ_plq_message_t_type_add(&builder, PLQ_plq_msgtype_t_command);
	PLQ_plq_message_t_payload_plq_command_t_add(&builder, cmd_ref);
	PLQ_plq_message_t_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Send to backend
		PluQ_Frontend_SendInput(buf, size);
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
}

qboolean PluQ_Frontend_ReceiveWorldState(void)
{
	nng_msg *msg;
	static int call_count = 0;

	call_count++;
	if (call_count <= 3)
	{
		printf("[ReceiveWorldState] Call %d\n", call_count);
		fflush(stdout);
	}

	if (!PluQ_Frontend_ReceiveFrame(&msg))
		return false;

	void *buf = nng_msg_body(msg);

	// Parse plq_message_t
	PLQ_plq_message_t_table_t plq_msg = PLQ_plq_message_t_as_root(buf);
	if (!plq_msg)
	{
		nng_msg_free(msg);
		return false;
	}

	PLQ_plq_msgtype_t_enum_t msg_type = PLQ_plq_message_t_type(plq_msg);

	if (msg_type == PLQ_plq_msgtype_t_frame)
	{
		PLQ_plq_frame_t_table_t frame = (PLQ_plq_frame_t_table_t)PLQ_plq_message_t_payload(plq_msg);
		if (!frame)
		{
			nng_msg_free(msg);
			return false;
		}

		// Parse view state
		const PLQ_plq_viewstate_t_t *view = PLQ_plq_frame_t_view(frame);
		if (view)
		{
			received_state.viewangles[0] = view->viewangles_x;
			received_state.viewangles[1] = view->viewangles_y;
			received_state.viewangles[2] = view->viewangles_z;
			received_state.velocity[0] = view->velocity_x;
			received_state.velocity[1] = view->velocity_y;
			received_state.velocity[2] = view->velocity_z;
			received_state.punchangle[0] = view->punchangle_x;
			received_state.punchangle[1] = view->punchangle_y;
			received_state.punchangle[2] = view->punchangle_z;
			received_state.viewheight = view->viewheight;
			received_state.time = view->time;
			received_state.viewentity = view->viewentity;
			received_state.flags = view->flags;
		}

		// Parse stats
		const PLQ_plq_stats_t_t *stats = PLQ_plq_frame_t_stats(frame);
		if (stats)
		{
			received_state.health = stats->health;
			received_state.frags = stats->frags;
			received_state.weapon = stats->weapon;
			received_state.ammo = stats->ammo;
			received_state.armor = stats->armor;
			received_state.weaponframe = stats->weaponframe;
			received_state.shells = stats->shells;
			received_state.nails = stats->nails;
			received_state.rockets = stats->rockets;
			received_state.cells = stats->cells;
			received_state.activeweapon = stats->activeweapon;
			received_state.totalsecrets = stats->totalsecrets;
			received_state.totalmonsters = stats->totalmonsters;
			received_state.secrets = stats->secrets;
			received_state.monsters = stats->monsters;
			received_state.items = stats->items;
		}

		// Parse entities (could store these for rendering)
		PLQ_entity_state_t_vec_t entities = PLQ_plq_frame_t_entities(frame);
		size_t num_entities = PLQ_entity_state_t_vec_len(entities);

		received_state.valid = true;
		frames_received++;
		last_received_frame = frames_received;

		Con_DPrintf("PLQ Frontend: Received frame %u (health=%d, %zu entities)\n",
			frames_received, received_state.health, num_entities);
	}
	else if (msg_type == PLQ_plq_msgtype_t_mapchange)
	{
		PLQ_plq_mapchange_t_table_t mapchange = (PLQ_plq_mapchange_t_table_t)PLQ_plq_message_t_payload(plq_msg);
		if (mapchange)
		{
			const char *mapname = PLQ_plq_mapchange_t_mapname(mapchange);
			const char *levelname = PLQ_plq_mapchange_t_levelname(mapchange);
			Con_Printf("PLQ Frontend: Map changed to %s (%s)\n",
				mapname ? mapname : "(null)",
				levelname ? levelname : "(null)");
		}
	}
	else if (msg_type == PLQ_plq_msgtype_t_disconnect)
	{
		PLQ_plq_disconnect_t_table_t disc = (PLQ_plq_disconnect_t_table_t)PLQ_plq_message_t_payload(plq_msg);
		if (disc)
		{
			const char *reason = PLQ_plq_disconnect_t_reason(disc);
			Con_Printf("PLQ Frontend: Disconnected: %s\n", reason ? reason : "(no reason)");
		}
	}
	else if (msg_type == PLQ_plq_msgtype_t_lightstyles)
	{
		PLQ_plq_lightstyles_t_table_t lightstyles = (PLQ_plq_lightstyles_t_table_t)PLQ_plq_message_t_payload(plq_msg);
		if (lightstyles)
		{
			PLQ_plq_lightstyle_t_vec_t styles = PLQ_plq_lightstyles_t_styles(lightstyles);
			size_t num_styles = PLQ_plq_lightstyle_t_vec_len(styles);
			Con_DPrintf("PLQ Frontend: Received %zu light styles\n", num_styles);
		}
	}

	nng_msg_free(msg);
	return true;
}

void PluQ_Frontend_ApplyReceivedState(void)
{
	if (!frontend_initialized || !received_state.valid)
		return;

	// Apply view state
	VectorCopy(received_state.viewangles, cl.viewangles);
	VectorCopy(received_state.velocity, cl.velocity);
	VectorCopy(received_state.punchangle, cl.punchangle);
	cl.viewheight = received_state.viewheight;
	cl.time = received_state.time;
	cl.viewentity = received_state.viewentity;

	// Apply flags
	cl.paused = (received_state.flags & PLQ_FLAG_PAUSED) ? 1 : 0;
	cl.onground = (received_state.flags & PLQ_FLAG_ONGROUND) ? 1 : 0;
	cl.inwater = (received_state.flags & PLQ_FLAG_INWATER) ? 1 : 0;
	cl.intermission = (received_state.flags & PLQ_FLAG_INTERMISSION) ? 1 : 0;

	// Apply player stats
	cl.stats[STAT_HEALTH] = received_state.health;
	cl.stats[STAT_FRAGS] = received_state.frags;
	cl.stats[STAT_WEAPON] = received_state.weapon;
	cl.stats[STAT_AMMO] = received_state.ammo;
	cl.stats[STAT_ARMOR] = received_state.armor;
	cl.stats[STAT_WEAPONFRAME] = received_state.weaponframe;
	cl.stats[STAT_SHELLS] = received_state.shells;
	cl.stats[STAT_NAILS] = received_state.nails;
	cl.stats[STAT_ROCKETS] = received_state.rockets;
	cl.stats[STAT_CELLS] = received_state.cells;
	cl.stats[STAT_ACTIVEWEAPON] = received_state.activeweapon;
	cl.stats[STAT_TOTALSECRETS] = received_state.totalsecrets;
	cl.stats[STAT_TOTALMONSTERS] = received_state.totalmonsters;
	cl.stats[STAT_SECRETS] = received_state.secrets;
	cl.stats[STAT_MONSTERS] = received_state.monsters;
	cl.items = received_state.items;

	// Note: Entity rendering would go here
	// For now, frontend displays stats/HUD based on backend's authoritative state
}

void PluQ_Frontend_SendInputCommand(usercmd_t *cmd)
{
	static uint32_t sequence = 0;

	if (!frontend_initialized || !cmd)
		return;

	// Build plq_input_t FlatBuffer wrapped in plq_message_t
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Build usercmd_t struct (binary compatible with Quake's usercmd_t)
	PLQ_usercmd_t_t usercmd;
	usercmd.viewangles_x = cmd->viewangles[0];
	usercmd.viewangles_y = cmd->viewangles[1];
	usercmd.viewangles_z = cmd->viewangles[2];
	usercmd.forwardmove = cmd->forwardmove;
	usercmd.sidemove = cmd->sidemove;
	usercmd.upmove = cmd->upmove;

	// Create input table
	PLQ_plq_input_t_start(&builder);
	PLQ_plq_input_t_cmd_add(&builder, &usercmd);
	PLQ_plq_input_t_msec_add(&builder, (uint8_t)(host_frametime * 1000));
	PLQ_plq_input_t_ref_t input_ref = PLQ_plq_input_t_end(&builder);

	// Wrap in message
	PLQ_plq_message_t_start_as_root(&builder);
	PLQ_plq_message_t_type_add(&builder, PLQ_plq_msgtype_t_input);
	PLQ_plq_message_t_payload_plq_input_t_add(&builder, input_ref);
	PLQ_plq_message_t_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Send to backend
		PluQ_Frontend_SendInput(buf, size);
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
	sequence++;
}

void PluQ_Frontend_ApplyViewAngles(void)
{
	// Frontend receives view angles from backend via plq_frame_t
	// This is applied in PluQ_Frontend_ApplyReceivedState()
	// This function is a no-op for frontend (unlike backend which receives from frontend)
}

void PluQ_Frontend_Move(usercmd_t *cmd)
{
	// Frontend generates movement locally and sends to backend
	// This function can be used to modify local movement before sending
	// For now, it's a pass-through - movement is sent as-is via PluQ_Frontend_SendInputCommand()
}

uint32_t PluQ_Frontend_GetFramesReceived(void)
{
	return frames_received;
}

// ============================================================================
// RESOURCE STREAMING
// ============================================================================

qboolean PluQ_Frontend_RequestResource(PLQ_plq_resource_type_t_enum_t resource_type,
                                        uint16_t resource_index,
                                        const char *resource_name,
                                        void **data_out,
                                        size_t *size_out)
{
	if (!frontend_initialized)
		return false;

	Con_DPrintf("PLQ Frontend: Requesting resource type=%d, index=%u, name=%s\n",
		resource_type, resource_index, resource_name ? resource_name : "(null)");

	// Build plq_resourcereq_t wrapped in plq_message_t
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	PLQ_plq_resourcereq_t_start(&builder);
	PLQ_plq_resourcereq_t_type_add(&builder, resource_type);
	PLQ_plq_resourcereq_t_index_add(&builder, resource_index);
	if (resource_name && *resource_name)
		PLQ_plq_resourcereq_t_name_create_str(&builder, resource_name);
	PLQ_plq_resourcereq_t_ref_t request_ref = PLQ_plq_resourcereq_t_end(&builder);

	// Wrap in message
	PLQ_plq_message_t_start_as_root(&builder);
	PLQ_plq_message_t_type_add(&builder, PLQ_plq_msgtype_t_resourcereq);
	PLQ_plq_message_t_payload_plq_resourcereq_t_add(&builder, request_ref);
	PLQ_plq_message_t_end_as_root(&builder);

	// Finalize request buffer
	size_t request_size;
	void *request_buf = flatcc_builder_finalize_buffer(&builder, &request_size);

	if (!request_buf)
	{
		flatcc_builder_clear(&builder);
		return false;
	}

	// Send request (blocking REQ/REP pattern)
	int rv = nng_send(frontend_ctx.resources_req, request_buf, request_size, 0);
	flatcc_builder_aligned_free(request_buf);
	flatcc_builder_clear(&builder);

	if (rv != 0)
	{
		Con_Printf("PLQ Frontend: Failed to send resource request: %s\n", nng_strerror(rv));
		return false;
	}

	// Receive response (blocking)
	nng_msg *msg;
	rv = nng_recvmsg(frontend_ctx.resources_req, &msg, 0);
	if (rv != 0)
	{
		Con_Printf("PLQ Frontend: Failed to receive resource response: %s\n", nng_strerror(rv));
		return false;
	}

	// Parse plq_message_t
	void *response_buf = nng_msg_body(msg);
	PLQ_plq_message_t_table_t response_msg = PLQ_plq_message_t_as_root(response_buf);
	if (!response_msg || PLQ_plq_message_t_type(response_msg) != PLQ_plq_msgtype_t_resourceresp)
	{
		Con_Printf("PLQ Frontend: Invalid resource response message\n");
		nng_msg_free(msg);
		return false;
	}

	PLQ_plq_resourceresp_t_table_t response = (PLQ_plq_resourceresp_t_table_t)PLQ_plq_message_t_payload(response_msg);
	if (!response)
	{
		Con_Printf("PLQ Frontend: Resource response payload is null\n");
		nng_msg_free(msg);
		return false;
	}

	// Get resource data
	flatbuffers_uint8_vec_t data = PLQ_plq_resourceresp_t_data(response);
	size_t data_len = flatbuffers_uint8_vec_len(data);

	Con_DPrintf("PLQ Frontend: Received resource response (%zu bytes)\n", data_len);

	qboolean success = false;
	if (data_len > 0)
	{
		// Copy data - caller must free
		*data_out = malloc(data_len);
		if (*data_out)
		{
			memcpy(*data_out, data, data_len);
			*size_out = data_len;
			success = true;
		}
	}

	nng_msg_free(msg);
	return success;
}
