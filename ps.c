/**
 * gcc -DNUM_THREADS=1 mt-ps.c -o mt-ps
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define handle_error_en(en, msg)    \
	do {                        \
		errno = en;         \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	} while (0)

#define handle_error(msg)           \
	do {                        \
		perror(msg);        \
		exit(EXIT_FAILURE); \
	} while (0)

struct host_info {
	char *host;
	long start_port;
	long end_port;
};

static void *scanner(void *arg)
{
	struct host_info *hinfo = arg;
	struct sockaddr_in addr;
	struct timeval timeout;
	int socket_fd;

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, hinfo->host, &addr.sin_addr) <= 0)
		handle_error("inet_pton failed");

	for (int port = hinfo->start_port; port <= hinfo->end_port; port++) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (socket_fd == -1)
			handle_error("socket");

		if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO,
			       (struct timeval *)&timeout,
			       sizeof(struct timeval)) < 0) {
			close(socket_fd);
			handle_error("setsockopt failed");
		}

		addr.sin_port = htons(port);
		if (connect(socket_fd, (struct sockaddr *)&addr, sizeof addr) !=
		    -1) {
			printf("port %d open\n", port);
			close(socket_fd);
		}
		close(socket_fd);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	long start_port, end_port;
	struct host_info hinfo;
	char *endptr, *range;

	if (argc < 3) {
		fprintf(stderr, "Usage: %s host start_port:end_port\n",
			argv[0]);
		return 1;
	}

	range = argv[2];
	start_port = strtol(range, &endptr, 10);

	if (*endptr == ':') {
		end_port = strtol(endptr + 1, &endptr, 10);
		if (*endptr != '\0') {
			fprintf(stderr, "Usage: %s host start_port:end_port\n",
				argv[0]);
			return 1;
		}
	} else if (*endptr == '\0')
		end_port = start_port;
	else {
		fprintf(stderr, "Usage: %s host start_port:end_port\n",
			argv[0]);
		return 1;
	}

	hinfo.host = argv[1];
	hinfo.start_port = start_port;
	hinfo.end_port = end_port;
	scanner(&hinfo);

	return 0;
}
