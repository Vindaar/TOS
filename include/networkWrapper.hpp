#ifndef NETWORKWRAPPER
#define NETWORKWRAPPER 1

// this file provides a wrapper for the standard network communication functions.
// used to give a common interface for windows and unix
// TODO: check which of these are actually needed!
#ifdef __WIN32__
# include <winsock2.h>
# include <ws2tcpip.h>
# include <fcntl.h>
# include <io.h>
# include <windows.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif

int sendWrapper(int sock, unsigned char* SendBuffer, int OutgoingLength, int flags);
int recvWrapper(int sock, unsigned char* RecvBuffer, int IncomingLength, int flags);
// TODO: think about whether to create a wrapper for select as well
int getsockoptWrapper(int sock, int level, int optname, int *sock_buf_size, socklen_t *optlen_pointer);
int setsockoptWrapper(int sock, int level, int optname, int *sock_buf_size, socklen_t optlen);

#endif
