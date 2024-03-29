#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define MAXDATASIZE 20000

void err_sys(const char* x)
{
    perror(x);
    exit(1);
}


int Socket(int family, int type, int flags) {
   int sockfd;
   if ((sockfd = socket(family, type, flags)) < 0) {
      err_sys("socket");
   } else
      return sockfd;
}

void Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
   if (bind(sockfd, addr, addrlen) == -1) { // "Associa um nome a um socket"
      err_sys("bind");
   }
}

int Accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    //accept connection from an incoming client
    int acceptedfd = accept(sockfd, addr, addrlen);
    if (acceptedfd < 0) {
      if(errno != EINTR){
        err_sys("accept error");
      }
    }
    return acceptedfd;
}

void Listen(int sockfd, int backlog) {
    if (listen(sockfd, backlog) == -1) {
        err_sys("listen");

    }
}

void Close(int s) {
    close(s);
}

void Log(char *ip, int port, int state) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[64];
    strftime(date, sizeof(date), "%c", tm);

    FILE *fptr;
    fptr = fopen("server_log.txt", "a");
    if(fptr == NULL){
      err_sys("Error!");
    }

    if (state == 0)
        fprintf(fptr,"Conexão aceita...%s: %s %d\n", date, ip, port);
    else
        fprintf(fptr,"Conexão encerrada...%s: %s %d\n", date, ip, port);
    fclose(fptr);
}

void Print(char *ip, int port, char *buf) {
    printf("Command from %s:%d ->\n %s\n", ip, port, buf);
}

typedef void Sigfunc(int);

Sigfunc * Signal(int signo, Sigfunc *func){
  struct sigaction act, oact;
  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if(signo == SIGALRM){
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
  }else{
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART;
#endif
  }
  if(sigaction (signo, &act, &oact) < 0){
    return(SIG_ERR);
  }
  return (oact.sa_handler);
}

void sig_chld(int signo){
  pid_t pid;
  int stat;
  while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
    printf("child %d terminated\n", pid);
  }
  return;
}

int main (int argc, char **argv) {
    int    listenfd, connfd, pid, read_size, port;
    socklen_t c;
    struct sockaddr_in servaddr, clientaddr;
    char   buf[MAXDATASIZE];
    FILE *fp;
    char path[MAXDATASIZE], outt[MAXDATASIZE], address[INET_ADDRSTRLEN];
    struct in_addr ipAddr;
    struct sockaddr_in *pV4Addr;

    if (argc != 3) {
      printf("%d", argc);
      printf("Define which port this server will use!\n");
      exit(1);
    }

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr)); // Apaga sizeof(servaddr) bytes de servaddr colocando '\0' em cada um deles
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //Inverte de da ordem de byte do host (little endian) para a ordem de byte de rede (big endian) para um UL

    servaddr.sin_port = htons(atoi(argv[1])); // Mesma coisa para um short

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    printf("Bind successfull\n");

    //Queue size been passed
    Listen(listenfd, atoi(argv[2]));
    Signal (SIGCHLD, sig_chld);
    printf("Awaiting for clients...\n");
    sleep(100);
    while (1) {
    	bzero(&clientaddr, sizeof(clientaddr));
      connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &c);

    	pV4Addr = (struct sockaddr_in *)&clientaddr;
    	ipAddr = pV4Addr->sin_addr;
    	inet_ntop(AF_INET, &ipAddr, address, INET_ADDRSTRLEN);
      port = ntohs(pV4Addr->sin_port);
      printf("Connection accepted -> %s:%d\n", address, port);
      Log(address, port, 0);
      if ( (pid = fork()) == 0) {
          Close(listenfd);

          while( (read_size = read(connfd , buf , MAXDATASIZE)) > 0 ) {
              buf[read_size] = 0;
              Print(address, port, buf);
              // printf("%s\n", buf);
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
              Log(address, port, 1);
              pV4Addr = (struct sockaddr_in *)&clientaddr;
              ipAddr = pV4Addr->sin_addr;
              inet_ntop(AF_INET, &ipAddr, address, INET_ADDRSTRLEN);
              printf("Client disconnected -> %s:%d\n", address, ntohs(pV4Addr->sin_port));
              fflush(stdout);
          }
          Close(connfd);
          exit(0);
      }
      Close(connfd);
    }

   return 0;
}
