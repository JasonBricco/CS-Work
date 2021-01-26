# Jason Bricco
# CS4461 Program 1
# 9/21/20

import sys, os
from glob import *
from socket import *

port = 5000

if len(sys.argv) > 1:
	port = int(sys.argv[1])

# Lists information about each message.
names = []
files = []
sizes = []
deleted = []

# Returns the message number given as part of a command
# of the form xxxx num. Returns -1 if the message number
# does not refer to a valid message.
def message_num(cmd):
	if len(cmd) == 1:
		return -1
	else:
		num = int(cmd[1])

		if num < 1 or num > len(files):
			return -1

		return num - 1

# Returns the number of messages that are not marked
# as deleted.
def count_messages():
	count = 0

	for value in deleted:
		if not value:
			count += 1

	return count

# Get all files ending in .txt to load as messages.
for fileName in glob("*.txt"):
	names.append(fileName)
	file = open(fileName, 'r')
	files.append(file)
	sizes.append(os.fstat(file.fileno()).st_size)
	deleted.append(False)

# Create a new TCP connection socket.
sock = socket(AF_INET, SOCK_STREAM)
sock.bind(('', port))

# Enable the server to accept connections.
sock.listen(1)

running = True

while running:
	# Accept a connection from a client.
	conn, addr = sock.accept()

	# Keep communicating with the client.
	while True:
		data = conn.recv(4096).decode().lower()

		if data == '':
			break

		cmd = data.split()

		# STAT: Return the number of files and the total size of the files in bytes.
		# Deleted files are not included in the total.
		if cmd[0] == 'stat':
			result = '+OK ' + str(count_messages()) + ' ' + str(sum(sizes))

		# LIST: List each message number with its size in bytes.
		# If an argument is given, lists information about that particular message.
		# Does not list deleted messages.
		elif cmd[0] == 'list':
			num = message_num(cmd)

			if num == -1:
				result = '+OK Mailbox scan listing follows\n'

				for i in range(0, len(sizes)):
					if not deleted[i]:
						result += str(i + 1) + ' ' + str(sizes[i]) + '\n'

				result += '.'
			else:
				if deleted[num]:
					result = '-ERR That message has been deleted'
				else:
					result = '+OK ' + str(num + 1) + ' ' + str(sizes[num])

		# RETR: Retrieve the entire raw message, including headers.
		# Deleted messages cannot be retrieved.
		elif cmd[0] == 'retr':
			num = message_num(cmd)

			if num == -1:
				result = '-ERR Invalid message number'
			else:
				if deleted[num]:
					result = '-ERR That message has been deleted'
				else:
					result = '+OK ' + str(sizes[num]) + ' octets\n'

					# Read file lines into a list to be added to the result.
					files[num].seek(0)
					lines = files[num].readlines()

					for line in lines:
						result += line

					result += '\n.'

		# DELE: Delete a message.
		# Already deleted messages cannot be deleted again.
		# The message isn't actually deleted; it is only marked as such,
		# as per the spec (we aren't implementing the UPDATE state).
		elif cmd[0] == 'dele':
			num = message_num(cmd)
			
			if num == -1:
				result = '-ERR Invalid message number'
			else:
				if deleted[num]:
					result = '-ERR Message already deleted'
				else:
					deleted[num] = True
					files[num].close()
					sizes[num] = 0

					# Delete the file off disk, since the assignment description
					# says to do this.
					os.remove(names[num])

					result = '+OK Message deleted'

		# TOP: Returns the top (headers) of the specified message, as well as
		# the specified number of lines from the body.
		elif cmd[0] == 'top':
			num = message_num(cmd)

			if num == -1:
				result = '-ERR Invalid message number'
			else:
				if len(cmd) < 3:
					result = '-ERR Invalid number of body lines'
				else:
					bodyLines = int(cmd[2])

					if deleted[num]:
						result = '-ERR That message has been deleted'
					else:
						result = '+OK Top of message follows\n'

						files[num].seek(0)
						lines = files[num].readlines()

						bodyStart = 0

						# Add headers to the result.
						for i in range(0, len(lines)):
							if lines[i].isspace():
								bodyStart = i
								break

							result += lines[i]

						# Add body lines.
						if bodyLines > 0:
							end = min(bodyStart + bodyLines + 1, len(lines))

							for i in range(bodyStart, end):
								result += lines[i]

						if not result.endswith('\n'):
							result += '\n'

						result += '.'

		# QUIT: The client is disconnecting.
		elif cmd[0] == 'quit':
			result = '+OK'

		# Allows me to exit the server during my own testing.
		elif cmd[0] == 'kill':
			running = False
			result = '+OK'

		else:
			result = '-ERR Command is not recognized by the server'

		# Append \r\n to the result unless it's the final message.
		if result != '+OK':
			result += '\r\n'

		conn.send(result.encode())

# Close all files that aren't yet closed from being deleted.
for i in range(0, len(files)):
	if not deleted[i]:
		files[i].close()

sock.close()
