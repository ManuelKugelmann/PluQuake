/*
 * PluQ Monitor - Simple test program to receive and display PluQ frames
 * 
 * Connects to a PluQ backend and prints received frame data
 */

#include <stdio.h>
#include <stdlib.h>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include "pluq_reader.h"

#define PLUQ_URL_GAMEPLAY "tcp://127.0.0.1:9002"

static int running = 1;

void signal_handler(int sig)
{
	printf("\nReceived signal %d, shutting down...\n", sig);
	running = 0;
}

int main(void)
{
	int rv;
	nng_socket sub;
	nng_dialer dialer;

	printf("PluQ Monitor - FlatBuffers Frame Receiver\n");
	printf("==========================================\n\n");

	// Setup signal handler
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	// Open SUB socket (nng 1.x API)
	if ((rv = nng_sub0_open(&sub)) != 0)
	{
		fprintf(stderr, "Failed to create SUB socket: %s\n", nng_strerror(rv));
		return 1;
	}

	// Subscribe to all topics (nng 1.x API)
	if ((rv = nng_sub0_socket_subscribe(sub, "", 0)) != 0)
	{
		fprintf(stderr, "Failed to subscribe: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	// Connect to backend using dialer API (nng 1.x)
	printf("Connecting to %s...\n", PLUQ_URL_GAMEPLAY);
	if ((rv = nng_dialer_create(&dialer, sub, PLUQ_URL_GAMEPLAY)) != 0)
	{
		fprintf(stderr, "Failed to create dialer: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	if ((rv = nng_dialer_start(dialer, 0)) != 0)
	{
		fprintf(stderr, "Failed to start dialer: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	printf("Connected! Waiting for frames...\n");
	printf("Skipping warmup frames (marked with frame_number 0xFFFFFFFF)...\n\n");

	uint32_t warmup_count = 0;
	uint32_t real_frame_count = 0;
	uint32_t expected_frames = 100;
	int warmup_phase = 1;

	// Receive loop - run for a bit longer than expected
	// Needs to wait for: warmup (3s) + warmup frames (0.17s) + real frames (1.67s) + buffer time (2s) = ~7s
	int timeout_counter = 0;
	int max_timeout = 600;  // ~10 seconds at 60 FPS to account for all delays

	int first_receive = 1;
	while (running && timeout_counter < max_timeout)
	{
		nng_msg *msg;

		if ((rv = nng_recvmsg(sub, &msg, NNG_FLAG_NONBLOCK)) == 0)
		{
			size_t size = nng_msg_len(msg);

			// Parse FlatBuffers to check if it's a warmup frame
			PluQ_GameplayMessage_table_t gameplay_msg = PluQ_GameplayMessage_as_root(nng_msg_body(msg));
			PluQ_GameplayEvent_union_type_t event_type = PluQ_GameplayMessage_event_type(gameplay_msg);

			if (first_receive) {
				printf("First message received!\n");
				printf("Message size: %zu, event_type: %d (expected %d for FrameUpdate)\n",
					size, event_type, PluQ_GameplayEvent_FrameUpdate);
				first_receive = 0;
			}

			if (event_type == PluQ_GameplayEvent_FrameUpdate) {
				PluQ_FrameUpdate_table_t frame = PluQ_GameplayMessage_event(gameplay_msg);
				uint32_t frame_number = PluQ_FrameUpdate_frame_number(frame);

				if (frame_number == 0xFFFFFFFF) {
					// Warmup frame - don't count
					warmup_count++;
					if (warmup_count == 1) {
						printf("Receiving warmup frames...\n");
					}
				} else {
					// Real frame - count it
					real_frame_count++;
					if (warmup_phase) {
						warmup_phase = 0;
						printf("Warmup complete! Counting real frames...\n\n");
					}
					if (real_frame_count % 10 == 0 || real_frame_count <= 5 || real_frame_count >= 96) {
						printf("Frame %u: Received %zu bytes (seq %u)\n",
							real_frame_count, size, frame_number);
					}
				}
			}

			nng_msg_free(msg);
			timeout_counter = 0;  // Reset timeout on successful receive
		}
		else if (rv != NNG_EAGAIN)
		{
			fprintf(stderr, "Receive error: %s\n", nng_strerror(rv));
			break;
		}
		else
		{
			timeout_counter++;
			if (timeout_counter == 100) {
				printf("Still waiting... (%d/600)\n", timeout_counter);
			}
		}

		usleep(16666);  // ~60 FPS
	}

	printf("\n=== Results ===\n");
	printf("Warmup frames: %u\n", warmup_count);
	printf("Real frames received: %u\n", real_frame_count);
	printf("Expected frames: %u\n", expected_frames);
	if (real_frame_count == expected_frames) {
		printf("✓ SUCCESS: All frames received!\n");
	} else {
		printf("✗ MISSED: %d frames (%.1f%% success)\n",
			expected_frames - real_frame_count,
			(real_frame_count * 100.0) / expected_frames);
	}
	nng_close(sub);
	return 0;
}
