// logger.c
// jmbricco
// CS3411 Program 5

#include "shared.h"

// Connection status.
#define UNCONNECTED 0
#define CONNECTED 1
#define LISTING 2

#define DESC_COUNT 32
#define BLOCK_SIZE 128

// Sends input from stdin to stdout.
static int SendToOut()
{
	char buf[256];

	int bytesRead;

	// Read until EOF in chunks of 256, writing what we read to stdout.
	do
	{
		bytesRead = read(0, buf, 256);

		if (bytesRead < 0)
		{
			ERROR("Failed to read from stdin. Error: %s\n", strerror(errno));
			exit(-1);
		}

		if (bytesRead > 0)
		{
			if (write(1, buf, bytesRead) < 0)
			{
				ERROR("Failed to write to stdout. Error %s\n", strerror(errno));
				exit(-1);
			}
		}
		else return -1;
	}
	while (bytesRead > 0);

	return bytesRead;
}

// Close the server when all connections have been closed (for testing).
static void CloseIfNoConnections(int* connections, bool* isDone)
{
	for (int i = 0; i < DESC_COUNT; ++i)
	{
		if (connections[i] != UNCONNECTED)
			return;
	}

	*isDone = true;
}

// Displays a message to the given descriptor.
static void WriteLogResult(int desc, char* msg)
{
	if (write(desc, msg, strlen(msg)) < 0)
	{
		ERROR("Failed to write log command result. Error %s\n", strerror(errno));
		exit(-1);
	}
}

// Displays the "log #:" prompt to the given descriptor.
static void WritePrompt(int desc)
{
	char* msg = "log #: ";

	if (write(desc, msg, strlen(msg)) < 0)
	{
		ERROR("Failed to write the prompt. Error %s\n", strerror(errno));
		exit(-1);
	}
}

// Outputs a block of 128 characters from the server log file.
// Text starts at 'pos' in the log file, and is sent to the 'target'
// descriptor. 
static int OutputBlock(int logFile, int target, char* buf, int pos)
{
	// Save where we are in the file, then seek to where we want to read from.
	off_t cur = lseek(logFile, 0, SEEK_CUR);
	off_t next = lseek(logFile, pos, SEEK_SET);

	if (cur == (off_t)-1 || next == (off_t)-1)
	{
		ERROR("Failed to seek the log file. Error: %s\n", strerror(errno));
		exit(-1);
	}

	int bytesRead = read(logFile, buf, BLOCK_SIZE);

	if (bytesRead < 0)
	{
		ERROR("Failed to read from the log file (descriptor %d). Error: %s\n", logFile, strerror(errno));
		exit(-1);
	}

	if (write(target, buf, bytesRead) < 0)
	{
		ERROR("Failed to write the log contents to the client. Error: %s\n", strerror(errno));
		exit(-1);
	}

	// Seek back to where we were before now that the read is finished.
	if (lseek(logFile, cur, SEEK_SET) == (off_t)-1)
	{
		ERROR("Failed to seek the log file. Error: %s\n", strerror(errno));
		exit(-1);
	}

	return bytesRead;
}

int main(int argc, char** argv)
{
	// Establishes a communication endpoint and returns a file descriptor to it.
	int listener = socket(AF_INET, SOCK_STREAM, 0);

	if (listener < 0)
	{
		ERROR("Failed to create the socket. Error: %s\n", strerror(errno));
		return -1;
	}

	struct sockaddr_in s1;

	bzero((char*)&s1,  sizeof(s1));
	s1.sin_family = (short)AF_INET;
	s1.sin_addr.s_addr = htonl(INADDR_ANY);
	s1.sin_port = htons(0);

	// Assign the sockaddr_in address to the socket.
	if (bind(listener, (struct sockaddr*)&s1, sizeof(s1)) < 0)
	{
		ERROR("Failed to bind to the socket. Erorr: %s\n", strerror(errno));
		return -1;
	}

	int len = sizeof(s1);

	// Return the address at which the socket is bound.
	if (getsockname(listener, (struct sockaddr*)&s1, (socklen_t*)&len) < 0)
	{
		ERROR("getsockname failed. Error: %s\n", strerror(errno));
		return -1;
	}

	PRINT("Port Number: %d\n", ntohs(s1.sin_port));

	// Allow the socket to accept incoming connection requests.
	listen(listener, 1);

	// Represents connection status.
	int connections[DESC_COUNT] = {0};

	// Stores file positions while listing files to the client.
	int positions[DESC_COUNT] = {0};

	// Log file descriptor.
	int logFile = -1;
	
	// Server closes when this is true.
	bool isDone = false;

	while (!isDone)
	{
		// Create a set with descriptors we want to wait for 
		// changes on.
		fd_set set;

		FD_ZERO(&set);
		FD_SET(listener, &set);
		FD_SET(0, &set);

		int highest = listener;

		// Add open connection descriptors to the set.
		for (int i = 0; i < DESC_COUNT; ++i)
		{
			// Only add connected descriptors to the set.
			if (connections[i] != UNCONNECTED)
			{
				FD_SET(i, &set);
				highest = i;
			}
		}

		// Time out in 30 seconds.
		struct timeval timeout = { 30.0f };

		// Wait for input or for a new connection.
		int result = select(highest + 1, &set, NULL, NULL, &timeout);

		if (result < 0)
		{
			ERROR("Failed to select. Error: %s\n", strerror(errno));
			return -1;
		}
		else if (result > 0)
		{
			// Establish a connection if there is input in the listener.
			if (FD_ISSET(listener, &set))
			{
				struct sockaddr_in s2;
				len = sizeof(s2);

				// Attempt to accept a connection request.
				int conn = accept(listener, (struct sockaddr*)&s2, (socklen_t*)&len);

				if (conn < 0)
				{
					ERROR("Failed to accept a connection. Error: %s\n", strerror(errno));
					return -1;
				}

				// Add this connection to our list of connections.
				connections[conn] = CONNECTED;
				WritePrompt(conn);
			}
			else if (FD_ISSET(0, &set))
			{
				// Send stdin input to stdout.
				SendToOut();
			}
			else
			{
				for (int i = 0; i < DESC_COUNT; ++i)
				{
					if (connections[i] != UNCONNECTED && FD_ISSET(i, &set))
					{
						char buf[1048] = {0};
						
						int bytesRead = read(i, buf, sizeof(buf));

						// This signals the connection is closed.
						if (bytesRead == 0)
						{
							connections[i] = UNCONNECTED;
							CloseIfNoConnections(connections, &isDone);
						}
						else if (bytesRead < 0)
						{
							ERROR("Failed to read the command from the client. Error: %s\n", strerror(errno));
							return -1;
						}
						else
						{
							if (strncmp(buf, "log", 3) == 0)
							{
								// Typing log with space after is 4 characters. If that's all we have, there
								// is nothing to add to the log file.
								if (bytesRead > 4)
								{
									WriteLogResult(i, "#log: logging\n");

									// Create the log file if it hasn't been created yet.
									if (logFile == -1)
										logFile = open("Log.txt", O_CREAT | O_RDWR | O_TRUNC, 0644);

									if (write(logFile, buf + 4, bytesRead - 4) < 0)
									{
										ERROR("Failed to write to the server log file. Error %s\n", strerror(errno));
										return -1;
									}
								}
							}
							else if (strncmp(buf, "list", 4) == 0)
							{
								positions[i] = 0;

								if (logFile != -1)
								{
									WriteLogResult(i, "#log: listing\n");

									char logBuf[BLOCK_SIZE];
									int bytesRead = OutputBlock(logFile, i, logBuf, positions[i]);

									if (bytesRead == BLOCK_SIZE)
									{
										connections[i] = LISTING;

										// Increment where we are in the positions array for the sake of
										// the next read.
										positions[i] += bytesRead;

										// Add a new line if we haven't read everything so the prompt
										// isn't on the same line as the listing.
										if (write(i, "\n", 1) < 0)
										{
											ERROR("Failed to write a newline to the client. Error: %s\n", strerror(errno));
											return -1;
										}
									}
								}
							}
							else if (connections[i] == LISTING && strchr(buf, '\n') != NULL)
							{
								// Only handle the more command if we're in listing state.
								
								if (logFile != -1)
								{
									WriteLogResult(i, "#log: more\n");

									char logBuf[BLOCK_SIZE];
									int bytesRead = OutputBlock(logFile, i, logBuf, positions[i]);

									// No more to list, exit listing state.
									if (bytesRead < BLOCK_SIZE)
									{
										connections[i] = CONNECTED;
										positions[i] = 0;
									}
									else 
									{
										positions[i] += bytesRead;

										if (write(i, "\n", 1) < 0)
										{
											ERROR("Failed to write a newline to the client. Error: %s\n", strerror(errno));
											return -1;
										}
									}
								}
							}
							else WriteLogResult(i, "#log: Command not recognized.\n");

							WritePrompt(i);
						}
					}
				}
			}
		}
	}
}
