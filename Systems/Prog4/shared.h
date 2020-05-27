// shared.h
// jmbricco
// CS3411 Program 4

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// Helper print macro for convenience.
#define ERROR(...) { \
	char out_buf[256]; \
	sprintf(out_buf, __VA_ARGS__); \
	write(2, out_buf, strlen(out_buf)); \
}
