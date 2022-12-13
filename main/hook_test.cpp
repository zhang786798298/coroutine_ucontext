#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "process.h"
#include "mutex.h"
#include <iostream>
#include "waitgroup.h"
#include "channel.h"
#include "defer.h"

bool bQuit = false;
Coroutine::WaitGroup wg;

int initserver(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cout<<"socket() failed"<<std::endl;
        return -1;
    }
    int opt = 1; 
    unsigned int len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cout<<"bind() failed."<<std::endl;
        close(sock);
        return -1;
    }

    if (listen(sock, 5) != 0)
    {
        std::cout<<"listen() failed."<<std::endl;
        close(sock);
        return -1;
    }
    
    return sock;
}

void Server()
{
    int listensock = initserver(5001);
    if (listensock < 0)
        std::cout<<"initserver() failed."<<std::endl;
    while (!bQuit)
    {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int clientsock = accept(listensock, (struct sockaddr*)&client, &len);
        if (clientsock < 0)
        {
            std::cout<<"accept() failed." << std::endl;
        }
        else
        {
            wg.Add(1);
            Coroutine::Go([=](){
                while (true) 
                {
                    char buffer[1024];
                    memset(buffer, 0, sizeof(buffer));
                    ssize_t isize = recv(clientsock, buffer, sizeof(buffer), 0);
                    if (isize <= 0)
                    {
                        printf("client(eventfd=%d) disconnected.\n", clientsock);
                        close(clientsock);
                        break;
                    }
                    printf("server recv(eventfd=%d,size=%ld):%s\n", clientsock, isize, buffer);
                    send(clientsock, buffer, strlen(buffer),0);
                }
                wg.Done();
            });
        }
    }
    wg.Done();
}

void client()
{
    int port = 5001;
    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0) 
    { 
        std::cout <<"socket() failed." << std::endl;
        return; 
    }

    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(port);
    servaddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);

    if (connect(sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
    {
        std::cout <<"connect failed "<<errno << std::endl; 
        close(sockfd); 
        return;
    }
    std::cout <<"connect ok.\n";
    Coroutine::Go([=](){
        while(true)
        {
            char buf[1024];
            memset(buf,0,sizeof(buf));
            if (recv(sockfd,buf,sizeof(buf), 0) <=0)
            {
                printf("client recv() failed.\n");  
                //close(sockfd);  
                return;
            }
            printf("client recv:%s\n",buf);
        }
    });
    for (int ii = 0; ii < 10; ii++)
    {
        std::string strBuf = "client send " + std::to_string(ii);
        if (send(sockfd, strBuf.data(), strBuf.size(), 0) <= 0)
        {
            std::cout <<"client send() failed." << std::endl;
            close(sockfd); 
            return;
        }
        Coroutine::Sleep(200);
    }
    Coroutine::Sleep(1000);
    close(sockfd); 
    wg.Done();
    bQuit = true;
}

int _cmain(int32_t argc, char** argv)
{
    wg.Add(1);
    Coroutine::Go(Server);

    Coroutine::Sleep(1000);

    wg.Add(1);
    Coroutine::Go(client);

    wg.Wait();
    
    defer([=](){
        std::cout<<"defer hook test end" << std::endl;
    });

    return 0;
}


int32_t main(int32_t argc, char** argv) 
{
    Coroutine::ProcessInstance()->SetProcesscount(0);
    Coroutine::ProcessInstance()->Start(argc, argv, _cmain);
    return 0;
}