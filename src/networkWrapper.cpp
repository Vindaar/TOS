// implementation of the networkWrapper function for a common interface for Unix and Windows
#include "networkWrapper.hpp"

int sendWrapper(int sock, unsigned char* SendBuffer, int OutgoingLength, int flags){
    // this function needs to perform the following:
    // in case of unix, it only needs to call the send() function with the arguments given to this
    // function
    // for windows however: the windows send() has the following syntax:
    // int send(
    //   _In_       SOCKET s,
    //   _In_ const char   *buf,
    //   _In_       int    len,
    //   _In_       int    flags
    // );
    // that means we first need to convert the unsigned char* to a const char* and
    // after calling the function convert the result back to an unsigned char*
    int recvBytes;

#ifdef __WIN32__
    // create a char * windows variable
    char *winSendBuffer;
    // perform a static cast from unsigned to signed char*
    winSendBuffer = reinterpret_cast<char *> (SendBuffer);
    // call windows function with char *
    recvBytes = send(sock, winSendBuffer, OutgoingLength, flags);
    // convert char * back to unsigned char *
    SendBuffer = reinterpret_cast<unsigned char *> (winSendBuffer);
#else
    recvBytes = send(sock, SendBuffer, OutgoingLength, flags);
#endif
    
    return recvBytes;
}


int recvWrapper(int sock, unsigned char* RecvBuffer, int IncomingLength, int flags){
    // this function needs to perform the following:
    // in case of unix, it only needs to call the recv() function with the arguments given to this
    // function
    // for windows however: the windows recv() has the following syntax:
    // int recv(
    //   _In_  SOCKET s,
    //   _Out_ char   *buf,
    //   _In_  int    len,
    //   _In_  int    flags
    // );
    // that means we first need to convert the unsigned char* to a const char* and
    // after calling the function convert the result back to an unsigned char*
    int recvBytes;

#ifdef __WIN32__
    // same procedure as in sendWrapper()
    char *winRecvBuffer;
    winRecvBuffer = reinterpret_cast<char *> (RecvBuffer);
    recvBytes = recv(sock, winRecvBuffer, IncomingLength, flags);
    RecvBuffer = reinterpret_cast<unsigned char*> (winRecvBuffer);
#else
    recvBytes = recv(sock, RecvBuffer, IncomingLength, flags);
#endif

    return recvBytes;
}

int getsockoptWrapper(int sock, int level, int optname, int *sock_buf_size_pointer, socklen_t *optlen_pointer){
    // this function acts as wrapper for the getsockopt function to have a common interface
    // for windows and unix
    // int sock: socket number
    // int level: socket level
    // int optname: send or receive buffer
    // int* sock_buf_size_pointer: pointer to int in which socket buffer size is saved
    // socklen_t *optlen_pointer: pointer to size of socket buffer length variable
    int nbytes;

#ifdef __WIN32__
    // similar procedure to sendWrapper()
    char *win_sock_buf_size_pointer;
    win_sock_buf_size_pointer = reinterpret_cast<char *> (sock_buf_size_pointer);
    nbytes = getsockopt(sock, level, optname, win_sock_buf_size_pointer, optlen_pointer);
    sock_buf_size_pointer = reinterpret_cast<int *> (win_sock_buf_size_pointer);
#else
    nbytes = getsockopt(sock, level, optname, sock_buf_size_pointer, optlen_pointer);
#endif
    
    return nbytes;
}

int setsockoptWrapper(int sock, int level, int optname, int *sock_buf_size_pointer, socklen_t optlen){
    // this function acts as wrapper for the setsockopt function to have a common interface
    // for windows and unix
    // int sock: socket number
    // int level: socket level
    // int optname: send or receive buffer
    // int* sock_buf_size_pointer: pointer to int in which socket buffer size is saved
    // socklen_t optlen: size of socket buffer length variable
    int nbytes;

#ifdef __WIN32__
    // similar procedure to sendWrapper()
    char *win_sock_buf_size_pointer;
    win_sock_buf_size_pointer = reinterpret_cast<char *> (sock_buf_size_pointer);
    nbytes = setsockopt(sock, level, optname, win_sock_buf_size_pointer, optlen);
    sock_buf_size_pointer = reinterpret_cast<int *> (win_sock_buf_size_pointer);
#else
    nbytes = setsockopt(sock, level, optname, sock_buf_size_pointer, optlen);
# endif
    
    return nbytes;
}
