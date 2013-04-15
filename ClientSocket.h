//ClientSocket class header file
//Course: 4050-212-02
//Authors: Luke Matarazzo and Jackson Sadowski
//Filename: ClientSocket.h

#include "Socket.h"

class ClientSocket : public Socket
{
    public:
        void connectToServer(const char*, int); //connects to server
        void getFile();
        void listFiles();
};