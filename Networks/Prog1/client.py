# Jason Bricco
# CS4461 Program 1
# 9/21/20

import sys
from socket import *

server = sys.argv[1];
port = 5000

if len(sys.argv) > 2:
	port = int(sys.argv[2])

# Connect to the server for this session.
sock = socket(AF_INET, SOCK_STREAM)
sock.connect((server, port))

while True:
	cmd = input('Enter a command: ')

	# Send the command to the server in encoded form,
	# then wait for the server's response.
	sock.send(cmd.encode())
	result = sock.recv(4096).decode()

	print(result)

	# Exit if the user typed 'quit' or 'kill'.
	if (cmd == 'QUIT'.lower() or cmd == 'KILL'.lower()):
		break

sock.close()
