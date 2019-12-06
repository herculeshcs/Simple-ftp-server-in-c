#include <dirent.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>  	/* Para errno, perror */
#include <sys/time.h>   	/* Para setitimer */
#include <signal.h>   		/* Para signal */

/* threads */
#include<pthread.h>
/* Semáforos */
#include <semaphore.h>

#define BUFSIZE 255
#define PORTA 9001
#define SA struct sockaddr
#define MAXPENDING 5
#define MAXUSERS 100
#define MAXDADOS 1501

/*Verção 1.01 biblioteca de  gerenciamento de arquivos
* tem que usar newDirent antes de usar qualquer função
*/


/* Cabeçalho/Mensagem */
typedef struct{
  char flag;            	/* Diz se o pacote é de dados ou de confirmação */
  int  type;            	/* Tipo de menssagem se é para listar os arquivos ou se é para retorna o  ip */
  int  portFile;        	/* Porta onde o arquivo transferido será recebido */
  unsigned short  tamdados;
  char dados[MAXDADOS];
  char remetente[20];
  
}msg_t;


/*flag:
* 0 confirmação,
* 1 dados
*/

/* Contém o IP do usuário */
typedef struct{
  char id [20];
}Users;

/* Argumentos que a thread envia para a função sendFile */
typedef struct
{
  struct sockaddr_in client;
  struct sockaddr_in servaddr;
  msg_t  msg;
  int    portFile; 		/* Porta onde o arquivo transferido será recebido */
  char*  file;
}ArgumentOfThreadSend;

/* Argumentos que a thread envia para a função recvFile */
typedef struct
{
  int    sizeOfFile;       /* Tamanho do arquivo que vai ser recebido */
  char*  ip;		   /* IP de quem está tranferindo o arquivo */
  char*  file; 		   /* Ponteiro para o início do arquivo */
  int    portFile;
}ArgumentOfThreadRecv;


/* Armazena o arquivo que foi encontrado */
typedef struct
{
  int    size;        /* Tamanho do arquivo encontrado */
  char*  ip;          /* IP do cliente que possui o arquivo */
  int    portFile;    /* Porta onde o arquivo transferido será recebido */
}Achados;

/* Estrutura que contém dados dos arquivos */
typedef struct
{
  int size;      /* Tamanho do arquivo */
  char* name;   /* Nome do arquivo */
  FILE* f;      /* Ponteiro para o início do arquivo  */
}arquivo;

/* Lista dos arquivos */
typedef struct
{
  char* name;     	 /* Nome do arquivo */
  char* ip;      	/* IP do usuário que possui o arquivo */
}UserListOfFiles;

/* Armazena uma lista de arquivos */
typedef struct
{
  char ip [20];        /* IP do usuário que contém esses arquivos */
  int numberOfFiles;   /* Número de arquivos que esse usuário possui */
  arquivo* files;     /* Files aponta para a estutura que contém os dados dos arquivos */
}ListOfFiles;

/* Lista de arquivos do usuário */
typedef struct
{
  int size;     	/* Tamanho de ListOfFilesUsers */
  ListOfFiles* list; /* Vetor do tipo struct ListOfFiles que contém todos os arquivos de cada usuário conectado */
}ListOfFilesUsers;

sem_t semaforo; 		/* Controle das trheads */
sem_t semaforoFiles;		/* Controle dos arquivos */
sem_t semaforoFind;
sem_t sem_ip;

/* Declaração de variáveis globais */
int quantidade;             /* Quantidade de arquivos no diretório tem que usar newDirent antes de usar qualquer função */
int correntUsers;  			/* Quantidade de usuários conectados */
int sizeListUsersFiles; 	/* Quantos arquivos cada usuário possui */
int correntfindArquivos; 	/* Números de arquivos encontrados */
int modo; 				    /* 1 estou receibendo dados , em 1 ou zero ele esta pronto pra intrepetar menssagens de controle */
int numberOfUploads;
int sock,config1,config2;
int setar = 1,nc,FLAG;
int fASize, rLen;
int portaUpload = 55542;
FILE *arq;
time_t	ticks;
char *broadcastIP = "192.168.126.255";     /* endereço de broadcast */
char myIp[20];

/* Variáveis do tipo struct */
Users usuarios[MAXUSERS];       /* Users é um vetor de struct que contém o IP do usuário */
UserListOfFiles filesUsers[1000];
Achados findArquivos[MAXUSERS];
ListOfFiles * files; 		/* Lista de arquivos */
struct sockaddr_in sock_in,myAddr;

pthread_t thread[2];
pthread_t* upload[100];

/* Declaração das funções */
/* Lê comandos do usuário e executa as solicitações*/
void *interface(void *);

/* Prepara o cabeçalho para a mensagem ser enviada */
msg_t prepara(char *, int );

/* Envia uma mensagem via Broadcast */
void sendBroadcast(msg_t);

/* Recebe mensagens Broadcast */
void *receives(void *);

/* Envia o arquivo requisitado */
void *sendFile(void *);

/* Recebe arquivo requisitado */
void *recvFile(void *);

/* Transforma inteiro para string */
void intToString(int value, char *string);


void addUserListFile(UserListOfFiles * listUsers,char * name ,char * id,int portFile);

/* Exibe mensagem de erro */
void error(char *msg);

void recvfrom_alarm(int signo);

void alarmListFiles(int signo);

void alarmFind (int);

/* Imprime todos os usuários conectados */
void  printUsers();

/* Imprime os arquivos de todos os usuários conectados */
void printUsersFiles(UserListOfFiles * list, int tam);

/* Imprime usuários que contém o arquivo requisitado */
void printUsersFind(Achados * filesUsers,int sizeListUsersFiles);


/***************************
* Função newListOfFilesUsers
****************************/
/* Percorre o diretorio dirent abre os arquivos no modo de type e retorna um ponteiro para uma estrutura com os arquivos */
ListOfFiles * listFileInDirent(char * dirent, char * type)
{
  DIR *dir = 0;
  ListOfFiles * files= malloc(sizeof(ListOfFiles));
  files->numberOfFiles=0;
  struct dirent *entrada = 0;
  unsigned char isFile = 0x8;
  dir = opendir (dirent);
  files->files=(arquivo*) malloc (sizeof(arquivo)*quantidade);

  if (dir == 0)
    printf("Nao foi possivel abrir diretorio %s .\n",dirent);

  /* Iterar sobre o diretorio */

  int i=0;
  while (entrada = readdir (dir))
  {
    struct stat buf;
    char aux[256];
    strcpy(aux,dirent);
    #ifdef WIN32
    strcat(aux, "\\");
    #else
    strcat(aux, "/");
    #endif
    strcat(aux, entrada->d_name);
    stat(aux, &buf);



    if (entrada->d_type== 8)
    {
      if(entrada->d_name[0]!='.')
      {

	files->files[i].f= fopen(entrada->d_name,type);
	if(files->files[i].f!=NULL)
	{

	  files->files[i].name= (char*) malloc((strlen(entrada->d_name)+1)*sizeof(char));
	  strcpy(files->files[i].name,entrada->d_name);
	  //printf(" %s %d ",entrada->d_name,entrada->d_type);
	  //printf(" %d , %s  tamanho -> %d\n", fgetc(files->files[i].f),files->files[i].name,(int)buf.st_size);
	  files->files[i].size=(int)buf.st_size;
	  files->numberOfFiles++;
	  i++;
	}
      }
    }
  }
  closedir (dir);
  return files;
}

/***************************
* Função newListOfFilesUsers
***************************/
ListOfFilesUsers * newListOfFilesUsers(int tam)
{
  ListOfFilesUsers * list = (ListOfFilesUsers*) malloc(tam*sizeof(ListOfFilesUsers));
  list->size=0;
  return list;
}


/**************************
* Função wFile *
***************************/
void wFile(char * file, int tam , char * name)
{
  FILE * f =fopen(name,"wb");
  fwrite(file,sizeof(char),tam, f);
  fclose(f);
}

/**************************
* Função newDirent *
***************************/
/* Inicia quantidade que corresponde a quantidade de arquivos no diretorio. */
void newDirent(int quant)
{
  quantidade=quant;

}


/**************************
* Função findFile *
***************************/
/* Recebe uma lista de arquivos de um determinado usuário e um nome de um arquivo, procura o mesmo, se existir retorna o arquivo se não retorna NULL */
arquivo * findFile(ListOfFiles * list, char * name)
{
  int i;
  for(i=0; i<list->numberOfFiles; i++)
  {
    if(strcmp(list->files[i].name,name)==0)
    {
      return &list->files[i];
    }
  }
  return NULL;
}



