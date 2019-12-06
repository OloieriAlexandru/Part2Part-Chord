#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

#define PORT 2909

extern int errno;

static void* serverLogic(void *);
int createServer();

static void* treat(void *);

void clientLogic();

int main (int argc, char *argv[]) {
  pthread_t serverThread;
  if (pthread_create(&serverThread, NULL, serverLogic, NULL)){
    perror("[main]Error when creating main thread for the server!");
    return 1;
  }

  clientLogic();
  return 0;
}

static void* serverLogic(void *) {
	pthread_detach(pthread_self());

  struct sockaddr_in from;
  int sd = createServer();
  bool running = true;

  if (listen(sd, 5) == -1){
    perror("[server]Error when calling listen() in server!");
  }

  pthread_t cv;

  while (running){
    int client;
    memset(&from, 0, sizeof(from));
    uint length = sizeof(from);
    if ((client = accept (sd, (struct sockaddr *) &from, &length)) < 0){
      perror ("[server]Error when calling accept()!");
      continue;
    }
    pthread_create(&cv, NULL, treat, &client);
  }

  return NULL;
}

int createServer() {
  struct sockaddr_in server;
  int sd, on = 1;

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
    perror ("[server]Error when creating the socket for the server!");
    return errno;
  }
  setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

  bzero (&server, sizeof (server));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);

  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1) {
    perror ("[server]Error when binding the socket with the server!");
    return errno;
  }
  return sd;
}

static void* treat(void* arg) {
  if (arg == NULL){
    perror("[server]Internal error! The argument for treat function should be the descriptor of the client!");
    return NULL;
  }
  int cd = *((int*)arg);

  std::cout<<cd<<'\n';

  if (-1 == close(cd)){
    perror("[server]Error when calling close() for a client socket descriptor!");
    return NULL;
  }
  return NULL;
};

void clientLogic(){
  int sd, nr=0, port;
  struct sockaddr_in server;
  std::string address;

  std::cout<<"Sintax: <adresa_server> <port>\n";
  std::cin>>address>>port;

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1){
    perror("[client]Error when calling socket()!");
    exit(errno);
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(address.c_str());
  server.sin_port = htons (port);
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1){
    perror ("[client]Error when calling connect()!\n");
    exit(errno);
  }

  std::cin>>nr;

  if (write (sd,&nr,sizeof(int)) <= 0){
    perror ("[client]Eroare when calling write()!\n");
    exit(errno);
  }

  if (read (sd, &nr,sizeof(int)) < 0){
    perror ("[client]Eroare when calling read()!\n");
    exit(errno);
  }

  printf ("[client]Mesajul primit este: %d\n", nr);

  close (sd);
}