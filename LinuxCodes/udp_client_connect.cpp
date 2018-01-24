#include<iostream>  
#include<stdlib.h>  
#include<stdio.h>  
#include<string.h>  
#include<unistd.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<arpa/inet.h>  
#include<sys/select.h>  
using namespace std;  

#define BUFSZ 1024
#define PORT  8888
#define SERVER_IP "127.0.0.1"
  
int main()  
{  
    int sockClient,nMsgLen,nReady;  
    char szRecv[1024],szSend[1024],szMsg[1024];  
    struct sockaddr_in addrServer,addrClient,addrLocal;  
    socklen_t addrLen;  
    fd_set setHold,setTest;  
  
    sockClient=socket(AF_INET,SOCK_DGRAM,0);  
    addrLen=sizeof(struct sockaddr_in);  
    bzero(&addrServer,sizeof(addrServer));  
    addrServer.sin_family=AF_INET;  
    addrServer.sin_addr.s_addr=inet_addr(SERVER_IP);
    addrServer.sin_port=htons(PORT);  
 
    addrLocal.sin_family=AF_INET;//bind to a local port  
    addrLocal.sin_addr.s_addr=htonl(INADDR_ANY);  
    addrLocal.sin_port=htons(9000);  
    if(bind(sockClient,(struct sockaddr*)&addrLocal,sizeof(addrLocal))==-1)  
    {  
        perror("error in binding");  
        exit(2);  
    }  
  
    int f = 0;  
    if(connect(sockClient,(struct sockaddr*)&addrServer,sizeof(addrServer))==-1)  
    {  
        perror("error in connecting");  
        exit(1);  
    }  
  
    f = 1;
    FD_ZERO(&setHold);  
    FD_SET(STDIN_FILENO,&setHold);  
    FD_SET(sockClient,&setHold);  
    cout<<"you can type in sentences any time"<<endl;  
    while(true)  
    {  
        setTest=setHold;  
        nReady=select(sockClient+1,&setTest,NULL,NULL,NULL);
		
        //can write
		if(FD_ISSET(0,&setTest))  
        {  
            nMsgLen=read(0,szMsg,1024);  
            write(sockClient,szMsg,nMsgLen);  
        }  
        
		// can read 
        if(FD_ISSET(sockClient,&setTest))  
        {  
            if ( 1 == f )  
            {  
                // 1:by read/write........  
                //nMsgLen=read(sockClient,szRecv,1024);  
                // 2:by recvfrom/sendto  
                nMsgLen = recvfrom(sockClient,szRecv,1024,0,NULL,NULL);  
                perror("error in connecting recvfrom");  
                // unconnect  
                addrServer.sin_family=AF_UNSPEC;  
                connect(sockClient,(struct sockaddr*)&addrServer,sizeof(addrServer));  
                f = 0;  
            } 
            else  
            {
                f = 1;
                nMsgLen = recvfrom(sockClient,szRecv,1024,0,(struct sockaddr*)&addrServer,&addrLen);  
                // connect  
                bzero(&addrServer,sizeof(addrServer));  
                addrServer.sin_family=AF_INET;  
                addrServer.sin_addr.s_addr=inet_addr(SERVER_IP);  
                addrServer.sin_port=htons(PORT);  
                connect(sockClient,(struct sockaddr*)&addrServer,sizeof(addrServer));  
            }  
            szRecv[nMsgLen]='\0';  
            cout<<"read:"<<szRecv<<endl;  
        }  
    }  
}  

/*
客户端2，主动向客户端1发送数据:
在client1调用connect向server建立连接后，不会接受client2的数据，断开连接后会接受client2的数据。
*/

