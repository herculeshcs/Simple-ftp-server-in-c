
#include <stdio.h>          /* printf */
#include <stdlib.h>         /* exit */
#include <string.h>         /* bzero */
#include <sys/socket.h>     /* recv, send */
#include <arpa/inet.h>      /* struct sockaddr */
#include <unistd.h>         /* exit */
#include <pthread.h>
#include <time.h>

 #include "Files.h"
#define BUFFSIZE 32000
#define MAXPENDING 5
#define SA struct sockaddr
typedef struct
{
        int tam;
        int flag;
        char * data;
}msg;
 int connfd;
void error(char *msg);
void*  fala( void * arg)
{
  
    int *a = (int*)arg;
     int connfd= *a,i;
     char buffer[BUFFSIZE];
      char Buffer[BUFFSIZE];
     int n;	
     char aux[2000];
     printf("entro ai \n");

      if((n = recv(connfd, buffer, BUFFSIZE, 0)) < 0)
	  error("Falhou ao receber os dados iniciais do cliente");
        if(buffer[0]='P')
        {
                // receber
                int tam;
                recv(connfd,&tam, sizeof(int), 0);
                printf("o arquivo tem tamanho = %d\n",tam);
        }
        else if(buffer[0]=='L')
        {
                //enviar lista de arquivos
              
                  newDirent();
                  ListOfFiles *files=listFileInDirent(".","r");
                  int b;
                  b= files->numberOfFiles;
                  send(connfd,&b, sizeof(int), 0);
                  for(i=0;i<b;i++)
                    {
                        send(connfd,files->arquivo[i].nome, strlen(files->arquivo[i].nome)+1, 0);
                     }
                         
        }
}
  close(connfd);
}
int main(int argc, char *argv[])
{
  int listenfd, clientlen, n;
  char buffer[BUFFSIZE];
 
  
  
  struct sockaddr_in servaddr, client;

  if(argc != 2)
    error("Use: SERVERecho <porta>");
  
  if((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    error("Falha ao criar o socket");

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(atoi(argv[1]));

  if(bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
    error("Falha ao observar o socket do servidor");

  if(listen(listenfd, MAXPENDING) < 0)
    error("Falha ao tentar escutar o socket do servidor");

  for( ; ; )
    {
      n = -1;
      clientlen = sizeof(client);
      pthread_t tid;
      connfd = accept(listenfd, (SA *) &client, &clientlen);
     // if(( = accept(listenfd, (SA *) &client, &clientlen)) < 0)
      int a = connfd;// copia numero para a  só preucupação para não mandar msg para cliente errado 
      // cria trhead.
    
      puts("Cliente connectado.\n");
      printf("%u\n",ntohs(client.sin_port));// imprime a porta do cliente
      printf("%s\n",inet_ntoa(client.sin_addr));// imprime o ip do cliente
     // send(connfd,"conexão realizada",18,0);//envia menssagem para o cliente
      pthread_create(&tid, NULL,fala, (void*)&a);
      //pthread_exit(NULL); 
      
    }
 close(connfd);
  exit(0);
}

/* Imprime mensagens de erro */
void error(char *msg)
{
  printf("%s\n", msg);
  exit(0);

  return;
}
