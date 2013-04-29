//Course: 4050-212-02
//Authors: Alex Buie, Luke Matarazzo, Jackson Sadowski, Steven Tucker
//IDE/editor: Sublime Text 2
//Compilers: mingw32-g++
//Final SMTP Project
//Filename: TSMain.cpp   server main
//Purpose: 

#define _WIN32_WINNT 0x501
#include <cstdlib>
#include <cctype>
#include "ThreadSock.h"
#include "Status.h"

using namespace std;

// our thread for recving commands
DWORD WINAPI handleMail(LPVOID lpParam)
{
    cout << "Thread Created\n";

    //set our socket to the socket passed in as a parameter
    ThreadSock current_client;

    current_client.setSock((SOCKET)lpParam);

    string recMessage = ""; //will hold the command the client sent

    //Set and send the welcome message
    current_client.sendResponse(Status::SMTP_SRV_RDY, "stimpy 0.0.1/flopcity - Welcome"); //send initiation hello

    current_client.recvData(recMessage); //get data from the client

    //checking out the string to see if it's helo
	bool bHeloSent = FALSE;
	do{
		if (recMessage.substr(0,4) == "HELO") //if the first word is helo
		{
			current_client.sendData(Status::SMTP_ACTION_COMPLETE); //send back 250 that it's good
			cout << "Connection Successful. We received a HELO from the client.\n";
			bHeloSent = TRUE;

		}    
		else //if it's not HELO, return error code
		{
			current_client.sendData(Status::SMTP_CMD_SNTX_ERR); //sending the error code
			cout << "Connection Failed. We did not recieive a HELO from client...\n";
			current_client.recvData(recMessage); //get data from the client
		}
	} while (!bHeloSent);

    current_client.recvData(recMessage); //recieving the verify and a username
    string username;

    //checking to see if it's a verify
    if (recMessage.substr(0,4) == "VRFY")
    {
        username = recMessage.substr(5); //trim the username

        //if it is, validate the username and continue
        if (current_client.validateUser(username))
        {
            current_client.sendData(Status::SMTP_ACTION_COMPLETE); //if the username was valid, send back 250
        }
        else
        {
            current_client.sendData(Status::SMTP_MBOX_UNAV);
        }
    }

    //getting data from the client
    int clientFlop = 0; //will hold the value that the recv function returns
    clientFlop = current_client.recvData(recMessage);

    //our send/recv loop
    while(recMessage != "QUIT" || recMessage != "Quit" || recMessage != "quit")
    {
		bool bRecipientSent = FALSE;
		string sRecipient = "";

        if(recMessage.substr(0,9) == "MAIL FROM")
        {
            cout << "Client Sent: " << recMessage << endl; //for debugging
            current_client.sendData(Status::SMTP_ACTION_COMPLETE);

            //get more data from client
            clientFlop = current_client.recvData(recMessage);

            //rcpt to section
            if (recMessage.substr(0,7) != "RCPT TO") //checking to see if it's RCPT TO
            {
                cout << "RCPT TO wasn't sent...\n"; //should probably sent syntax error back
                current_client.sendData(Status::SMTP_CMD_SNTX_ERR); //send error
            }
            else 
            {
                cout << "Client Send: " << recMessage << endl; //for debugging
                sRecipient = recMessage.substr(9, recMessage.find("@")-9);

                //cout << "recipient username: " << recMessage.substr(9, recMessage.find("@")-9) << endl; //for debugging
                //checking to see if the user is valid
                if (!current_client.validateUser(sRecipient))
                {
                    current_client.sendResponse(Status::SMTP_CMD_SNTX_ERR, "Malformed Recipient");
                }
                //sending back a bad error code
                else
                {
                    current_client.sendResponse(Status::SMTP_ACTION_COMPLETE, "OK");//if the username was valid, send back 250
                    bRecipientSent = TRUE;

                    //getting data and writing to file part
                    //cout << "before recvdata: " << recMessage << endl;
                    clientFlop = current_client.recvData(recMessage); //getting more data from client
                    //cout << "after recvdata " << recMessage << endl;

                    //checking to see if the string is DATA
                    if (recMessage.substr(0,6) != "DATA")
                    {
                        cout << "DATA wasn't received\n";
                        current_client.sendData(Status::SMTP_CMD_SNTX_ERR); //send error
                    }
                    else
                    {
                        //get the data of the message part
                        //create file output object and open it in append mode
                        ofstream fout;
                        fout.open ((string(sRecipient + ".txt")).c_str(), ios::app);

                        //write the initial part of the email
                        
                        //tell client to send data, then get data and write to file
                        current_client.sendResponse(Status::SMTP_BEGIN_MSG,"OK -- Send Data");
                        clientFlop = current_client.recvData(recMessage); //getting a line from the client

                        while (recMessage != ".") //while line !=. we want to keep getting message data from the client
                        {
                            if(clientFlop == -1)
                                break;

                            fout << recMessage; //write line to file
                            if(recMessage != "\n")
                                fout << endl;

                            cout << "Message: " << recMessage << endl;

                            clientFlop = current_client.recvData(recMessage); //getting next line from the user
                        }

                        //send status code that action is complete and close the file
                        current_client.sendData(Status::SMTP_ACTION_COMPLETE);
                        fout.close();
                    }
                }
            }
        }

        else if(recMessage.substr(0,5) == "INBOX") //if they send an inbox and want to check their inbox
        {
            current_client.sendResponse(Status::SMTP_ACTION_COMPLETE, "OK"); //send 250 OK so they know we got the command okay, then send mail

        }

        else
        {
            cout << "Client didn't send 'MAIL FROM' or 'INBOX' in the beginning\n";
            current_client.sendData(Status::SMTP_CMD_SNTX_ERR);
        }

        //get data from the client before starting loop again
        clientFlop = current_client.recvData(recMessage);

        if(recMessage == "QUIT" || clientFlop == -1) //if they sent quit, break from while loop and the thread will end after exiting this
        {
            current_client.sendResponse(Status::SMTP_SRV_CLOSE, "OK -- Goodbye...");
            break;
        }   
        
    } //end of while
}

int main()
{
    cout << "Starting up multi-threaded SMTP server\n";

    //our masterSocket(socket that listens for connections)
    SOCKET sock;
    WSADATA wsaData;
    sockaddr_in server;

    //start winsock
    int ret = WSAStartup(0x101,&wsaData); //use highest version of winsock avalible

    if(ret != 0)
    {
        return 0;
    }

    //fill in winsock struct ...
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(31000); //listen on telnet port 31000

    //create our socket
    sock=socket(AF_INET,SOCK_STREAM,0);

    if(sock == INVALID_SOCKET)
    {
        return 0;
    }

    //bind our socket to a port(port 123)
    if(bind(sock,(sockaddr*)&server,sizeof(server)) !=0)
    {
        return 0;
    }

    //listen for a connection
    if(listen(sock,5) != 0)
    {
        return 0;
    }

    //for our thread
    DWORD thread;

    //socket that we sendrecv data on
    SOCKET client;

    sockaddr_in from;
    int fromlen = sizeof(from);

    //loop forever
    while(true)
    {
        //accept connections
        client = accept(sock,(struct sockaddr*)&from,&fromlen);
        cout << "Client connected.\n";
        //create our recv_cmds thread and pass client socket as a parameter
        CreateThread(NULL, 0,handleMail,(LPVOID)client, 0, &thread);
    }

    WSACleanup(); //windows cleanup

    return 0; //ends program
}
