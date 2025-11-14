/*
Copyright (C) 2024 QuakeSpasm/Ironwail developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
*/

// pluq_backend.c -- PluQ Backend (Server) Implementation

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
	Con_Printf("PluQ Backend: Initialization deferred until Enable()\n");

	// Auto-enable backend mode when using -pluq
	if (COM_CheckParm("-pluq"))
	{
		Con_Printf("PluQ backend mode enabled via -pluq flag\n");
		PluQ_Backend_Enable();
	}
}

qboolean PluQ_Backend_Enable(void)
{
	int rv;

	if (backend_ctx.initialized)
	{
		Con_Printf("PluQ Backend: Already initialized\n");
		backend_enabled = true;
		return true;
	}

	Con_Printf("PluQ Backend: Initializing IPC sockets (nng+FlatBuffers)...\n");

	memset(&backend_ctx, 0, sizeof(backend_ctx));

	// Resources channel (REP socket - replies to resource requests)
	if ((rv = nng_rep0_open(&backend_ctx.resources_rep)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create REP socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.resources_listener, backend_ctx.resources_rep, PLUQ_URL_RESOURCES)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.resources_listener, 0)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_RESOURCES, nng_strerror(rv));
		goto error;
	}

	// Gameplay channel (PUB socket - broadcasts world state)
	if ((rv = nng_pub0_open(&backend_ctx.gameplay_pub)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create PUB socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.gameplay_listener, backend_ctx.gameplay_pub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.gameplay_listener, 0)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		goto error;
	}

	// Input channel (PULL socket - receives input commands)
	if ((rv = nng_pull0_open(&backend_ctx.input_pull)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create PULL socket: %s\n", nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_create(&backend_ctx.input_listener, backend_ctx.input_pull, PLUQ_URL_INPUT)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to create listener for %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}
	if ((rv = nng_listener_start(backend_ctx.input_listener, 0)) != 0)
	{
		Con_Printf("PluQ Backend: Failed to start listener on %s: %s\n", PLUQ_URL_INPUT, nng_strerror(rv));
		goto error;
	}

	Con_Printf("PluQ Backend: IPC sockets initialized successfully\n");
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

	Con_Printf("PluQ Backend: Shutting down\n");

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
		Con_Printf("PluQ Backend: Failed to send resource: %s\n", nng_strerror(rv));
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
		Con_Printf("PluQ Backend: Failed to publish gameplay frame: %s\n", nng_strerror(rv));
		return false;
	}
	return true;
}

qboolean PluQ_Backend_ReceiveInput(void **flatbuf_out, size_t *size_out)
{
	nng_msg *msg;

	if (!backend_ctx.initialized)
		return false;

	int rv = nng_recvmsg(backend_ctx.input_pull, &msg, NNG_FLAG_NONBLOCK);
	if (rv != 0)
	{
		if (rv != NNG_EAGAIN)
			Con_Printf("PluQ Backend: Failed to receive input command: %s\n", nng_strerror(rv));
		return false;
	}

	*flatbuf_out = nng_msg_body(msg);
	*size_out = nng_msg_len(msg);
	// Note: Caller must call nng_msg_free(msg) when done
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
		Con_Printf("PluQ Backend: Broadcasting frame %u\n", frame_counter);

	// Initialize FlatBuffers builder
	flatcc_builder_t builder;
	flatcc_builder_init(&builder);

	// Build FrameUpdate
	PluQ_FrameUpdate_start(&builder);

	// Frame info
	PluQ_FrameUpdate_frame_number_add(&builder, frame_counter++);
	PluQ_FrameUpdate_timestamp_add(&builder, cl.time);

	// View state
	PluQ_Vec3_t view_origin = QuakeVec3_To_FB(r_refdef.vieworg);
	PluQ_Vec3_t view_angles = QuakeVec3_To_FB(cl.viewangles);
	PluQ_FrameUpdate_view_origin_add(&builder, &view_origin);
	PluQ_FrameUpdate_view_angles_add(&builder, &view_angles);

	// Player stats
	PluQ_FrameUpdate_health_add(&builder, (int16_t)cl.stats[STAT_HEALTH]);
	PluQ_FrameUpdate_armor_add(&builder, (int16_t)cl.stats[STAT_ARMOR]);
	PluQ_FrameUpdate_weapon_add(&builder, (uint8_t)cl.stats[STAT_WEAPON]);
	PluQ_FrameUpdate_ammo_add(&builder, (uint16_t)cl.stats[STAT_AMMO]);

	// Game state
	PluQ_FrameUpdate_paused_add(&builder, (cl.paused != 0));
	PluQ_FrameUpdate_in_game_add(&builder, true);

	// Entities - build vector of visible entities
	PluQ_Entity_vec_start(&builder);

	for (int i = 0; i < cl_numvisedicts; i++)
	{
		entity_t *ent = cl_visedicts[i];
		if (!ent)
			continue;

		// Build entity
		PluQ_Vec3_t origin = QuakeVec3_To_FB(ent->origin);
		PluQ_Vec3_t angles = QuakeVec3_To_FB(ent->angles);

		PluQ_Entity_vec_push_start(&builder);
		PluQ_Entity_origin_add(&builder, &origin);
		PluQ_Entity_angles_add(&builder, &angles);

		// Model ID: Use model pointer as ID (will be 0 if no model)
		// Frontend will need to request model data via Resources channel
		PluQ_Entity_model_id_add(&builder, ent->model ? (uint16_t)((size_t)ent->model & 0xFFFF) : 0);

		PluQ_Entity_frame_add(&builder, (uint8_t)ent->frame);
		PluQ_Entity_colormap_add(&builder, ent->colormap ? ent->colormap[0] : 0);
		PluQ_Entity_skin_add(&builder, (uint8_t)ent->skinnum);
		PluQ_Entity_effects_add(&builder, (uint32_t)ent->effects);
		PluQ_Entity_alpha_add(&builder, ent->alpha / 255.0f); // Convert byte to float

		PluQ_Entity_vec_push_end(&builder);
	}

	PluQ_Entity_vec_ref_t entities_ref = PluQ_Entity_vec_end(&builder);
	PluQ_FrameUpdate_entities_add(&builder, entities_ref);

	PluQ_FrameUpdate_ref_t frame_ref = PluQ_FrameUpdate_end(&builder);

	// Wrap in GameplayMessage
	PluQ_GameplayEvent_union_ref_t event;
	event.type = PluQ_GameplayEvent_FrameUpdate;
	event.value = frame_ref;

	PluQ_GameplayMessage_create(&builder, event);
	PluQ_GameplayMessage_end_as_root(&builder);

	// Finalize buffer
	size_t size;
	void *buf = flatcc_builder_finalize_buffer(&builder, &size);

	if (buf)
	{
		// Publish frame
		PluQ_Backend_PublishFrame(buf, size);

		// Update stats
		pluq_stats_t stats;
		PluQ_GetStats(&stats);
		stats.frames_sent++;
		stats.total_entities += cl_numvisedicts;
		double frame_time = Sys_DoubleTime() - start_time;
		stats.total_time += frame_time;
		if (frame_time > stats.max_frame_time)
			stats.max_frame_time = frame_time;
		if (stats.min_frame_time == 0.0 || frame_time < stats.min_frame_time)
			stats.min_frame_time = frame_time;
		PluQ_SetStats(&stats);

		// Free buffer
		flatcc_builder_aligned_free(buf);
	}

	flatcc_builder_clear(&builder);
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
	void *buf;
	size_t size;

	if (!PluQ_Backend_IsEnabled())
		return;

	// Process all pending input commands
	while (PluQ_Backend_ReceiveInput(&buf, &size))
	{
		// Parse FlatBuffer
		PluQ_InputCommand_table_t cmd = PluQ_InputCommand_as_root(buf);
		if (!cmd)
		{
			Con_Printf("PluQ Backend: Failed to parse InputCommand\n");
			nng_msg_free((nng_msg *)buf);
			continue;
		}

		// Store input data for PluQ_Move and PluQ_ApplyViewAngles
		current_input.sequence = PluQ_InputCommand_sequence(cmd);
		current_input.timestamp = PluQ_InputCommand_timestamp(cmd);
		current_input.forward_move = PluQ_InputCommand_forward_move(cmd);
		current_input.side_move = PluQ_InputCommand_side_move(cmd);
		current_input.up_move = PluQ_InputCommand_up_move(cmd);

		// Get view angles
		const PluQ_Vec3_t *angles = PluQ_InputCommand_view_angles(cmd);
		if (angles)
			FB_Vec3_To_Quake(angles, current_input.view_angles);

		current_input.buttons = PluQ_InputCommand_buttons(cmd);
		current_input.impulse = (uint8_t)PluQ_InputCommand_impulse(cmd);

		// Get command text (if any)
		const char *cmd_text = PluQ_InputCommand_cmd_text(cmd);
		if (cmd_text && cmd_text[0])
		{
			Con_Printf("PluQ Backend: Received command: \"%s\"\n", cmd_text);
			Cbuf_AddText(cmd_text);
			Cbuf_AddText("\n");
			q_strlcpy(current_input.cmd_text, cmd_text, sizeof(current_input.cmd_text));
		}
		else
		{
			current_input.cmd_text[0] = 0;
		}

		has_current_input = true;
		nng_msg_free((nng_msg *)buf);
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
