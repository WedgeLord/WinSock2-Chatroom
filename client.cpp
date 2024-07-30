#include <stdio.h>
#include <string>
#include <sstream>
#include <map>
#include "winsock2.h"
#include <thread>
#include <mutex>

#define SERVER_PORT  19664
#define MAX_LINE      256
/*
Caleb Foster
cjf6qh
CS 4850
*/

std::mutex recvLock;
bool userCommandCheck(std::stringstream&, char*);
void receiveMsgs(bool&, bool&, char[MAX_LINE], SOCKET&);
void sendCommands(std::map<std::string, int>&, bool&, bool&, char[MAX_LINE], SOCKET&);

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("\nUseage: client serverName\n");
        return 1;
    }

    printf("Caleb's chat room client. Version Two.\n\n");

    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 1;
    }

    //translate the server name or IP address (128.90.54.1) to resolved IP address
    unsigned int ipaddr;
    // If the user input is an alpha name for the host, use gethostbyname()
    // If not, get host by addr (assume IPv4)
    if (isalpha(argv[1][0])) {   // host address is a name  
        hostent* remoteHost = gethostbyname(argv[1]);
        if (remoteHost == NULL) {
            printf("Host not found\n");
            WSACleanup();
            return 1;
        }
        ipaddr = *((unsigned long*)remoteHost->h_addr);
    }
    else //"128.90.54.1"
        ipaddr = inet_addr(argv[1]);


    // Create a socket.
    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Connect to a server.
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ipaddr;
    addr.sin_port = htons(SERVER_PORT);
    if (connect(s, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Failed to connect.\n");
        WSACleanup();
        return 1;
    }

    // key-value map used for mapping commands to switch cases
    std::map<std::string, int> commands{
        {"logout", 0},
        {"login", 1},
        {"newuser", 2},
        {"send", 3},
        {"who", 4},
    };
    bool LOGGED_IN = false;
    bool CONNECTED = true;
    char buf[MAX_LINE];
    std::thread(sendCommands, std::ref(commands), std::ref(CONNECTED), std::ref(LOGGED_IN), buf, std::ref(s)).detach();
    std::thread(receiveMsgs, std::ref(CONNECTED), std::ref(LOGGED_IN), buf, std::ref(s)).join();
}

void receiveMsgs(bool& CONNECTED, bool& LOGGED_IN, char buf[MAX_LINE], SOCKET& s) {
    int len;
    fd_set set = { 1, s };
    timeval tv = { 0, 100000 };

    while (s != INVALID_SOCKET && CONNECTED) {
        FD_SET(s, &set);
        {
            int result = select(0, &set, NULL, NULL, &tv);
            std::unique_lock<std::mutex> lock(recvLock);
            //printf("receiving %d\n", result == -1 ? WSAGetLastError() : result);
            //Sleep(1000);
            if (result > 0)
            {
                len = recv(s, buf, MAX_LINE - 1, 0);
            }
            else {
                lock.unlock();
                continue;
            }
            if (len <= 0) {
                closesocket(s);
                return;
            }
            else {
                buf[len] = 0;
                if (!LOGGED_IN) {
                    if (strcmp(buf, "Login Success") == 0) {
                        LOGGED_IN = true;
                        continue;
                    }
                }
                printf("%s\n>", buf);
            }
        }
    }
}

void sendCommands(std::map<std::string, int>& commands, bool& CONNECTED, bool& LOGGED_IN, char buf[MAX_LINE], SOCKET& s) {
    // Send and receive data.
    // reusable char array that will allow the stringstream to be broken up into words and computed
    char command[MAX_LINE];
    // in this loop, there is a switch statement that will redirect program flow based on the user's input.
    // "break" will begin waiting for the server's response, and "continue" will return to reading user input.
    while (CONNECTED) {
        printf(">");
        fgets(buf, MAX_LINE, stdin);
        {
            std::unique_lock<std::mutex> lock(recvLock);
            // stringstream allows a string to be broken up into words, then passed to a char array
            std::stringstream bufstream(buf);
            bufstream >> command;
            if (commands.find(command) == commands.end()) {
                printf("No command \"%s\" found.\n", command);
                continue;
            }
            else {
                // begin switch
                switch (commands.find(command)->second) {
                    // logout
                case 0:
                    if (LOGGED_IN) {
                        CONNECTED = false;
                        LOGGED_IN = false;
                        send(s, buf, strlen(buf), 0);
                        break;
                    }
                    else {
                        printf("Already logged out!\n");
                        continue;
                    }

                    // login
                case 1:
                    if (LOGGED_IN) {
                        printf("Denied. Already logged in.\n");
                        continue;
                    }
                    if (userCommandCheck(bufstream, command)) {
                        send(s, buf, strlen(buf), 0);
                        break;
                    }
                    else {
                        continue;
                    }
                    break;

                    // newuser
                case 2:
                    if (LOGGED_IN) {
                        printf("Denied. Please logout first.\n");
                        continue;
                    }
                    else {
                        if (userCommandCheck(bufstream, command)) {
                            send(s, buf, strlen(buf), 0);
                        }
                        else {
                            continue;
                        }
                    }
                    break;

                    // send {user}
                case 3:
                    if (LOGGED_IN) {
                        if (bufstream >> command) {
                            if (bufstream >> command) {
                                send(s, buf, strlen(buf), 0);
                                break;
                            }
                        }
                        printf("\"send\" message empty. Try again!\n");
                        continue;
                    }
                    else {
                        printf("Denied. Please login first.\n");
                        continue;
                    }
                    break;

                    // who
                case 4:
                    if (LOGGED_IN) {
                        send(s, "who", 4, 0);
                    }
                    else {
                        printf("Denied. Log in first\n");
                    }

                default:
                    continue;
                }
                // end of switch
            }
        }
    }
    //printf("%d\n", WSAGetLastError());
    closesocket(s);
}
   /*
   This function checks the syntax of the "username" and "password" fields of 
   the "login" and "newuser" commands, which have identical syntax.
   
   Returns "true" if syntax is accepted, false otherwise.
   
   Additionally, this function will print the syntax error to the client user's
   output. 
   */
   bool userCommandCheck(std::stringstream& commandStream, char * command) {
       static char word[MAX_LINE];
       commandStream >> word;
       if ((strlen(word) <= 32) && (strlen(word) >= 3)) {
           word[0] = '\0';
           commandStream >> word;
           if ((strlen(word) <= 8) && (strlen(word) >= 4)) {
               if (commandStream >> word) {
                   printf("Command \"%s\" should only have two arguments.\n",
                       !strcmp(command, "login") ? "login" : "newuser");
               }
               else
                   return true;
           }
           else {
               printf("Argument 2 (password) should be between 4 and 8 characters long.\n");
           }
       }
       else {
           printf("Argument 1 (username) should be between 3 and 32 characters long.\n");
       }
       return false;
   }