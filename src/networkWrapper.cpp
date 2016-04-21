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
    recvBytes = recv(sock, winRecvBuffer, IncomingLength, 0);
    RecvBuffer = reinterpret_cast<unsigned char*> (winRecvBuffer);
#else
    recvBytes = recv(sock, RecvBuffer, IncomingLength, 0);
#endif     

    return recvBytes;
}
