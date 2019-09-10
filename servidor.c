#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#define LISTENQ 10
#define MAXDATASIZE 100

int main (int argc, char **argv) {
   int    listenfd, connfd;
   struct sockaddr_in servaddr;
   char   buf[MAXDATASIZE];
   time_t ticks;

   if (argc != 2) {
      printf("Por favor defina a porta que será utilizada!\n");
      exit(1);
   }

   if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
   }

   bzero(&servaddr, sizeof(servaddr)); // Apaga sizeof(servaddr) bytes de servaddr colocando '\0' em cada um deles
   servaddr.sin_family      = AF_INET;
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //Inverte de da ordem de byte do host (little endian) para a ordem de byte de rede (big endian) para um UL
   
   short port = atoi(argv[1]);

   servaddr.sin_port = htons(port); // Mesma coisa para um short

   if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) { // "Associa um nome a um socket"
      perror("bind");
      exit(1);
   }

   if (listen(listenfd, LISTENQ) == -1) { // Torna um socket em um socket passivo (socket usado para aceitar conexões por meio do accept())
      perror("listen"); // LISTENQ é o tamanho máximo da fila de conexões
      exit(1);
   }

   struct sockaddr_in clientaddr; // estrutura para armazenar os dados recebidos pelo getpeername
   socklen_t addrlen = sizeof(clientaddr);
   for ( ; ; ) {
      if ((connfd = accept(listenfd, (struct sockaddr *) NULL, NULL)) == -1 ) {  // accept extrai a primeira conexão do socket listenfd, cria novo socket conectado e retorna descritor de arquivo do novo socket
         perror("accept");
         exit(1);
      }

      if (getpeername(connfd, (struct sockaddr *) &clientaddr, &addrlen) < 0) { // retorna endereço do par conectado em connfd, em um buffer apontado por clientaddr
      	 perror("getpeername");
      	 exit(1);
      }

      printf("Nova conexão de: %s\n", inet_ntoa(clientaddr.sin_addr)); // inet_ntoa converte o endereço de clientaddr IPv4 para a string IPv4 com notaçãp padrão
      printf("Porta: %d\n\n", (int) ntohs(clientaddr.sin_port)); // converte unsigned short da porta de clientaddr da ordem de byte da Internet para ordem de byte do host

      // Escreve a data em um buffer, e envia para cliente connfd
      ticks = time(NULL);
      snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
      write(connfd, buf, strlen(buf));

      close(connfd);
   }
   return(0);
}
