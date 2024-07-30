# WinSock2-Chatroom

Allows users to communicate via a server using WinSock2.

# Starting the chatroom

Run `make serve` in a terminal to start the server.
In a second terminal, run `make join` to join the server as a client.
In a third terminal, run that command again.

# Using the chatroom

Before sending messages, you must log in.
Before logging in, the account must already exist.

Register a new account using the command 'newuser' followed by a username and a password, separated by spaces.
After registering, use the 'login' command with the same username and password.

To send a message, use the 'send' command followed by the target username (or 'all' to send to all users on server) and some message.

The 'logout' command will exit the server.

# Additional commands

In a terminal, run `make clear` to clear the saved user list. The server must be closed for this to work.
Run `make clean` to remove the .exe binaries and the user list.
