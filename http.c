
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
	printf(" %s\n",buffer);
      
      printf("n= %d\n",n);
      newDirent (1200);
      ListOfFiles * file = listFileInDirent(".","r");
    
      for(i=0;i<file->numberOfFiles;i++)
      {
	  printf(" %s\n",file->files[i].name);
      }
      printf("5 = %c\n",buffer[5]);
      
      for(i=5;buffer[i]!=' ';i++) 
      {
	  aux[i-5]=buffer[i];
      }
      aux[i-5]='\0';
      printf("eu quero = %s\n",aux);	
      arquivo * quero = findFile(file, aux);
      printf("tudo sussa\n");
      if(quero == NULL)
      {
	  printf(" deu pau\n");
	  send(connfd,"HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n erro 404",strlen("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n erro 404"),0);
      }
      
      else
      {
	  int c;
	  
	  time_t tic= time(NULL);
	  printf(" %.24s\n",ctime(&tic));
	   sprintf(aux,"\r\nHTTP/1.1 200 OK\r\nConnection: close\r\nLast-Modified: %.24s\r\n\r\n",ctime(&tic));
	  int tam = strlen(aux);
	  int tamArq= quero->size;
	  FILE * envia=  fopen(quero->name,"rb");
	  send(connfd, aux, tam, 0);
	  
	int tamdados = fread(Buffer,1,MAXDADOS-1,envia);
	while(tamdados>0){
	c = send(connfd, Buffer, tamdados, 0);
	  if ( c != tamdados)
	  printf("Falha ao enviar via Broadcast todos os bytes, foi enviado apenas %d bytes \n",c);
	  tamdados = fread(Buffer,1,MAXDADOS-1,envia);
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