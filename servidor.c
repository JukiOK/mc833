#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXDATASIZE 20000

int Socket(int family, int type, int flags) {
   int sockfd;
   if ((sockfd = socket(family, type, flags)) < 0) {
      perror("socket");
      exit(1);
   } else
      return sockfd;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
   if (bind(sockfd, addr, addrlen) == -1) { // "Associa um nome a um socket"
      perror("bind");
      exit(1);
   }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    //accept connection from an incoming client
    int acceptedfd = accept(sockfd, addr, addrlen);
    if (acceptedfd < 0) {
    	perror("accept");
    	exit(1);
    }
    return acceptedfd;
}

void Listen(int sockfd) {
    if (listen(sockfd, 10) == -1) {
        perror("listen");
        exit(1);
    }
}

void Close(int s) {
    close(s);
}

int main (int argc, char **argv) {
    int    listenfd, connfd, pid, read_size;
    socklen_t c;
    struct sockaddr_in servaddr, clientaddr;
    char   buf[MAXDATASIZE];
    FILE *fp;
    char path[MAXDATASIZE], outt[MAXDATASIZE], address[INET_ADDRSTRLEN];
    struct in_addr ipAddr;
    struct sockaddr_in *pV4Addr;

    if (argc != 2) {
      printf("%d", argc);
      printf("Define which port this server will use!\n");
      exit(1);
    }

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr)); // Apaga sizeof(servaddr) bytes de servaddr colocando '\0' em cada um deles
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //Inverte de da ordem de byte do host (little endian) para a ordem de byte de rede (big endian) para um UL

    short port = atoi(argv[1]);

    servaddr.sin_port = htons(port); // Mesma coisa para um short

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Bind successfull\n");

    //Queue size doesn't matter
    Listen(listenfd);

    printf("Awaiting for clients...\n");

    while (1) {

	bzero(&clientaddr, sizeof(clientaddr));
        connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &c);
        
	pV4Addr = (struct sockaddr_in *)&clientaddr;
	ipAddr = pV4Addr->sin_addr;
	inet_ntop(AF_INET, &ipAddr, address, INET_ADDRSTRLEN);
        printf("Connection accepted -> %s:%d\n", address, ntohs(pV4Addr->sin_port));
	if ( (pid = fork()) == 0) {
            Close(listenfd);

            while( (read_size = read(connfd , buf , MAXDATASIZE)) > 0 ) {
                buf[read_size] = 0;
                printf("%s\n", buf);

                /* Open the command for reading. */
                fp = popen(buf, "r");
                if (fp == NULL) {
                    printf("Failed to run command\n" );
                    Close(connfd);
                    exit(1);
                }

                /* Read the output a line at a time - output it. */
                while (fgets(path, sizeof(path)-1, fp) != NULL) {
                  strcat(outt, path);
                }

                /* close */
                int status = pclose(fp);
                if(status==0){
                  if(outt[0]=='\0'){
                    outt[0] = ' ';
                  }
                }else{
                  if(outt[0]=='\0'){
                    buf[strcspn(buf, "\n")] = 0;
                    strcpy(outt, strcat(buf, ": not found"));
                  }
                }
                write(connfd , outt , strlen(outt));
                memset(outt, 0, strlen(outt));

                path[0] = '\0';
            }

            if(read_size == 0) {
                printf("Client disconnected\n");
                fflush(stdout);
            }

            Close(connfd);
            exit(0);
        }
        Close(connfd);
    }

   return 0;
}
