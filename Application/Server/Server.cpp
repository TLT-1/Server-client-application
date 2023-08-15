#include "Server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

int Server::init(uint16_t port)
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
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        WSACleanup();
        return SHUTDOWN;
    }

    // Listen for incoming connections
    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        WSACleanup();
        return SHUTDOWN;
    }

    // Clear the sets
    FD_ZERO(&m_readSet);
    FD_SET(m_socket, &m_readSet);

    return SUCCESS;
}

int Server::run()
{
    while (true)
    {
        fd_set copySet = m_readSet;

        int activity = select(0, &copySet, nullptr, nullptr, nullptr);
        if (activity == SOCKET_ERROR)
        {
            return SHUTDOWN;
        }

        if (FD_ISSET(m_socket, &copySet))
        {
            // New incoming connection
            sockaddr_in clientAddr;
            int addrLength = sizeof(clientAddr);
            SOCKET newClient = accept(m_socket, (struct sockaddr*)&clientAddr, &addrLength);
            if (newClient == INVALID_SOCKET)
            {
                continue;
            }

            // Add the new client to the clients array
            if (m_clients.size() < MAX_CLIENTS)
            {
                m_clients.push_back(newClient);
                FD_SET(newClient, &m_readSet);
                std::cout << "New client connected." << std::endl;
            }
            else
            {
                // Too many clients, close the connection
                std::cerr << "Too many clients. Connection rejected." << std::endl;
                closesocket(newClient);
            }
        }

        // Check for data from clients
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
        {
            SOCKET clientSocket = *it;
            if (FD_ISSET(clientSocket, &copySet))
            {
                char buffer[MESSAGE_SIZE];
                int bytesRead = recv(clientSocket, buffer, MESSAGE_SIZE, 0);
                if (bytesRead == SOCKET_ERROR)
                {
                    // Error or connection closed
                    std::cerr << "Client disconnected." << std::endl;
                    closesocket(clientSocket);
                    FD_CLR(clientSocket, &m_readSet);
                    it = m_clients.erase(it);
                }
                else if (bytesRead == 0)
                {
                    // Connection closed by client
                    std::cout << "Client closed the connection." << std::endl;
                    closesocket(clientSocket);
                    FD_CLR(clientSocket, &m_readSet);
                    it = m_clients.erase(it);
                }
                else
                {
                    // Data received
                    buffer[bytesRead] = '\0';
                    std::cout << "Received: " << buffer << std::endl;
                }
            }
        }
    }

    return SHUTDOWN;

}

int Server::readMessage(char* buffer, int32_t size)
{
    fd_set readSet;
    FD_ZERO(&readSet);

    // Add client sockets to the set
    for (SOCKET clientSocket : m_clients)
    {
        FD_SET(clientSocket, &readSet);
    }

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000; // 10 milliseconds

    int activity = select(0, &readSet, nullptr, nullptr, &timeout);
    if (activity == SOCKET_ERROR)
    {
        return SHUTDOWN;
    }

    for (SOCKET clientSocket : m_clients)
    {
        if (FD_ISSET(clientSocket, &readSet))
        {
            int bytesRead = recv(clientSocket, buffer, size, 0);
            if (bytesRead == SOCKET_ERROR)
            {
                // Error or connection closed
                std::cerr << "Client disconnected." << std::endl;
                return SHUTDOWN;
            }
            else if (bytesRead == 0)
            {
                // Connection closed by client
                std::cout << "Client closed the connection." << std::endl;
                return SHUTDOWN;
            }
            else
            {
                return bytesRead;
            }
        }
    }

    return 0; // No data received
}

int Server::sendMessage(char* data, int32_t length)
{
    for (SOCKET clientSocket : m_clients)
    {
        int bytesSent = send(clientSocket, data, length, 0);
        if (bytesSent == SOCKET_ERROR)
        {
            // Error occurred, handle it (e.g., close the connection)
            std::cerr << "Failed to send message to client." << std::endl;
            return SHUTDOWN;
        }
    }

    return length; // Return the total bytes sent (useful for error checking)
}


void Server::stop()
{
    for (SOCKET clientSocket : m_clients)
    {
        closesocket(clientSocket);
    }
    closesocket(m_socket);
    WSACleanup();
}
