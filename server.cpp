#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include "serverthreadpool.cpp"
#include "winsock2.h"

#define SERVER_PORT   19664
#define MAX_PENDING   5
#define MAX_LINE      256
#define MAX_USERS     3

/*
Caleb Foster
cjf6qh
CS 4850
*/

std::mutex newuserWriteLock;
std::condition_variable clientLeft;
void clientConnect(std::map<std::string, std::string>&, SOCKET, FILE*, std::map<std::string, SOCKET>&);
int CONNECTED_CLIENTS = 0;

int main() {
    printf("Caleb's chatroom server. Version Two.\n\n");

    // Initialize User List
    std::map<std::string, std::string> userList;
    FILE* userFile = fopen("users.txt", "a+");
    if (userFile == NULL) {
        printf("New user list created.\n");
        FILE* userFile = fopen("users.txt", "w+");
        fflush(userFile);
        if (userFile == NULL) return 1;
    }
    // else block will only trigger on a file opened with "a+" (file exists)
    else {
        while (!feof(userFile)) {
            char username[33];
            char password[9];
            username[0] = password[0] = 0;
            fscanf(userFile, "(%32[^,], %8[^)]\n) ", username, password);
            // set ends to 0 to appease debugger
            username[32] = password[8] = 0;
            // if no user was read, break from loop
            if (strcmp(username, "") == 0) {
                break;
            }
            userList[username] = password;
        }
    }

    // Initialize Winsock.
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 1;
    }

    // Create a socket.
    SOCKET listenSocket;
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Bind the socket.
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; //use local address
    addr.sin_port = htons(SERVER_PORT);
    if (bind(listenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("bind() failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    // Listen on the Socket.
    if (listen(listenSocket, MAX_PENDING) == SOCKET_ERROR) {
        printf("Error listening on socket.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    SOCKET s;
    serverthreadpool threads(MAX_USERS);
    std::map<std::string, SOCKET> activeUsers;
    threads.start();

    while (1) {
        printf("Waiting for a client to connect...\n");
        s = accept(listenSocket, NULL, NULL);
        if (s == SOCKET_ERROR) {

            printf("accept() error \n");
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        CONNECTED_CLIENTS += 1;
        // MIGHT NOT PASS LIST BY REF
        threads.acceptConnection([&userList, s, userFile, &activeUsers] () { clientConnect(userList, s, userFile, activeUsers); });  
    }
    closesocket(listenSocket);
    fclose(userFile);

}


// Accept connections.
void clientConnect(std::map<std::string,std::string>& userList, SOCKET s, FILE* userFile, std::map<std::string, SOCKET>& activeUsers) {
    printf("Thread active\n");
    char buf[MAX_LINE];
    buf[0] = 0;
    char command[MAX_LINE];
    // command library
    std::map<std::string, int> commands{
    {"logout", 0},
    {"login", 1},
    {"newuser", 2},
    {"send", 3},
        {"who", 4},
    };


    printf("Client Connected.\n");
    bool CONNECTED = true;
    char ACTIVE_USER[33];
    std::string returnMessage;

    while (CONNECTED) {
        // Send and receive data.

        // The commands received by the server are assumed to follow expected syntax
        int len = recv(s, buf, MAX_LINE - 1, 0);
        if (len == 0) {
            break;
        }
        buf[len] = 0;
        std::stringstream bufstream(buf);
        bufstream >> command;

        // searches command library
        if (commands.find(command) == commands.end()) {
            returnMessage.clear();
            returnMessage.append("Command ").append(command).append(" not found");
            if (returnMessage.length() > MAX_LINE - 1) {
                send(s, "No matching command found", 26, 0);
                continue;
            }
        }

        else {
            char username[33];
            char password[9];

            // begin switch
            switch (commands.find(command)->second) {
                // logout
            case 0:
                CONNECTED = false;
                returnMessage.clear();
                returnMessage.append(ACTIVE_USER).append(" left.");
                printf("%s logout.\n", ACTIVE_USER);
                activeUsers.erase(ACTIVE_USER);
                for (auto user : activeUsers) {
                    send(user.second, returnMessage.c_str(), returnMessage.length(), 0);
                }
                break;

                // login
            case 1:
                returnMessage.clear();
                bufstream >> username;
                    if (activeUsers.find(username) != activeUsers.end()) {
                        returnMessage.append("Denied. User \"").append(username).append("\" is already logged in.");
                        break;
                }
                if (userList.find(username) != userList.end()) {
                    bufstream >> password;
                    if (strcmp(password, userList.at(username).c_str()) == 0) {
                        returnMessage.append(username).append(" joins.");
                        for (auto user : activeUsers) {
                            send(user.second, returnMessage.c_str(), returnMessage.length(), 0);
                        }
                        strcpy(ACTIVE_USER, username);
                        activeUsers[ACTIVE_USER] = s;
                        send(s, "Login Success", 14, 0);
                        returnMessage.clear();
                        returnMessage.append("login confirmed");
                        printf("%s login.\n", username);
                        break;
                    }
                }
                returnMessage.append("Denied. User name or password incorrect.");
                break;

                // newuser
            case 2:
                returnMessage.clear();
                bufstream >> username;
                if (userList.find(username) != userList.end()) {
                    returnMessage.append("User account already exists.");
                    break;
                }
                else {
                    bufstream >> password;
                    if (userList.size() == 0) {
                        int chars = fprintf(userFile, "(%s, %s)", username, password);
                    }
                    else {
                        int chars = fprintf(userFile, "\n(%s, %s)", username, password);
                    }
                    fflush(userFile);
                    //printf("%d chars written\n", chars);
                    userList[username] = password;
                    returnMessage.append("New user account created. Please login.");
                    printf("New user account created.\n");
                }
                break;

                // send {user | all}
            case 3:
                returnMessage.clear();
                bufstream >> username;
                bufstream.get(buf, MAX_LINE);
                returnMessage.append(ACTIVE_USER).append(":").append(buf);
                if (returnMessage.length() > 255) {
                    returnMessage.resize(255);
                }
                if (activeUsers.find(username) == activeUsers.end()) {
                    if (strcmp(username, "all") == 0) {
                        for (const auto& user : activeUsers) {
                            send(user.second, returnMessage.c_str(), returnMessage.length(), 0);
                        }
                        printf("%s\n", returnMessage.c_str());
                        continue;
                    }
                    else {
                        returnMessage.clear();
                        returnMessage.append("No user \"").append(username).append("\" found on server.");
                        break;
                    }
                }               
                send(activeUsers.at(username), returnMessage.c_str(), returnMessage.length(), 0);
                continue;

                // who
            case 4:
                returnMessage.clear();
                for (auto user : activeUsers) {
                    returnMessage.append(user.first).append(", ");
                }
                break;


                // command unknown
            default:
                returnMessage.clear();
                returnMessage.append("Command not recognized.");
                break;
            }
            send(s, returnMessage.c_str(), returnMessage.size(), 0);
        }
    }
    printf("Client Closed.\n");
    closesocket(s);
    CONNECTED_CLIENTS -= 1;
    clientLeft.notify_one();
    //}
}

