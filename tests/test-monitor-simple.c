#include <stdio.h>
#include <stdlib.h>
#include <nng/nng.h>
#include <nng/protocol/pubsub0/sub.h>
#include <signal.h>
#include <unistd.h>

#define PLUQ_URL_GAMEPLAY "tcp://127.0.0.1:9002"

static int running = 1;

void signal_handler(int sig)
{
	running = 0;
}

int main(void)
{
	int rv;
	nng_socket sub;

	printf("Simple Monitor - Counting messages\n\n");

	signal(SIGINT, signal_handler);

	if ((rv = nng_sub0_open(&sub)) != 0) {
		fprintf(stderr, "Failed to create SUB socket: %s\n", nng_strerror(rv));
		return 1;
	}

	if ((rv = nng_sub0_socket_subscribe(sub, "", 0)) != 0) {
		fprintf(stderr, "Failed to subscribe: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	nng_dialer dialer;
	if ((rv = nng_dialer_create(&dialer, sub, PLUQ_URL_GAMEPLAY)) != 0) {
		fprintf(stderr, "Failed to create dialer: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	if ((rv = nng_dialer_start(dialer, 0)) != 0) {
		fprintf(stderr, "Failed to start dialer: %s\n", nng_strerror(rv));
		nng_close(sub);
		return 1;
	}

	printf("Connected! Receiving...\n");

	int count = 0;
	int timeout = 0;

	while (running && timeout < 600) {
		nng_msg *msg;
		if ((rv = nng_recvmsg(sub, &msg, NNG_FLAG_NONBLOCK)) == 0) {
			count++;
			if (count % 10 == 0 || count <= 5) {
				printf("Message %d: %zu bytes\n", count, nng_msg_len(msg));
			}
			nng_msg_free(msg);
			timeout = 0;
		} else if (rv != NNG_EAGAIN) {
			fprintf(stderr, "Error: %s\n", nng_strerror(rv));
			break;
		} else {
			timeout++;
		}
		usleep(16666);
	}

	printf("\nTotal: %d messages\n", count);
	printf("Expected: 110 (10 warmup + 100 real)\n");
	printf("Success: %.1f%%\n", (count * 100.0) / 110.0);

	nng_close(sub);
	return 0;
}
