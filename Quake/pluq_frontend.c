/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_frontend.c -- PluQ Frontend IPC Implementation
// Frontend binary only - connects to backend, receives world state, sends input

#include "pluq_frontend.h"
#include <string.h>

// ============================================================================
// FRONTEND CONTEXT
// ============================================================================

static pluq_context_t frontend_ctx;
static qboolean frontend_initialized = false;
static uint32_t last_received_frame = 0;

// Received frame state storage
typedef struct
{
	uint32_t frame_number;
	double timestamp;
	vec3_t view_origin;
	vec3_t view_angles;
	int16_t health;
	int16_t armor;
	uint8_t weapon;
	uint16_t ammo;
	qboolean paused;
	qboolean in_game;
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
		Con_Printf("PluQ Frontend already initialized\n");
		return true;
	}

	Con_Printf("Initializing PluQ Frontend IPC sockets (nng+FlatBuffers)...\n");

	// Note: nng library already initialized by PluQ_Init()

	memset(&frontend_ctx, 0, sizeof(frontend_ctx));
	frontend_ctx.is_backend = false;
	frontend_ctx.is_frontend = true;

	// Initialize frontend sockets (REQ, SUB, PUSH)

	// Resources channel (REQ/REP) - Frontend connects with REQ
	if ((rv = nng_req0_open(&frontend_ctx.resources_req)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create resources REQ socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.resources_dialer, frontend_ctx.resources_req, PLUQ_URL_RESOURCES)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.resources_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}

	// Gameplay channel (PUB/SUB) - Frontend connects with SUB
	if ((rv = nng_sub0_open(&frontend_ctx.gameplay_sub)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create gameplay SUB socket: %s\n", nng_strerror(rv));
		goto error;
	}
	// Subscribe to all topics (empty string = all)
	// nng 2.0 API: use nng_sub0_socket_subscribe() instead of nng_socket_set_string()
	if ((rv = nng_sub0_socket_subscribe(frontend_ctx.gameplay_sub, "", 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to subscribe to gameplay channel: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.gameplay_dialer, frontend_ctx.gameplay_sub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.gameplay_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}

	// Input channel (PUSH/PULL) - Frontend connects with PUSH
	if ((rv = nng_push0_open(&frontend_ctx.input_push)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create input PUSH socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_create(&frontend_ctx.input_dialer, frontend_ctx.input_push, PLUQ_URL_INPUT)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to create dialer for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_dialer_start(frontend_ctx.input_dialer, 0)) != 0)
	{
		Con_Printf("PluQ Frontend: Failed to start dialer on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}

	Con_Printf("PluQ Frontend: IPC sockets initialized successfully\n");
	Con_Printf("PluQ Frontend: Connected to backend on ports 9001-9003\n");

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

	Con_Printf("PluQ Frontend: Shutting down\n");

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

qboolean PluQ_Frontend_RequestResource(uint32_t resource_id)
{
	// TODO: Build ResourceRequest FlatBuffer and send
	Con_Printf("PluQ Frontend: Requesting resource ID %u (not yet implemented)\n", resource_id);
	return false;
}

qboolean PluQ_Frontend_ReceiveResource(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(frontend_ctx.resources_req, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ Frontend: Failed to receive resource: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Frontend_ReceiveFrame(void **flatbuf_out, size_t *size_out)
{
	int rv;
	nng_msg *msg;

	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	if ((rv = nng_recvmsg(frontend_ctx.gameplay_sub, &msg, NNG_FLAG_NONBLOCK)) != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ Frontend: Failed to receive gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
	return true;
}

qboolean PluQ_Frontend_SendInput(const void *flatbuf, size_t size)
{
	if (!frontend_ctx.initialized || !frontend_ctx.is_frontend)
		return false;

	int rv = nng_send(frontend_ctx.input_push, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ Frontend: Failed to send input command: %s\n", nng_strerror(rv));
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

	// Build InputCommand FlatBuffer
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Create InputCommand with just the command text
	flatbuffers_string_ref_t cmd_str = flatbuffers_string_create_str(&builder, cmd_text);

	PluQ_InputCommand_start(&builder);
	PluQ_InputCommand_cmd_text_add(&builder, cmd_str);
	PluQ_InputCommand_end_as_root(&builder);

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
	void *buf;
	size_t size;

	if (!PluQ_Frontend_ReceiveFrame(&buf, &size))
		return false;

	// Parse GameplayMessage
	PluQ_GameplayMessage_table_t msg = PluQ_GameplayMessage_as_root(buf);
	if (!msg)
	{
		nng_msg_free((nng_msg *)buf);  // Free the nng_msg wrapper
		return false;
	}

	// Get event type and value
	PluQ_GameplayEvent_union_type_t event_type = PluQ_GameplayMessage_event_type(msg);
	flatbuffers_generic_t event_value = PluQ_GameplayMessage_event(msg);

	if (event_type == PluQ_GameplayEvent_FrameUpdate)
	{
		PluQ_FrameUpdate_table_t frame = (PluQ_FrameUpdate_table_t)event_value;

		// Parse and store frame data
		received_state.frame_number = PluQ_FrameUpdate_frame_number(frame);
		received_state.timestamp = PluQ_FrameUpdate_timestamp(frame);

		// View state
		const PluQ_Vec3_t *view_origin = PluQ_FrameUpdate_view_origin(frame);
		const PluQ_Vec3_t *view_angles = PluQ_FrameUpdate_view_angles(frame);
		if (view_origin)
			FB_Vec3_To_Quake(view_origin, received_state.view_origin);
		if (view_angles)
			FB_Vec3_To_Quake(view_angles, received_state.view_angles);

		// Player stats
		received_state.health = PluQ_FrameUpdate_health(frame);
		received_state.armor = PluQ_FrameUpdate_armor(frame);
		received_state.weapon = PluQ_FrameUpdate_weapon(frame);
		received_state.ammo = PluQ_FrameUpdate_ammo(frame);

		// Game state
		received_state.paused = PluQ_FrameUpdate_paused(frame);
		received_state.in_game = PluQ_FrameUpdate_in_game(frame);
		received_state.valid = true;

		last_received_frame = received_state.frame_number;
		Con_DPrintf("PluQ Frontend: Received frame %u (health=%d, armor=%d)\n",
			last_received_frame, received_state.health, received_state.armor);
	}
	else if (event_type == PluQ_GameplayEvent_MapChanged)
	{
		PluQ_MapChanged_table_t mapchange = (PluQ_MapChanged_table_t)event_value;
		const char *mapname = PluQ_MapChanged_mapname(mapchange);
		Con_Printf("PluQ Frontend: Map changed to %s\n", mapname);
	}
	else if (event_type == PluQ_GameplayEvent_Disconnected)
	{
		PluQ_Disconnected_table_t disc = (PluQ_Disconnected_table_t)event_value;
		const char *reason = PluQ_Disconnected_reason(disc);
		Con_Printf("PluQ Frontend: Disconnected: %s\n", reason);
	}

	nng_msg_free((nng_msg *)buf);
	return true;
}

void PluQ_Frontend_ApplyReceivedState(void)
{
	if (!frontend_initialized || !received_state.valid)
		return;

	// Apply view state
	VectorCopy(received_state.view_origin, r_refdef.vieworg);
	VectorCopy(received_state.view_angles, cl.viewangles);

	// Apply player stats
	cl.stats[STAT_HEALTH] = received_state.health;
	cl.stats[STAT_ARMOR] = received_state.armor;
	cl.stats[STAT_WEAPON] = received_state.weapon;
	cl.stats[STAT_AMMO] = received_state.ammo;

	// Apply game state
	cl.paused = received_state.paused;
	cl.time = received_state.timestamp;

	// Note: Entity rendering would go here
	// For now, frontend displays stats/HUD based on backend's authoritative state
}

void PluQ_Frontend_SendInputCommand(usercmd_t *cmd)
{
	static uint32_t sequence = 0;

	if (!frontend_initialized || !cmd)
		return;

	// Build InputCommand FlatBuffer
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	PluQ_Vec3_t view_angles = QuakeVec3_To_FB(cmd->viewangles);

	PluQ_InputCommand_start(&builder);
	PluQ_InputCommand_sequence_add(&builder, sequence++);
	PluQ_InputCommand_timestamp_add(&builder, Sys_DoubleTime());
	PluQ_InputCommand_forward_move_add(&builder, cmd->forwardmove);
	PluQ_InputCommand_side_move_add(&builder, cmd->sidemove);
	PluQ_InputCommand_up_move_add(&builder, cmd->upmove);
	PluQ_InputCommand_view_angles_add(&builder, &view_angles);
	// Note: buttons and impulse would be added here if available in usercmd_t
	PluQ_InputCommand_end_as_root(&builder);

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

void PluQ_Frontend_ApplyViewAngles(void)
{
	// Frontend receives view angles from backend via FrameUpdate
	// This is applied in PluQ_Frontend_ApplyReceivedState()
	// This function is a no-op for frontend (unlike backend which receives from frontend)
}

void PluQ_Frontend_Move(usercmd_t *cmd)
{
	// Frontend generates movement locally and sends to backend
	// This function can be used to modify local movement before sending
	// For now, it's a pass-through - movement is sent as-is via PluQ_Frontend_SendInputCommand()
}

// ============================================================================
// RESOURCE STREAMING
// ============================================================================

typedef struct
{
	uint32_t resource_id;
	PluQ_ResourceType_enum_t resource_type;
	void *data;
	size_t data_size;
	qboolean valid;
} received_resource_t;

static received_resource_t received_resource = {0};

qboolean PluQ_Frontend_RequestResource(PluQ_ResourceType_enum_t resource_type,
                                        uint32_t resource_id,
                                        const char *resource_name,
                                        void **data_out,
                                        size_t *size_out)
{
	if (!frontend_initialized)
		return false;

	Con_DPrintf("PluQ Frontend: Requesting resource type=%d, id=%u, name=%s\n",
		resource_type, resource_id, resource_name ? resource_name : "(null)");

	// Build ResourceRequest
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	PluQ_ResourceRequest_start(&builder);
	PluQ_ResourceRequest_resource_type_add(&builder, resource_type);
	PluQ_ResourceRequest_resource_id_add(&builder, resource_id);
	if (resource_name && *resource_name)
	{
		PluQ_ResourceRequest_resource_name_add(&builder,
			flatbuffers_string_create_str(&builder, resource_name));
	}
	PluQ_ResourceRequest_ref_t request_ref = PluQ_ResourceRequest_end(&builder);
	PluQ_ResourceRequest_end_as_root(&builder);

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
		Con_Printf("PluQ Frontend: Failed to send resource request: %s\n", nng_strerror(rv));
		return false;
	}

	// Receive response (blocking)
	nng_msg *msg;
	rv = nng_recvmsg(frontend_ctx.resources_req, &msg, 0);
	if (rv != 0)
	{
		Con_Printf("PluQ Frontend: Failed to receive resource response: %s\n", nng_strerror(rv));
		return false;
	}

	// Parse ResourceResponse
	void *response_buf = nng_msg_body(msg);
	size_t response_size = nng_msg_len(msg);

	PluQ_ResourceResponse_table_t response = PluQ_ResourceResponse_as_root(response_buf);
	if (!response)
	{
		Con_Printf("PluQ Frontend: Invalid resource response\n");
		nng_msg_free(msg);
		return false;
	}

	uint32_t response_id = PluQ_ResourceResponse_resource_id(response);
	PluQ_ResourceData_union_type_t data_type = PluQ_ResourceResponse_data_type(response);

	Con_DPrintf("PluQ Frontend: Received resource response id=%u, type=%d\n",
		response_id, data_type);

	// Extract resource data based on type
	qboolean success = false;
	if (data_type == PluQ_ResourceData_Texture)
	{
		PluQ_Texture_table_t texture = (PluQ_Texture_table_t)PluQ_ResourceResponse_data(response);
		if (texture)
		{
			uint16_t width = PluQ_Texture_width(texture);
			uint16_t height = PluQ_Texture_height(texture);
			flatbuffers_uint8_vec_t pixels = PluQ_Texture_pixels(texture);
			size_t pixels_len = flatbuffers_uint8_vec_len(pixels);

			Con_DPrintf("PluQ Frontend: Received texture %dx%d (%zu bytes)\n",
				width, height, pixels_len);

			// Allocate and copy texture data
			if (pixels_len > 0)
			{
				// Copy pixel data - caller must free
				*data_out = malloc(pixels_len + 8); // +8 for width/height
				if (*data_out)
				{
					// Store as qpic_t format
					qpic_t *pic = (qpic_t *)(*data_out);
					pic->width = width;
					pic->height = height;
					memcpy(pic->data, pixels, pixels_len);
					*size_out = pixels_len + 8;
					success = true;
				}
			}
		}
	}
	else if (data_type == PluQ_ResourceData_Model)
	{
		Con_DPrintf("PluQ Frontend: Model data received (not yet processed)\n");
		// TODO: Process model data
	}

	nng_msg_free(msg);
	return success;
}
