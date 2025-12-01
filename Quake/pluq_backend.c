/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_backend.c -- PLQ Backend (Server) Implementation

#include "quakedef.h"
#include "pluq_backend.h"
#include <string.h>

// nng 1.x protocol headers
#include <nng/protocol/reqrep0/rep.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pipeline0/pull.h>

// ============================================================================
// BACKEND CONTEXT
// ============================================================================

typedef struct
{
	nng_socket resources_rep;
	nng_socket gameplay_pub;
	nng_socket input_pull;
	nng_listener resources_listener;
	nng_listener gameplay_listener;
	nng_listener input_listener;
	qboolean initialized;
} pluq_backend_context_t;

// Backend-specific state
static pluq_backend_context_t backend_ctx;
static qboolean backend_enabled = false;

// Input state from frontend
static pluq_input_cmd_t current_input = {0};
static qboolean has_current_input = false;

// ============================================================================
// BACKEND INITIALIZATION
// ============================================================================

void PluQ_Backend_Init(void)
{
	Con_Printf("PLQ Backend: Initialization deferred until Enable()\n");

	// Auto-enable backend mode when using -pluq
	if (COM_CheckParm("-pluq"))
	{
		Con_Printf("PLQ backend mode enabled via -pluq flag\n");
		PluQ_Backend_Enable();
	}
}

qboolean PluQ_Backend_Enable(void)
{
	int rv;

	if (backend_ctx.initialized)
	{
		Con_Printf("PLQ Backend: Already initialized\n");
		backend_enabled = true;
		return true;
	}

	Con_Printf("PLQ Backend: Initializing IPC sockets (nng+FlatBuffers)...\n");

	memset(&backend_ctx, 0, sizeof(backend_ctx));

	// Resources channel (REP socket - replies to resource requests)
	if ((rv = nng_rep0_open(&backend_ctx.resources_rep)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create REP socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.resources_listener, backend_ctx.resources_rep, PLUQ_URL_RESOURCES)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.resources_listener, 0)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}

	// Gameplay channel (PUB socket - broadcasts world state)
	if ((rv = nng_pub0_open(&backend_ctx.gameplay_pub)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create PUB socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.gameplay_listener, backend_ctx.gameplay_pub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.gameplay_listener, 0)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}

	// Input channel (PULL socket - receives input commands)
	if ((rv = nng_pull0_open(&backend_ctx.input_pull)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create PULL socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.input_listener, backend_ctx.input_pull, PLUQ_URL_INPUT)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.input_listener, 0)) != 0)
	{
		Con_Printf("PLQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}

	Con_Printf("PLQ Backend: IPC sockets initialized successfully\n");
	backend_ctx.initialized = true;
	backend_enabled = true;
	return true;

error:
	PluQ_Backend_Shutdown();
	return false;
}

void PluQ_Backend_Shutdown(void)
{
	if (!backend_ctx.initialized)
		return;

	Con_Printf("PLQ Backend: Shutting down\n");

	nng_close(backend_ctx.resources_rep);
	nng_close(backend_ctx.gameplay_pub);
	nng_close(backend_ctx.input_pull);

	memset(&backend_ctx, 0, sizeof(backend_ctx));
	backend_enabled = false;
}

qboolean PluQ_Backend_IsEnabled(void)
{
	return backend_enabled && backend_ctx.initialized;
}

// ============================================================================
// BACKEND TRANSPORT LAYER
// ============================================================================

qboolean PluQ_Backend_SendResource(const void *flatbuf, size_t size)
{
	if (!backend_ctx.initialized)
		return false;

	int rv = nng_send(backend_ctx.resources_rep, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PLQ Backend: Failed to send resource: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Backend_PublishFrame(const void *flatbuf, size_t size)
{
	if (!backend_ctx.initialized)
		return false;

	int rv = nng_send(backend_ctx.gameplay_pub, (void *)flatbuf, size, 0);
	if (rv != 0)
	{
		Con_Printf("PLQ Backend: Failed to publish gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Backend_ReceiveInput(nng_msg **msg_out)
{
	if (!backend_ctx.initialized)
		return false;

	int rv = nng_recvmsg(backend_ctx.input_pull, msg_out, NNG_FLAG_NONBLOCK);
	if (rv != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PLQ Backend: Failed to receive input command: %s\n", nng_strerror(rv));
		return false;
	}

	return true;
}

// ============================================================================
// BACKEND HIGH-LEVEL API
// ============================================================================

void PluQ_BroadcastWorldState(void)
{
	static int debug_count = 0;

	if (!PluQ_Backend_IsEnabled())
		return;

	// Don't broadcast if not in game
	if (!cl.worldmodel || cls.state != ca_connected)
	{
		if (debug_count++ < 5)
			Con_DPrintf("PluQ_BroadcastWorldState: no worldmodel (%p) or not connected (state=%d)\n",
				cl.worldmodel, cls.state);
		return;
	}

	static uint32_t frame_counter = 0;
	double start_time = Sys_DoubleTime();

	// Debug: Log first few broadcasts
	if (frame_counter < 5)
		Con_Printf("PLQ Backend: Broadcasting frame %u\n", frame_counter);

	// Initialize FlatBuffers builder
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Build view state struct
	PLQ_plq_viewstate_t_t viewstate;
	viewstate.viewangles_x = cl.viewangles[0];
	viewstate.viewangles_y = cl.viewangles[1];
	viewstate.viewangles_z = cl.viewangles[2];
	viewstate.velocity_x = cl.velocity[0];
	viewstate.velocity_y = cl.velocity[1];
	viewstate.velocity_z = cl.velocity[2];
	viewstate.punchangle_x = cl.punchangle[0];
	viewstate.punchangle_y = cl.punchangle[1];
	viewstate.punchangle_z = cl.punchangle[2];
	viewstate.viewheight = cl.viewheight;
	viewstate.time = cl.time;
	viewstate.viewentity = cl.viewentity;
	viewstate.flags = 0;
	if (cl.paused)
		viewstate.flags |= PLQ_FLAG_PAUSED;
	if (cl.onground)
		viewstate.flags |= PLQ_FLAG_ONGROUND;
	if (cl.inwater)
		viewstate.flags |= PLQ_FLAG_INWATER;
	if (cl.intermission)
		viewstate.flags |= PLQ_FLAG_INTERMISSION;

	// Build stats struct (direct copy from cl.stats[])
	PLQ_plq_stats_t_t stats;
	stats.health = cl.stats[STAT_HEALTH];
	stats.frags = cl.stats[STAT_FRAGS];
	stats.weapon = cl.stats[STAT_WEAPON];
	stats.ammo = cl.stats[STAT_AMMO];
	stats.armor = cl.stats[STAT_ARMOR];
	stats.weaponframe = cl.stats[STAT_WEAPONFRAME];
	stats.shells = cl.stats[STAT_SHELLS];
	stats.nails = cl.stats[STAT_NAILS];
	stats.rockets = cl.stats[STAT_ROCKETS];
	stats.cells = cl.stats[STAT_CELLS];
	stats.activeweapon = cl.stats[STAT_ACTIVEWEAPON];
	stats.totalsecrets = cl.stats[STAT_TOTALSECRETS];
	stats.totalmonsters = cl.stats[STAT_TOTALMONSTERS];
	stats.secrets = cl.stats[STAT_SECRETS];
	stats.monsters = cl.stats[STAT_MONSTERS];
	stats.items = cl.items;

	// Start building the frame
	PLQ_plq_frame_t_start(&builder);
	PLQ_plq_frame_t_view_add(&builder, &viewstate);
	PLQ_plq_frame_t_stats_add(&builder, &stats);

	// Build entities vector - using entity_state_t which is binary compatible
	PLQ_entity_state_t_vec_start(&builder);
	for (int i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *ent = cl_visedicts[i];
		if (!ent)
			continue;

		// Build entity_state_t struct
		PLQ_entity_state_t_t ent_state;
		ent_state.origin_x = ent->origin[0];
		ent_state.origin_y = ent->origin[1];
		ent_state.origin_z = ent->origin[2];
		ent_state.angles_x = ent->angles[0];
		ent_state.angles_y = ent->angles[1];
		ent_state.angles_z = ent->angles[2];
		ent_state.modelindex = ent->model ? (uint16_t)((size_t)ent->model & 0xFFFF) : 0;
		ent_state.frame = (uint16_t)ent->frame;
		ent_state.colormap = ent->colormap ? ent->colormap[0] : 0;
		ent_state.skin = (uint8_t)ent->skinnum;
		ent_state.alpha = ent->alpha;
		ent_state.scale = 128; // Default scale (128 = 1.0)
		ent_state.effects = ent->effects;

		PLQ_entity_state_t_vec_push(&builder, &ent_state);
	}
	PLQ_entity_state_t_vec_ref_t entities_ref = PLQ_entity_state_t_vec_end(&builder);
	PLQ_plq_frame_t_entities_add(&builder, entities_ref);

	// Build dynamic lights vector
	PLQ_dlight_t_vec_start(&builder);
	for (int i = 0; i < MAX_DLIGHTS; i++)
	{
		dlight_t *dl = &cl_dlights[i];
		if (dl->die < cl.time || dl->radius <= 0)
			continue;

		PLQ_dlight_t_t dlight;
		dlight.origin_x = dl->origin[0];
		dlight.origin_y = dl->origin[1];
		dlight.origin_z = dl->origin[2];
		dlight.radius = dl->radius;
		dlight.spawn = dl->spawn;
		dlight.die = dl->die;
		dlight.decay = dl->decay;
		dlight.minlight = dl->minlight;
		dlight.key = dl->key;
		dlight.color_r = dl->color[0];
		dlight.color_g = dl->color[1];
		dlight.color_b = dl->color[2];

		PLQ_dlight_t_vec_push(&builder, &dlight);
	}
	PLQ_dlight_t_vec_ref_t dlights_ref = PLQ_dlight_t_vec_end(&builder);
	PLQ_plq_frame_t_dlights_add(&builder, dlights_ref);

	// Build color shifts vector
	PLQ_cshift_t_vec_start(&builder);
	for (int i = 0; i < NUM_CSHIFTS; i++)
	{
		PLQ_cshift_t_t cshift;
		cshift.destcolor_r = cl.cshifts[i].destcolor[0];
		cshift.destcolor_g = cl.cshifts[i].destcolor[1];
		cshift.destcolor_b = cl.cshifts[i].destcolor[2];
		cshift.percent = (float)cl.cshifts[i].percent;

		PLQ_cshift_t_vec_push(&builder, &cshift);
	}
	PLQ_cshift_t_vec_ref_t cshifts_ref = PLQ_cshift_t_vec_end(&builder);
	PLQ_plq_frame_t_cshifts_add(&builder, cshifts_ref);

	PLQ_plq_frame_t_ref_t frame_ref = PLQ_plq_frame_t_end(&builder);

	// Wrap in message
	PLQ_plq_message_t_start_as_root(&builder);
	PLQ_plq_message_t_type_add(&builder, PLQ_plq_msgtype_t_frame);
	PLQ_plq_message_t_payload_plq_frame_t_add(&builder, frame_ref);
	PLQ_plq_message_t_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Publish frame
		PluQ_Backend_PublishFrame(buf, size);

		// Update stats
		pluq_stats_t perf_stats;
		PluQ_GetStats(&perf_stats);
		perf_stats.frames_sent++;
		perf_stats.total_entities += cl_numvisedicts;
		double frame_time = Sys_DoubleTime() - start_time;
		perf_stats.total_time += frame_time;
		if (frame_time > perf_stats.max_frame_time)
			perf_stats.max_frame_time = frame_time;
		if (perf_stats.min_frame_time == 0.0 || frame_time < perf_stats.min_frame_time)
			perf_stats.min_frame_time = frame_time;
		PluQ_SetStats(&perf_stats);

		// Free buffer
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
	frame_counter++;
}

qboolean PluQ_HasPendingInput(void)
{
	if (!PluQ_Backend_IsEnabled())
		return false;

	// Return true if we have received and stored input from frontend
	// PluQ_ProcessInputCommands() must be called to receive and store input first
	return has_current_input;
}

void PluQ_ProcessInputCommands(void)
{
	nng_msg *msg;

	if (!PluQ_Backend_IsEnabled())
		return;

	// Process all pending input commands
	while (PluQ_Backend_ReceiveInput(&msg))
	{
		void *buf = nng_msg_body(msg);

		// Parse FlatBuffer message
		PLQ_plq_message_t_table_t plq_msg = PLQ_plq_message_t_as_root(buf);
		if (!plq_msg)
		{
			Con_Printf("PLQ Backend: Failed to parse input message\n");
			nng_msg_free(msg);
			continue;
		}

		// Handle different message types
		PLQ_plq_msgtype_t_enum_t msg_type = PLQ_plq_message_t_type(plq_msg);

		if (msg_type == PLQ_plq_msgtype_t_input)
		{
			// Get input payload - cast union value to input table
			PLQ_plq_input_t_table_t input = (PLQ_plq_input_t_table_t)PLQ_plq_message_t_payload(plq_msg);
			if (input)
			{
				// Get usercmd_t struct (binary compatible)
				const PLQ_usercmd_t_t *cmd = PLQ_plq_input_t_cmd(input);
				if (cmd)
				{
					current_input.view_angles[0] = cmd->viewangles_x;
					current_input.view_angles[1] = cmd->viewangles_y;
					current_input.view_angles[2] = cmd->viewangles_z;
					current_input.forward_move = cmd->forwardmove;
					current_input.side_move = cmd->sidemove;
					current_input.up_move = cmd->upmove;
				}
				has_current_input = true;
			}
		}
		else if (msg_type == PLQ_plq_msgtype_t_command)
		{
			// Get command payload
			PLQ_plq_command_t_table_t cmd = (PLQ_plq_command_t_table_t)PLQ_plq_message_t_payload(plq_msg);
			if (cmd)
			{
				const char *cmd_text = PLQ_plq_command_t_text(cmd);
				if (cmd_text && cmd_text[0])
				{
					Con_Printf("PLQ Backend: Received command: \"%s\"\n", cmd_text);
					Cbuf_AddText(cmd_text);
					Cbuf_AddText("\n");
				}
			}
		}
		else
		{
			Con_DPrintf("PLQ Backend: Unexpected message type %d on input channel\n", msg_type);
		}
		nng_msg_free(msg);
	}
}

void PluQ_Move(usercmd_t *cmd)
{
	if (!PluQ_Backend_IsEnabled() || !has_current_input || !cmd)
		return;

	// Apply movement from frontend input
	cmd->forwardmove = current_input.forward_move;
	cmd->sidemove = current_input.side_move;
	cmd->upmove = current_input.up_move;
}

void PluQ_ApplyViewAngles(void)
{
	if (!PluQ_Backend_IsEnabled() || !has_current_input)
		return;

	// Apply view angles from frontend input
	VectorCopy(current_input.view_angles, cl.viewangles);
}

// ============================================================================
// RESOURCE STREAMING
// ============================================================================

void PluQ_ProcessResourceRequests(void)
{
	nng_msg *msg;
	int rv;

	if (!PluQ_Backend_IsEnabled())
		return;

	// Check for resource requests (non-blocking)
	rv = nng_recvmsg(backend_ctx.resources_rep, &msg, NNG_FLAG_NONBLOCK);
	if (rv != 0)
	{
		if (rv != NNG_EAGAIN)  // EAGAIN means no messages
			Con_Printf("PLQ Backend: Failed to receive resource request: %s\n", nng_strerror(rv));
		return;
	}

	// Parse ResourceRequest
	void *request_buf = nng_msg_body(msg);
	PLQ_plq_message_t_table_t request_msg = PLQ_plq_message_t_as_root(request_buf);
	if (!request_msg || PLQ_plq_message_t_type(request_msg) != PLQ_plq_msgtype_t_resourcereq)
	{
		Con_Printf("PLQ Backend: Invalid resource request message\n");
		nng_msg_free(msg);
		return;
	}

	PLQ_plq_resourcereq_t_table_t request = (PLQ_plq_resourcereq_t_table_t)PLQ_plq_message_t_payload(request_msg);
	if (!request)
	{
		Con_Printf("PLQ Backend: Resource request payload is null\n");
		nng_msg_free(msg);
		return;
	}

	PLQ_plq_resource_type_t_enum_t resource_type = PLQ_plq_resourcereq_t_type(request);
	uint16_t resource_index = PLQ_plq_resourcereq_t_index(request);
	const char *resource_name = PLQ_plq_resourcereq_t_name(request);

	Con_DPrintf("PLQ Backend: Resource request - type=%d, index=%u, name=%s\n",
		resource_type, resource_index, resource_name ? resource_name : "(null)");

	// Free request message
	nng_msg_free(msg);

	// Build response
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	PLQ_plq_resourceresp_t_start(&builder);
	PLQ_plq_resourceresp_t_type_add(&builder, resource_type);
	PLQ_plq_resourceresp_t_index_add(&builder, resource_index);
	if (resource_name)
		PLQ_plq_resourceresp_t_name_create_str(&builder, resource_name);

	// Handle different resource types
	switch (resource_type)
	{
	case PLQ_plq_resource_type_t_texture:
	{
		// Load texture from WAD
		if (resource_name && *resource_name)
		{
			lumpinfo_t *lump_info = NULL;
			byte *lump_data = W_GetLumpName(resource_name, &lump_info);

			if (lump_data && lump_info)
			{
				// qpic_t format: width, height, data
				qpic_t *pic = (qpic_t *)lump_data;
				int width = LittleLong(pic->width);
				int height = LittleLong(pic->height);

				// Add raw pixel data
				PLQ_plq_resourceresp_t_data_create(&builder, pic->data, width * height);

				Con_DPrintf("PLQ Backend: Sending texture '%s' (%dx%d, %d bytes)\n",
					resource_name, width, height, width * height);
			}
			else
			{
				Con_Printf("PLQ Backend: Texture '%s' not found\n", resource_name);
			}
		}
		break;
	}

	case PLQ_plq_resource_type_t_model:
	{
		Con_DPrintf("PLQ Backend: Model streaming not yet implemented\n");
		break;
	}

	default:
		Con_Printf("PLQ Backend: Unsupported resource type %d\n", resource_type);
		break;
	}

	PLQ_plq_resourceresp_t_ref_t response_ref = PLQ_plq_resourceresp_t_end(&builder);

	// Wrap in message
	PLQ_plq_message_t_start_as_root(&builder);
	PLQ_plq_message_t_type_add(&builder, PLQ_plq_msgtype_t_resourceresp);
	PLQ_plq_message_t_payload_plq_resourceresp_t_add(&builder, response_ref);
	PLQ_plq_message_t_end_as_root(&builder);

	// Finalize and send response
	size_t response_size;
	void *response_buf = flatcc_builder_finalize_buffer(&builder, &response_size);

	if (response_buf)
	{
		PluQ_Backend_SendResource(response_buf, response_size);
		flatcc_builder_aligned_free(response_buf);
	}

	flatcc_builder_clear(&builder);
}
