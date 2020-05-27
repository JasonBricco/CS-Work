// shared.h
// jmbricco
// CS3411 Program 5

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>

// Helper print macros for convenience.
#define ERROR(...) { \
	char out_buf[256]; \
	sprintf(out_buf, __VA_ARGS__); \
	write(2, out_buf, strlen(out_buf)); \
}

#define PRINT(...) { \
	char out_buf[256]; \
	sprintf(out_buf, __VA_ARGS__); \
	write(1, out_buf, strlen(out_buf)); \
}
