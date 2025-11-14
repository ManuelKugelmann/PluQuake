/*
 * PluQ Backend Simulator - Broadcasts FrameUpdate messages
 * Used to test the gameplay channel without running full ironwail
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <signal.h>
#include <unistd.h>

#define PLUQ_URL_GAMEPLAY "tcp://127.0.0.1:9002"

// Include FlatBuffers builder
#include "pluq_builder.h"

static volatile int keep_running = 1;

void sigint_handler(int sig)
{
	(void)sig;
	keep_running = 0;
}

int main(void)
{
	int rv;
	nng_socket pub;
	nng_listener listener;
	uint32_t frame_number = 0;

	printf("=== PluQ Backend Simulator ===\n");
	printf("Broadcasting FrameUpdate messages via gameplay channel\n");
	printf("Press Ctrl+C to stop\n\n");

	// Set up signal handler
	signal(SIGINT, sigint_handler);

	// Create PUB socket (backend side, nng 1.x API)
	if ((rv = nng_pub0_open(&pub)) != 0)
	{
		fprintf(stderr, "Failed to create PUB socket: %s\n", nng_strerror(rv));
		return 1;
	}

	// Create listener
	if ((rv = nng_listener_create(&listener, pub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		fprintf(stderr, "Failed to create listener for %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		nng_close(pub);
		return 1;
	}

	if ((rv = nng_listener_start(listener, 0)) != 0)
	{
		fprintf(stderr, "Failed to start listener on %s: %s\n", PLUQ_URL_GAMEPLAY, nng_strerror(rv));
		nng_close(pub);
		return 1;
	}

	printf("Broadcasting on %s\n", PLUQ_URL_GAMEPLAY);
	printf("Waiting for subscribers...\n\n");

	// Give subscribers time to connect (warmup period)
	// PUB/SUB subscriptions need time to establish even after connection
	printf("Warmup: 5 seconds to ensure subscriptions are established...\n");
	sleep(5);

	printf("Sending 10 warmup frames (not counted)...\n");
	for (int warmup_frame = 0; warmup_frame < 10; warmup_frame++) {
		// Build and send a minimal warmup frame
		flatcc_builder_t builder;
		flatcc_builder_init(&builder);
		flatcc_builder_start_buffer(&builder, 0, 0, 0);

		PluQ_FrameUpdate_start(&builder);
		PluQ_FrameUpdate_frame_number_add(&builder, 0xFFFFFFFF);  // Special marker for warmup
		PluQ_FrameUpdate_timestamp_add(&builder, -1.0);
		PluQ_FrameUpdate_ref_t frame_ref = PluQ_FrameUpdate_end(&builder);

		PluQ_GameplayEvent_union_ref_t event = PluQ_GameplayEvent_as_FrameUpdate(frame_ref);

		PluQ_GameplayMessage_start_as_root(&builder);
		PluQ_GameplayMessage_event_add_value(&builder, event);
		PluQ_GameplayMessage_event_add_type(&builder, event.type);
		PluQ_GameplayMessage_end_as_root(&builder);

		size_t size;
		void *buf = flatcc_builder_finalize_buffer(&builder, &size);
		if (buf) {
			nng_msg *msg;
			if (nng_msg_alloc(&msg, size) == 0) {
				memcpy(nng_msg_body(msg), buf, size);
				nng_sendmsg(pub, msg, 0);  // Ignore errors during warmup
			}
			flatcc_builder_aligned_free(buf);
		}
		flatcc_builder_clear(&builder);

		usleep(16666);  // ~60 FPS
	}

	printf("Starting frame broadcast (100 frames)...\n\n");

	// Broadcast loop
	while (keep_running && frame_number < 100)  // Limit to 100 frames for testing
	{
		// Initialize FlatBuffers builder
		flatcc_builder_t builder;
		flatcc_builder_init(&builder);

		// Start buffer before building
		flatcc_builder_start_buffer(&builder, 0, 0, 0);

		// Build FrameUpdate
		PluQ_FrameUpdate_start(&builder);

		// Frame info
		PluQ_FrameUpdate_frame_number_add(&builder, frame_number);
		PluQ_FrameUpdate_timestamp_add(&builder, frame_number * 0.016);  // ~60fps

		// View state (simulate camera moving and rotating)
		PluQ_Vec3_t view_origin = {
			100.0f + frame_number * 0.1f,
			200.0f,
			50.0f
		};
		PluQ_Vec3_t view_angles = {
			0.0f,
			frame_number * 0.5f,  // Slowly rotating
			0.0f
		};
		PluQ_FrameUpdate_view_origin_add(&builder, &view_origin);
		PluQ_FrameUpdate_view_angles_add(&builder, &view_angles);

		// Player stats (simulate taking damage and using ammo)
		PluQ_FrameUpdate_health_add(&builder, (int16_t)(100 - (frame_number % 10)));
		PluQ_FrameUpdate_armor_add(&builder, (int16_t)(50 + (frame_number % 20)));
		PluQ_FrameUpdate_weapon_add(&builder, (uint8_t)((frame_number / 10) % 8));
		PluQ_FrameUpdate_ammo_add(&builder, (uint16_t)(100 - (frame_number % 50)));

		// Game state
		PluQ_FrameUpdate_paused_add(&builder, false);
		PluQ_FrameUpdate_in_game_add(&builder, true);

		PluQ_FrameUpdate_ref_t frame_ref = PluQ_FrameUpdate_end(&builder);

		// Wrap in GameplayMessage
		PluQ_GameplayEvent_union_ref_t event = PluQ_GameplayEvent_as_FrameUpdate(frame_ref);

		PluQ_GameplayMessage_start_as_root(&builder);
		PluQ_GameplayMessage_event_add_value(&builder, event);
		PluQ_GameplayMessage_event_add_type(&builder, event.type);
		PluQ_GameplayMessage_end_as_root(&builder);

		// Finalize buffer
		size_t size;
		void *buf = flatcc_builder_finalize_buffer(&builder, &size);

		if (buf)
		{
			// Send via nng (use nng_msg for proper memory management)
			nng_msg *msg;
			if ((rv = nng_msg_alloc(&msg, size)) == 0)
			{
				memcpy(nng_msg_body(msg), buf, size);

				if ((rv = nng_sendmsg(pub, msg, 0)) == 0)
				{
					if (frame_number % 10 == 0)
						printf("Frame %u: Sent %zu bytes\n", frame_number, size);
				}
				else
				{
					fprintf(stderr, "Failed to send frame %u: %s\n", frame_number, nng_strerror(rv));
					nng_msg_free(msg);
				}
			}

			flatcc_builder_aligned_free(buf);
		}

		flatcc_builder_clear(&builder);
		frame_number++;

		// ~60 FPS
		usleep(16666);
	}

	printf("\nBroadcast complete! Sent %u frames\n", frame_number);

	// Cooldown: Give subscribers time to receive last frames before closing socket
	printf("Cooldown: 2 seconds to flush buffers...\n");
	sleep(2);

	printf("Closing...\n");
	nng_close(pub);
	return 0;
}
