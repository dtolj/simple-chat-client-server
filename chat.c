/*
Dejan Tolj
License: GPLv2
Date: Jan 2005
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MSG_SIZE 80
#define MAX_CLIENTS 95
#define MYPORT 7400

void exitClient(int fd, fd_set *readfds, char fd_array[], int *num_clients){
    int i;
    
    close(fd);
    FD_CLR(fd, readfds); //clear the leaving client from the set
    for (i = 0; i < (*num_clients) - 1; i++)
        if (fd_array[i] == fd)
            break;          
    for (; i < (*num_clients) - 1; i++)
        (fd_array[i]) = (fd_array[i + 1]);
    (*num_clients)--;
}


int main(int argc, char *argv[]) {
   int i=0;
   
   int port;
   int num_clients = 0;
   int server_sockfd, client_sockfd;
   struct sockaddr_in server_address;
   int addresslen = sizeof(struct sockaddr_in);
   int fd;
   char fd_array[MAX_CLIENTS];
   fd_set readfds, testfds, clientfds;
   char msg[MSG_SIZE + 1];     
   char kb_msg[MSG_SIZE + 10]; 
   
   /*Client variables=======================*/
   int sockfd;
   int result;
   char hostname[MSG_SIZE];
   struct hostent *hostinfo;
   struct sockaddr_in address;
   char alias[MSG_SIZE];
   int clientid;
   

   /*Client==================================================*/
   if(argc==2 || argc==4){
     if(!strcmp("-p",argv[1])){
       if(argc==2){
         printf("Invalid parameters.\nUsage: chat [-p PORT] HOSTNAME\n");
         exit(0);
       }else{
         sscanf(argv[2],"%i",&port);
         strcpy(hostname,argv[3]);
       }
     }else{
       port=MYPORT;
       strcpy(hostname,argv[1]);
     }
     printf("\n*** Client program starting (enter \"quit\" to stop): \n");
     fflush(stdout);
     
     /* Create a socket for the client */
     sockfd = socket(AF_INET, SOCK_STREAM, 0);

     /* Name the socket, as agreed with the server */
     hostinfo = gethostbyname(hostname);  /* look for host's name */
     address.sin_addr = *(struct in_addr *)*hostinfo -> h_addr_list;
     address.sin_family = AF_INET;
     address.sin_port = htons(port);

     /* Connect the socket to the server's socket */
     if(connect(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0){
       perror("connecting");
       exit(1);
     }
     
     fflush(stdout);
     
     FD_ZERO(&clientfds);
     FD_SET(sockfd,&clientfds);
     FD_SET(0,&clientfds);//stdin
     
     /*  Now wait for messages from the server */
     while (1) {
       testfds=clientfds;
       select(FD_SETSIZE,&testfds,NULL,NULL,NULL);
       
       for(fd=0;fd<FD_SETSIZE;fd++){
          if(FD_ISSET(fd,&testfds)){
             if(fd==sockfd){   /*Accept data from open socket */
                //printf("client - read\n");
                
                //read data from open socket
                result = read(sockfd, msg, MSG_SIZE);
                msg[result] = '\0';  /* Terminate string with null */
                printf("%s", msg +1);
                
                if (msg[0] == 'X') {                   
                    close(sockfd);
                    exit(0);
                }                             
             }
             else if(fd == 0){ /*process keyboard activiy*/
               // printf("client - send\n");
                
                fgets(kb_msg, MSG_SIZE+1, stdin);
                //printf("%s\n",kb_msg);
                if (strcmp(kb_msg, "quit\n")==0) {
                    sprintf(msg, "XClient is shutting down.\n");
                    write(sockfd, msg, strlen(msg));
                    close(sockfd); //close the current client
                    exit(0); //end program
                }
                else {
                   /* sprintf(kb_msg,"%s",alias);
                    msg[result]='\0';
                    strcat(kb_msg,msg+1);*/

                    sprintf(msg, "M%s", kb_msg);
                    write(sockfd, msg, strlen(msg));
                }                                                 
             }          
          }
       }      
     }
   }// end client code
   

   /*Server==================================================*/
   if(argc==1 || argc == 3){
     if(argc==3){
       if(!strcmp("-p",argv[1])){
         sscanf(argv[2],"%i",&port);
       }else{
         printf("Invalid parameter.\nUsage: chat [-p PORT] HOSTNAME\n");
         exit(0);
       }
     }else port=MYPORT;
     
     printf("\n*** Server program starting (enter \"quit\" to stop): \n");
     fflush(stdout);

     /* Create and name a socket for the server */
     server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
     server_address.sin_family = AF_INET;
     server_address.sin_addr.s_addr = htonl(INADDR_ANY);
     server_address.sin_port = htons(port);
     bind(server_sockfd, (struct sockaddr *)&server_address, addresslen);

     /* Create a connection queue and initialize a file descriptor set */
     listen(server_sockfd, 1);
     FD_ZERO(&readfds);
     FD_SET(server_sockfd, &readfds);
     FD_SET(0, &readfds);  /* Add keyboard to file descriptor set */
     
     //struct clientinfo c;
     
     //printf("sizeof(readfds)/(testfds): %d/%d\n",sizeof(readfds),sizeof(testfds));
     //printf("Number of bits in readfds/testfds: %d/%d\n", sizeof(readfds)*8,sizeof(testfds)*8);

     /*  Now wait for clients and requests */
     while (1) {
        testfds = readfds;
        select(FD_SETSIZE, &testfds, NULL, NULL, NULL);
                    
        /* If there is activity, find which descriptor it's on using FD_ISSET */
        for (fd = 0; fd < FD_SETSIZE; fd++) {
           if (FD_ISSET(fd, &testfds)) {
              
              if (fd == server_sockfd) { /* Accept a new connection request */
                 client_sockfd = accept(server_sockfd, NULL, NULL);
                 /*printf("client_sockfd: %d\n",client_sockfd);*/
                
                                
                 if (num_clients < MAX_CLIENTS) {
                    FD_SET(client_sockfd, &readfds);
                    fd_array[num_clients]=client_sockfd;
                    /*Client ID*/
                    printf("Client %d joined\n",num_clients++);
                    fflush(stdout);
                    
                    sprintf(msg,"M%2d",client_sockfd);
                    /*write 2 byte clientID */
                    send(client_sockfd,msg,strlen(msg),0);
                 }
                 else {
                    sprintf(msg, "XSorry, too many clients.  Try again later.\n");
                    write(client_sockfd, msg, strlen(msg));
                    close(client_sockfd);
                 }
              }
              else if (fd == 0)  {  /* Process keyboard activity */                 
                 fgets(kb_msg, MSG_SIZE + 1, stdin);
                 //printf("%s\n",kb_msg);
                 if (strcmp(kb_msg, "quit\n")==0) {
                    sprintf(msg, "XServer is shutting down.\n");
                    for (i = 0; i < num_clients ; i++) {
                       write(fd_array[i], msg, strlen(msg));
                       close(fd_array[i]);
                    }
                    close(server_sockfd);
                    exit(0);
                 }
                 else {
                    //printf("server - send\n");
                    sprintf(msg, "M%s", kb_msg);
                    for (i = 0; i < num_clients ; i++)
                       write(fd_array[i], msg, strlen(msg));
                 }
              }
              else if(fd) {  /*Process Client specific activity*/
                 //printf("server - read\n");
                 //read data from open socket
                 result = read(fd, msg, MSG_SIZE);
                 
                 if(result==-1) perror("read()");
                 else if(result>0){
                    /*read 2 bytes client id*/
                    sprintf(kb_msg,"M%2d",fd);
                    msg[result]='\0';
                    
                    /*concatinate the client id with the client's message*/
                    strcat(kb_msg,msg+1);                                        
                    
                    /*print to other clients*/
                    for(i=0;i<num_clients;i++){
                       if (fd_array[i] != fd)  /*dont write msg to same client*/
                          write(fd_array[i],kb_msg,strlen(kb_msg));
                    }
                    /*print to server*/
                    printf("%s",kb_msg+1);
                    
                     /*Exit Client*/
                    if(msg[0] == 'X'){
                       exitClient(fd,&readfds, fd_array,&num_clients);
                    }   
                 }                                   
              }                  
              else {  /* A client is leaving */
                 exitClient(fd,&readfds, fd_array,&num_clients);
              }//if
           }//if
        }//for
     }//while
  }//end Server code

}//main


