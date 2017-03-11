# Windows-TCPIP-Server
This TCP IP server is able to handle multiple clients. When a client connects and sends a message, the server broadcasts exactly what is received from the client (there is no special client managing). It is up the clients to display their own username. This server is limited to 512 character messages. Clients who unexpectedly disconnect are automatically removed from the broadcastig list. 
# Resources
I used the following resources for creating the server:<br/>
http://www.bogotobogo.com/cplusplus/multithreading_win32A.php<br/>
https://msdn.microsoft.com/en-us/library/windows/desktop/ms738545(v=vs.85).aspx<br/>
# Compile
When you compile, you must link to ws2_32; it contains the socket library required. 
