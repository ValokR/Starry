Target Platform: Ubuntu 17.10
Language       : c++
Compiler       : CMake 3.8.2

This program initializes sets up a TCP connection listening on port 4000.
Upon receiving an in coming message, it parses that message into a header,
payload length, code and finally payload.  Then based on the code given, it
will execute the proper command.  Each command is terminated with a message
being sent back to the client via the TCP connection previously set up.  After
sending a response message, the program exits.

If given more time / resources I would add the ability for the server to
remain listening in between messages, rather than having to be restarted for
each new incoming message.
