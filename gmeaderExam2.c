/*
    Greg Meaders
    Exam 2
    gmeadeExam2.c
    11/12/15
*/
#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <signal.h>
#include <netdb.h>

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define RCVBUFSIZE 250
char* Server;
char* FileName;
int FilesTransfered;

void DieWithError(char *errorMessage){
    perror(errorMessage);
    exit(0);
}
void  ExitHandler(int sig)
{
     signal(sig, SIG_IGN);
     if(FilesTransfered == 1) printf("\n%d file was transfered\n", FilesTransfered);
     else
        printf("\n%d files were transfered \n", FilesTransfered);
     exit(0);    
}
void HandleTCPClient(int clntSocket, char *FileName)
{
    char Buffer[RCVBUFSIZE];        /* Buffer for  string */
    int recvMsgSize;                    /* Size of received message */
    FILE *fp;
    fp = fopen(FileName, "wb");

    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, Buffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0)      /* zero indicates end of transmission */
    {
        Buffer[recvMsgSize] = '\0';
        fputs(Buffer, fp);
        /* See if there is more data to receive */
        if ((recvMsgSize = recv(clntSocket, Buffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");
        
    }
    fclose(fp);
    close(clntSocket);    /* Close client socket */
    FilesTransfered++;
}

void BeAServer(unsigned short ServPort, char *FileName)
{
    signal(SIGINT, ExitHandler);
    FilesTransfered = 0;
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in ServAddr; /* Local address */
    struct sockaddr_in ClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */

    /* Create socket for incoming connections */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
      
    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));   /* Zero out structure */
    ServAddr.sin_family = AF_INET;                /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(ServPort);      /* Local port */

    /* Bind to the local address */
    if (bind(servSock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
        DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen(servSock, MAXPENDING) < 0)
        DieWithError("listen() failed");

    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(ClntAddr);

        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, 
                               &clntLen)) < 0)
            DieWithError("accept() failed");

        /* clntSock is connected to a client! */
        HandleTCPClient(clntSock, FileName);
    }
    /* NOT REACHED */

}
void BeAClient(char* Server, unsigned short ServPort, char* FileName)
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in ServAddr; /*  server address */
    char Buffer[RCVBUFSIZE];     /* Buffer for  string */
    //unsigned int StringLen;      /* Length of string to  */
    //int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv() 
                                        //and total bytes read */
    FILE *fp;
    fp = fopen(FileName, "rb");
    if(fp == NULL){DieWithError("file not found");}

    struct hostent *host;


    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
    ServAddr.sin_family      = AF_INET;             /* Internet address family */
    ServAddr.sin_addr.s_addr = inet_addr(Server);   /* Server IP address */
    ServAddr.sin_port        = htons(ServPort); /* Server port */


    if(ServAddr.sin_addr.s_addr == -1)
    {
        host = gethostbyname(Server);
            ServAddr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
    }

    /* Establish the connection to the  server */
    if (connect(sock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
        DieWithError("connect() failed");


    while(fgets(Buffer, RCVBUFSIZE, fp) != NULL)
    {
        if (send(sock, Buffer, strlen(Buffer), 0) != strlen(Buffer))
            DieWithError("send() sent a different number of bytes than expected");
    }
    printf("File successfully sent! \n");
    close(sock);
    exit(0);
}

int main(int argc, char *argv[])
{
    int check = -1;
    unsigned short ServPort = 0;
    int checkFile = 0;
    int checkPort = 0;
    int checkCheck = 0;
    
    if (argc < 7 || argc > 9)     /* Test for correct number of arguments */
    {
        DieWithError("Incorrect number of arguments");
    }

    int i = 0;
    for(i = 0; i < argc; i++)
    {
        if((strcmp(argv[i], "-mode") == 0) && argc >= i+1)
        {
            check = atoi(argv[i+1]);
            checkCheck = 1;
        }
        if(((strcmp(argv[i], "-serverport") == 0) || (strcmp(argv[i], "-port") == 0)) && argc >= i+1)
        {
            ServPort = atoi(argv[i+1]);
            checkPort = 1;
        }
        if(((strcmp(argv[i], "-servername") == 0) || (strcmp(argv[i], "-server") == 0)) && argc >= i+1)
        {
           Server = argv[i+1];
        }
        if(((strcmp(argv[i], "-file") == 0) || (strcmp(argv[i], "-FileName") == 0)) && argc >= i+1)
        {
           FileName = argv[i+1];
           checkFile = 1;
        }
    }

    if(checkCheck > 1 || checkCheck < 0) DieWithError("Invalid Mode");
    if(checkFile == 0) DieWithError("FileName not given");
    if(checkPort == 0) DieWithError("Port not given or invalid");

    if(check == 0)
    {
        BeAClient(Server, ServPort, FileName);
    }
    if(check == 1)
    {
        BeAServer(ServPort, FileName);
    }
    else DieWithError("Invalid mode flag");
    return 0;  
}
