// step3.c
// jmbricco
// CS3411 Program 4

#include "shared.h"

typedef struct 
{
	char* baseName;
	int out, err;
	bool outExists, errExists;
} OutFiles;

// Opens a file descriptor for the given file name,
// appending the extension 'ext' to the end.
static int OpenOutputFile(char* fileName, char* ext)
{
	char buf[255] = {0};
	strcat(strcat(buf, fileName), ext);
	return open(buf, O_WRONLY | O_CREAT, 0644);
}

static int ReadFrom(OutFiles* files, int desc, int target)
{
	char buf[256];

	int bytesRead = read(desc, buf, 256);
	
	if (bytesRead < 0)
	{
		ERROR("Failed to read from descriptor %d\n", desc);
		exit(-1);
	}

	if (bytesRead > 0)
	{
		// Use a new buffer to display output from.
		// Non-printable characters are translated into 4
		// characters of the form <xx> (hex).
		char outBuf[1024];

		// Tracks where we are in the buffer (how many
		// characters have been written so far).
		int strP = 0;

		for (int i = 0; i < bytesRead; ++i)
		{
			char c = buf[i];

			// 32-126 is the printable range.
			if ((int)c >= 32 && (int)c <= 126)
			{
				sprintf(outBuf + strP, "%c", c);
				++strP;
			}
			else 
			{
				sprintf(outBuf + strP, "<%02x>", c);
				strP += 4;
			}
		}

		if (write(target, outBuf, strP) < 0)
		{
			ERROR("Failed to write results from descriptor %d to stdout\n", desc);
			exit(-1);
		}

		if (target == 1)
		{
			if (!files->outExists)
			{
				files->out = OpenOutputFile(files->baseName, ".stdout");

				if (files->out < 0)
				{
					ERROR("Failed to open the output file for stdout.\n");
					exit(-1);
				}

				files->outExists = true;
			}

			if (write(files->out, outBuf, strP) < 0)
			{
				ERROR("Failed to write to output file\n");
				exit(-1);
			}
		}
		else if (target == 2)
		{
			if (!files->errExists)
			{
				files->err = OpenOutputFile(files->baseName, ".stderr");

				if (files->err < 0)
				{
					ERROR("Failed to open the output file for stderr.\n");
					exit(-1);
				}

				files->errExists = true;
			}

			if (write(files->err, outBuf, strP) < 0)
			{
				ERROR("Failed to write to error file\n");
				exit(-1);
			}
		}
	}

	return bytesRead;
}

int main(int argc, char** argv)
{
	int out[2];
	int err[2];

	// Create pipes to communicate between the parent process
	// and child that intercepts the command output.
	if (pipe(out) < 0)
	{
		ERROR("Failed to create the output pipe.\n");
		return -1;
	}

	if (pipe(err) < 0)
	{
		ERROR("Failed to create the error pipe.\n");
		return -1;
	}

	pid_t result = fork();

	if (result == 0)
	{
		// Child doesn't need the read ends of the pipes.
		close(out[0]);
		close(err[0]);

		// Re-map stdout/stderr for the child to the
		// pipes created by the parent (which are
		// inherited here).
		if (dup2(out[1], 1) < 0)
		{
			ERROR("Failed to map stdout in the child process. Error: %s\n", strerror(errno));
			return -1;
		}

		if (dup2(err[1], 2) < 0)
		{
			ERROR("Failed to map stderr in the child process. Error: %s\n", strerror(errno));
			return -1;
		}

		// Advance argv to begin at the program name
		// we're going to exec into.
		++argv;

		// Allows us to preserve our environment variables.
		extern char** environ;

		execvpe(argv[0], argv, environ);

		ERROR("execve failed! Tried to begin process %s. Error: %s\n", argv[0], strerror(errno));
		return -1;
	}
	else if (result == -1)
	{
		ERROR("Failed to fork the child process. Error: %s\n", strerror(errno));
		return -1;
	}
	else
	{
		// Close parents' write end of the file. Only the child needs to write.
		close(out[1]);
		close(err[1]);

		OutFiles files = {0};
		files.baseName = argv[1];

		fd_set set;

		while (1)
		{
			// Create a descriptor set for select(). It starts empty,
			// and receives the stdin and fifo descriptors.
			FD_ZERO(&set);

			FD_SET(out[0], &set);
			FD_SET(err[0], &set);

			struct timeval timeout = { 5.0f };

			// Highest file descriptor, for use in select().
			int highest = out[0] > err[0] ? out[0] : err[0];

			// Wait for changes in one of the watched descriptors.
			int result = select(highest + 1, &set, NULL, NULL, &timeout);

			if (result < 0)
			{
				ERROR("Failed to select. Error: %s\n", strerror(errno));
				return -1;
			}
			else if (result == 0)
			{
				// Timeout occurred.
				char* out = "tick\n";
				write(1, out, strlen(out));
			}
			else
			{
				// Check if reads on the out and err descriptors
				// will block or not. If not, we will perform the read.
				// If we are told we can read but find nothing to read,
				// the file descriptors must be closed and we can exit.
				if (FD_ISSET(out[0], &set))
				{
					if (ReadFrom(&files, out[0], 1) == 0)
						break;
				}

				if (FD_ISSET(err[0], &set))
				{
					if (ReadFrom(&files, err[0], 2) == 0)
						break;
				}
			}
		}
	}
}
