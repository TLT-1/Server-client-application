#include "Client.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")


int Client::init(const char* serverIP, uint16_t serverPort)
{
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return SHUTDOWN;
    }

    // Create a socket
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket == INVALID_SOCKET)
    {
        WSACleanup();
        return SHUTDOWN;
    }

    // Prepare the address information
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0)
    {
        closesocket(m_socket);
        WSACleanup();
        return SHUTDOWN;
    }

    // Connect to the server
    if (connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        WSACleanup();
        return SHUTDOWN;
    }

    return SUCCESS;
}

int Client::sendMessage(const char* message)
{
    int bytesSent = send(m_socket, message, strlen(message), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        std::cerr << "Failed to send message." << std::endl;
        return SHUTDOWN;
    }

    return bytesSent;
}

int Client::receiveMessage(char* buffer, int32_t size)
{
    int bytesReceived = recv(m_socket, buffer, size - 1, 0);
    if (bytesReceived == SOCKET_ERROR)
    {
        std::cerr << "Failed to receive message." << std::endl;
        return SHUTDOWN;
    }
    else if (bytesReceived == 0)
    {
        std::cout << "Server closed the connection." << std::endl;
        return SHUTDOWN;
    }

    buffer[bytesReceived] = '\0';
    return bytesReceived;
}

void Client::stop()
{
    closesocket(m_socket);
    WSACleanup();
}
