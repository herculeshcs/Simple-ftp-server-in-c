/* SERVIDOR UDP */

/* Arquivo que contém as declarações necessárias e funções de arquivos implementadas */
#include "Files.h"
int foi;
int main(int argc, char *argv[])
{
  /* Abre arquivo de log */
  strcpy(myIp,"0000000");
  arq = fopen("log.txt","a+");
  foi=1;
  if (argc != 2)
    error("Use: UDPecho <porta>");
  modo = 0;
  numberOfUploads=0;
  correntUsers=0;

  /* Prepara a estrutura de endereços */
  sock_in.sin_family = AF_INET;
  /* sock_in.sin_addr.s_addr = valor em binário da string 127.0.0.255 que corresponde ao endereço de broadcast */
  sock_in.sin_addr.s_addr = inet_addr (broadcastIP);
  sock_in.sin_port = htons(atoi(argv[1]));

  /* Prepara a estrutura de endereços */
  bzero((char *)&myAddr,sizeof(myAddr)); /* Inicializa a struct myAddr com 0 */
  myAddr.sin_family=AF_INET;
  myAddr.sin_addr.s_addr=htonl(INADDR_ANY);
  myAddr.sin_port=htons(atoi(argv[1]));

  printf("Servidor UDP\n\nComandos disponíveis:\n\nusers: listar usuários\nfiles: listar arquivos\nsearchFile: procura arquivo\ngetFile: transfere arquivo\nexit: termina programa\n\n");

  /* Cria um socket UDP */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    printf("erro na criacao do socket\n");
    exit(1);
  }

  /* Configura o socket para utilizar broadcast */
  config1 = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &setar, sizeof(setar));
  /* Permite que vários processos (e seus respectivos sockets) se associem à mesma porta de uma máquina */
  config2 = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &setar,sizeof(setar)); /* O argumento SO_REUSEADDR permiti reutilizar o endereço local*/

  /* Associa o socket ao endereço do cliente */
  if(bind(sock, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0)
  {
    puts("Erro no bind");
    close(sock);
    exit(1);
  }

  /* Lista arquivos: */
  newDirent(1024);
  /* Atribui à file os arquivos */
  files = listFileInDirent(".","rb");

  /* Inicialização dos semáforos */
  sem_init(&semaforo,0,0);
  sem_init(&semaforoFind,0,0);
  sem_init(&semaforoFiles,0,0);
  sem_init(&sem_ip,0,0);

  /* Cria thread para atender ao cliente e executar as solicitações */
  nc=pthread_create(&thread[0],NULL,interface,NULL);
  /* Cria thread para receivers arquivo*/
  nc=pthread_create(&thread[0],NULL,receives,NULL);
  pthread_exit(NULL);
}

/***************************
* Função interface
****************************/
void *interface(void *lixo){
  char Buffer[BUFSIZE];  	/* Buffer de recepção */
  msg_t msg;			/* msg corresponde a struct que define cabeçalho */
  /* descubro  meu ip */
  modo=8;
  msg= prepara("",7);
  sendBroadcast(msg);
  sem_wait(&sem_ip);
  modo=0;
  for(;;){
    scanf(" %[^\n]",Buffer);    /* Lê comando que usuário deseja executar*/
    /* Solicitação da lista de usuários na RCA(rede de compartilhamento de arquivo) */
    if(strcmp(Buffer,"users") == 0){
      correntUsers=0;
      modo = 1; 		/* receberei dados por 5 segundos depois ignorarei todos os pacotes de dados */
      FLAG = 1;
      /* Constrói cabeçalho/mensagem*/
      msg = prepara(Buffer,0);
      /* Envia mensagem via broadcast */
      sendBroadcast(msg);
      /* Pega todos os usuários que responderem a mensagem entre 2 segundos, o semaforo controla as threads */
      sem_wait(&semaforo);
      /* Imprime todos os usuários que responderam a mensagem em 2 segundos */
      printUsers();
    }
    /* Solicitação da lista de arquivos na RCA */
    else if(strcmp(Buffer,"files") == 0){
      sizeListUsersFiles=0;
      /* Constrói cabeçalho/mensagem com type == 2 */
      msg = prepara(Buffer,2);
      modo = 2;
      /* Envia mensagem via broadcast */
      sendBroadcast(msg);
      sem_wait(&semaforoFiles);
      /* Imprime todos os arquivos no diretório corrente de todos os usuários conectados na rede */
      printUsersFiles(filesUsers,sizeListUsersFiles);
    }
    /* Solicitação para buscar arquivo */
    else if(!strcmp(Buffer,"searchFile"))  /* modo 4 e 5, sendo: 4 perguntando se o usuário que escuta a mensagem tem o arquivo, 5 usuário envia Broadcast se tiver */
    {
      printf("Informe o arquivo que deseja buscar na rede e aperte enter.\n");
      char temp[120];
      correntfindArquivos=0;
      /* Realiza leitura do arquivo a ser buscado */
      scanf(" %[^\n]",temp);
      /* Constrói cabeçalho mandando, setando o type = 4 */
       files = listFileInDirent(".","rb");
       if(findFile(files,temp)==NULL)
       {
	msg = prepara(temp,4);
	/* Pergunta aos usuários quem tem o arquivo solicitado */
	modo = 4;
	/* Envia mensagem via broadcast */
	sendBroadcast(msg);
	sem_wait(&semaforoFind);
	/* Imprime todos os usuários que contém o arquivo solicitado e o tamanho do mesmo */
	printUsersFind(findArquivos,correntfindArquivos);
       }
       else
       {
	 printf("O Arquivos esta no seu diretorio\n");
       }
    }
    /* Solicita a tranferência(baixar) um determinado arquivo */
    else if(strcmp(Buffer,"getFile") == 0){
      printf("Informe o arquivo que deseja baixar e tecle enter.\n");
      char temp[120];
      correntfindArquivos=0;
      /* Realiza leitura do arquivo a ser transferido */
      scanf(" %[^\n]",temp);
       /* Constrói cabeçalho, setando o type = 4 */
      msg = prepara(temp,4);
      modo = 4;
      sendBroadcast(msg);
      sem_wait(&semaforoFind);
      /* Imprime todos os usuários que contém o arquivo solicitado e o tamanho do mesmo */
      printUsersFind(findArquivos,correntfindArquivos);
      if(correntfindArquivos != 0){
        printf("Escolha uma das opções acima para transferir.\n");
        /* Modo está em 6 para habilitar o usuário a receber o arquivo solicitado */
        modo=6;
        /* op é a escolha de qual IP que contém o arquivo o usuário que requisitou escolhe baixar */
        int op;
        /* Realiza leitura para saber de qual usuário deve efetuar a tranferência */
        scanf(" %d",&op);
        /* type 6 requisitar arquivo */
        msg = prepara(temp,6);
        /* Porta em que o arquivo deve ser recebido */
        msg.portFile = findArquivos[op-1].portFile;
        sendBroadcast(msg);
        ArgumentOfThreadRecv * aux = (ArgumentOfThreadRecv*) malloc(sizeof(ArgumentOfThreadRecv));
        /* temp contém o nome do arquivo que deverá ser tranferido */
        aux->file= (char*) malloc(sizeof(char)*strlen(temp)+1);
        aux->ip= (char*) malloc(sizeof(char)*strlen(findArquivos[op-1].ip)+1);
        aux->sizeOfFile=findArquivos[op-1].size;
        strcpy(aux->file,temp);
        strcpy(aux->ip,findArquivos[op-1].ip);
        aux->portFile = msg.portFile;
        sleep(2);
        nc=pthread_create(&thread[0],NULL,recvFile,(void*) aux);
      }
    }
    /* Finaliza programa */
    else if(strcmp(Buffer,"exit") == 0){
      msg = prepara(Buffer,0);
      sendBroadcast(msg);
      close(sock);
      fclose(arq);
      exit(0);
    }
    /* Opção escolhida por usuário não existe */
    else
      printf("Opção não disponível\n");
  }
}

/***************************
* Função prepara
****************************/
msg_t prepara(char *Buffer, int type){
  msg_t msg;
  bzero(&msg,sizeof(msg));

  strcpy (msg.remetente,myIp);
  /* Lê os dados do disco */
  msg.tamdados = strlen(Buffer);
  
  strcpy(msg.dados,Buffer);
  /* Coloca a flag que precisar */
  msg.flag = FLAG;
  /* Converte para representação da rede */
  msg.tamdados = htons(msg.tamdados);
  msg.type=type;
  return msg;
}


/***************************
* Função sendBroadcast
****************************/
void sendBroadcast(msg_t msg){
  int tam_msg;
  /* Calcula o tamanho total da mensagem a ser realmente enviada via Broadcast */
  tam_msg = sizeof(msg_t)-MAXDADOS+msg.tamdados;
  if(sendto(sock, &msg, tam_msg, 0,(struct sockaddr *) &sock_in, sizeof(sock_in)) != tam_msg)
  {
    printf("Erro na transmissão\n");
    close(sock);
    exit(1);
  }
}

/***************************
* Função receives
****************************/
void *receives(void *lixo)
{
  msg_t msg;
  /* Instalação de alarmes */
  signal(SIGALRM, recvfrom_alarm);
  for(;;){
    /* Aguarda msg */
    fASize = sizeof(myAddr);

    if ((rLen = recvfrom(sock, &msg, BUFSIZE, 0,(struct sockaddr *) &myAddr, &fASize)) <= 0)
    {
      printf("Erro de recepcao\n");
      exit(1);
    }

    
    if(msg.type==7)
    {
      if(modo==8)
      {	
	msg_t  m = prepara(inet_ntoa(myAddr.sin_addr),8);
	sendBroadcast(m);
      }
    }
    if(msg.type==8)
    {
	if(modo==8)
	{
	    if(foi==1)
	    {
	      strcpy(myIp,msg.dados);
	      foi=0;
	      sem_post(&sem_ip);
	    }
	}
    }
    if(msg.type==6) /* msg solicitando um arquivo */
    {
      if(modo!=6) /* Se modo não está habilitando usuário a receber arquivo */
      {
        /* Escreve no arquivo de log*/
        ticks = time(NULL);
        fprintf(arq,"%s solicitou envio do arquivo %s - %.24s\n",inet_ntoa(myAddr.sin_addr),msg.dados,ctime(&ticks));
        /* Se possui o arquivo solicitado */
	  if(findFile(files,msg.dados)!=NULL)
	  {
	      upload[numberOfUploads]= (pthread_t*) malloc( sizeof(pthread_t)*1);
	      ArgumentOfThreadSend * aux = (ArgumentOfThreadSend*) malloc( sizeof(ArgumentOfThreadSend));
	      aux->file= (char*) malloc(strlen(msg.dados)*sizeof(char)+1);
	      aux->msg=msg;
	      aux->servaddr=myAddr;
	      strcpy(aux->file,msg.dados);
	      aux->portFile = msg.portFile;
	      /* Envia arquivo */
	      pthread_create(upload[numberOfUploads],NULL,sendFile,(void*)aux);
	      numberOfUploads++;
	  }
      }
    }
    /* Listar os arquivos */
    if(msg.type==3)
    {
      if(modo==2)
      {
	alarm(2);
	if(msg.type==3)
	{
	      if(strcmp(myIp,inet_ntoa(myAddr.sin_addr))&& modo!=6)
	  {
	  addUserListFile(filesUsers,msg.dados,inet_ntoa(myAddr.sin_addr),msg.portFile);
	  }
	}
      }
    }
    msg.tamdados = ntohs(msg.tamdados);
    /* Receber lista de usuários */
    if(msg.type==1)
    {
      if(modo==1) /* Recebe usuários */
      {
	alarm(2);
	    if(strcmp(myIp,inet_ntoa(myAddr.sin_addr))&&modo!=6)
	{
	strcpy(usuarios[correntUsers].id,inet_ntoa(myAddr.sin_addr));
	correntUsers++;
	}
      }
    }
    if(msg.type==4) /* Mensagem solicitando busca de arquivo */
    {
      /* Escreve no arquivo de log*/
      ticks = time(NULL);
      fprintf(arq,"%s buscou o arquivo %s na rede - %.24s\n",inet_ntoa(myAddr.sin_addr),msg.dados,ctime(&ticks));

      arquivo * arq = findFile(files,msg.dados);
      if(arq!=NULL)
      {
        char temp[20];
        intToString(arq->size, temp);
        msg_t manda = prepara(temp,5);
        manda.portFile = portaUpload++;
        sendBroadcast(manda);
      }
    }
    /* Menssagem retornando quem tem o arquivo */
    if(modo==4)
    {
      alarm(2);
      if(msg.type==5) /* Receber os arquivos encontrados */
      {
	if(strcmp(myIp,inet_ntoa(myAddr.sin_addr))&& modo!=6)
	{
	  findArquivos[correntfindArquivos].ip=(char*) malloc(sizeof (char) * strlen(inet_ntoa(myAddr.sin_addr)));
	  strcpy(findArquivos[correntfindArquivos].ip,(inet_ntoa(myAddr.sin_addr)));
	  findArquivos[correntfindArquivos].portFile = msg.portFile;
	  findArquivos[correntfindArquivos].size=atoi(msg.dados);
	  correntfindArquivos++;
	}
      }
    }
    /* Envia IP via Broadcast */
    if(strcmp(msg.dados,"users") == 0){
      /* Escreve no arquivo de log*/
      ticks = time(NULL);
      fprintf(arq,"%s solicitou usuários na rede - %.24s\n",inet_ntoa(myAddr.sin_addr),ctime(&ticks));

      if(modo==1)
        alarm(2);

      FLAG = 1;
      //obter ip da máquina,
      msg = prepara("lixo",1);
      sendBroadcast(msg);
    }
    /* sendBroadcast lista de arquivos */
    else if(strcmp(msg.dados,"files") == 0){
      if(msg.type==2)
      {
	int i;
	  files = listFileInDirent(".","rb");
	for(i=0;i<files->numberOfFiles;i++)
	{
	  msg = prepara(files->files[i].name,3);
	  sendBroadcast(msg);
	}
      }
    }
  }/* Fim do for */
 
  return NULL;
}


/***************************
* Função sendFile
****************************/
void *sendFile(void *lixo)
{
  int p = 0, c;
  char Buffer[MAXDADOS];
  int  tamdados;
  /* Reseta modo */
  modo = 0;
  ArgumentOfThreadSend * aux= ( ArgumentOfThreadSend *) lixo;
  int listenfd, clientlen, n;
  int  connfd;
  struct sockaddr_in client;
  struct sockaddr_in servaddr;
  if((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    error("Falha ao criar o socket");
  FILE * arquivo = fopen(aux->file,"rb");
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  servaddr.sin_port = htons(aux->portFile);

  if(bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
    error("Falha ao observar o socket do servidor");

  if(listen(listenfd, MAXPENDING) < 0)
    error("Falha ao tentar escutar o socket do servidor");
  while(p==0)
  {
    if((connfd = accept(listenfd, (SA *) &client, &clientlen)) < 0)
      error("Falhou ao aceitar a conexao do cliente");
    else
      p = 1;
  }
 tamdados = fread(Buffer,1,MAXDADOS-1,arquivo);
  while(tamdados>0){
    c = send(connfd, Buffer, tamdados, 0);
    if ( c != tamdados)
      printf("Falha ao enviar via Broadcast todos os bytes, foi enviado apenas %d bytes \n",c);
    tamdados = fread(Buffer,1,MAXDADOS-1,arquivo);
  }

  close(listenfd);
  numberOfUploads--;
  pthread_exit(NULL);
}


/***************************
* Função recvFile
****************************/
void *recvFile(void *lixo)
{
  /* Reseta modo */
  modo = 0;
  int p = 0, tam, c;
  ArgumentOfThreadRecv * aux = (ArgumentOfThreadRecv * ) lixo;
  int sockfd,tamdados,envios;
  char Buffer[MAXDADOS];
  FILE *arquivo;
  struct sockaddr_in servaddr;
  char * stream=(char*) malloc(aux->sizeOfFile*sizeof(char));

  if((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    error("Falha ao criar o socket");

  /* Inicializando a estrutura */
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr(aux->ip);
  servaddr.sin_port = htons(aux->portFile);

  /* Estabelecendo conexao */
  while(p==0)
  {
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
      error("Falha ao conectar ao servidor");
    else
      p = 1;
  }
  c = 0;
  tam = recv(sockfd,Buffer,MAXDADOS-1,0);
  memcpy(stream,Buffer,tam);
  while(tam < aux->sizeOfFile)
  {
    c = recv(sockfd,Buffer,MAXDADOS-1,0);
    memcpy(stream+tam,Buffer,c);
    tam=tam+c;
  }
  /* Escreve stream em um arquivo binaraio de nome aux->file de tam bytes */
  wFile(stream,tam,aux->file);
  printf("Transferência concluída.\n");
}


/***************************
* Função addUserListFile
****************************/
void addUserListFile(UserListOfFiles * listUsers,char * name ,char * id, int portFile)
{
  listUsers[sizeListUsersFiles].name = (char *) malloc(sizeof(char)*strlen(name)+1);
  listUsers[sizeListUsersFiles].ip = ( char *) malloc(sizeof (char)*strlen(id)+1);
  strcpy(listUsers[sizeListUsersFiles].name,name);
  strcpy(listUsers[sizeListUsersFiles].ip,id);
  sizeListUsersFiles++;
}


/***************************
* Função intToString
****************************/
void intToString(int value, char *string)
{
  int r, i = 0, j, max;
  char aux;
  while(value>0)
  {
    r= value%10;
    string[i]=r+48;
    i++;
    value=value/10;
  }
  string[i]='\0';
  max = i;
  for(i=max-1,j=0; i>j; i--,j++)
  {
    aux= string[i];
    string[i]=string[j];
    string[j]=aux;
  }
}


/***************************
* Função printUsers
****************************/
void printUsers()
{
  int i;
  for(i=0; i < correntUsers; i++)
  {
    printf("%s\n",usuarios[i].id);
  }
}


/***************************
* Função printUsersFind
****************************/
void printUsersFind(Achados * filesUsers,int sizeListUsersFiles)
{
  int i ;
  printf("arquivo disponível em %d servidor(es)\n",sizeListUsersFiles);
  for( i=0;i<sizeListUsersFiles;i++)
  {
    printf("%d %s %d bytes\n",i+1,filesUsers[i].ip,filesUsers[i].size);
  }
}


/***************************
* Função printUsersFiles
****************************/
void printUsersFiles(UserListOfFiles * list, int tam)
{
  int i;
  for(i=0;i<tam;i++)
  {
    printf("%s %s \n",list[i].ip,list[i].name);
  }
}

/***************************
* Função alarmListFiles
****************************/
void alarmListFiles(int signo)
{
  modo = 0;
  sem_post(&semaforoFiles);
}

/***************************
* Função alarmFind
****************************/
void alarmFind (int bla)
{
  modo = 0;
  sem_post(&semaforoFind);
}

/***************************
* Função recvfrom_alarm
****************************/
void recvfrom_alarm(int signo)
{
  /* Deu 2 segundos alarme toca */
  if(modo==2)
  {
    alarmListFiles(1);
    return;
  }
  if(modo==4)
  {
    /* Acorda Find */
    alarmFind(1);
    return;
  }
  FLAG = 0;
  modo = 0;
  sem_post(&semaforo);
}


/***************************
* Função error
****************************/
void error(char *msg)
{
  printf("%s\n", msg);
  exit(0);
  return;
}

