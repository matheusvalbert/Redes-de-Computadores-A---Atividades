#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>

/*
 * Servidor TCP
 */

#define SHM_KEY		0x1555
#define SEM_KEY		0x1276


int g_sem;
int g_shm_id;
int s, ns;
pid_t pid, fid;


struct sembuf	up[1];
struct sembuf	down[1];


void     INThandler(int);

struct Mensagem 
{
    	char nome[20];
	char mensagem[80];
};

struct GuardarMensagens 
{
	struct Mensagem array[10];
	int count;
};

int main(int argc, char **argv) 
{
	signal(SIGINT, INThandler);
	unsigned short port;                    
	struct sockaddr_in client;
	struct sockaddr_in server; 
	int namelen;
	struct Mensagem mensagem;
	struct GuardarMensagens *mensagensSalvas, mensagensExcluidas;
	int operacao;
	char nome[20];
	int flag = 0;
	int j, i;
	int podeInserir;
	fid = 0;
	
	down[0].sem_num   =  0;
	down[0].sem_op    = -1;
	down[0].sem_flg   =  0;

	up[0].sem_num =  0;
	up[0].sem_op  =  1;
	up[0].sem_flg =  0;

	if( ( g_sem = semget( SEM_KEY, 1, 0666 | IPC_CREAT ) ) == -1 ) {
		fprintf(stderr,"chamada a semget() falhou, impossivel criar o conjunto de semaforos!");
		exit(1);
	}
	if( semop(g_sem, up, 1) == -1 ) 
	{
		fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
		exit(1);
	}

	if( (g_shm_id = shmget( SHM_KEY, sizeof(struct GuardarMensagens), IPC_CREAT | 0666)) == -1 ) {
		fprintf(stderr,"Impossivel criar o segmento de memoria compartilhada!\n");
		exit(1);
	}

	if( (mensagensSalvas = shmat(g_shm_id, NULL, 0)) < 0 ) {
		fprintf(stderr,"Impossivel associar o segmento de memoria compartilhada!\n");
		exit(1);
	}

	mensagensSalvas->count = 0;
	mensagensExcluidas.count = 0;

	/*
	 * O primeiro argumento (argv[1]) e a porta
	 * onde o servidor aguardara por conexoes
	 */
	 
	if (argc != 2) 
	{

		fprintf(stderr, "Use: %s porta\n", argv[0]);
		exit(1);
	}

	port = (unsigned short) atoi(argv[1]);

	/*
	 * Cria um socket TCP (stream) para aguardar conexoes
	 */
	 
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) 
	{

		perror("Socket()");
		exit(2);
	}

   	/*
	 * Define a qual endereco IP e porta o servidor estara ligado.
	 * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
	 * os enderecos IP
	 */
	server.sin_family = AF_INET;   
	server.sin_port   = htons(port);       
	server.sin_addr.s_addr = INADDR_ANY;

	/*
	 * Liga o servidor a porta definida anteriormente.
	 */
	 
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) 
	{
       		perror("Bind()");
       		exit(3);
	}

	/*
	 * Prepara o socket para aguardar por conexoes e
	 * cria uma fila de conexoes pendentes.
	 */
	if (listen(s, 1) != 0) 
	{
		perror("Listen()");
      	 	exit(4);
   	}

	while(1) 
	{
		/*
   		 * Aceita uma conexao e cria um novo socket atraves do qual
    		 * ocorrera a comunicacao com o cliente.
    		 */
	 
    		namelen = sizeof(client);
    		if ((ns = accept(s, (struct sockaddr *)&client, (socklen_t *)&namelen)) == -1) 
		{
			perror("Accept()");
			exit(5);
	    	}
		/*
		 * Recebe qual operação o cliente deseja realizar
		 */
		if ((pid = fork()) == 0) 
		{
			/*
			 * Processo filho 
			 */

			/* 
			 * Fecha o socket aguardando por conexoes 
			 */
			close(s);
			/* 
			 * Processo filho obtem seu proprio pid 
			 */
			fid = getpid();

			while(1)
			{
				if (recv(ns, &operacao, sizeof(int), 0) == -1) 
				{
					perror("Recv()");
					exit(6);
				}



				switch(operacao) 
				{		

					case 1:
						if(mensagensSalvas->count < 10)
						{

							if( semop(g_sem, down, 1) == -1 ) 
							{
								fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
								exit(1);
							}
							podeInserir = 0;
							if (send(ns, &podeInserir, sizeof(int), 0) < 0) 
							{
									perror("Send()");
									exit(5);
					    		}
							/* 
							 *Recebe e salva uma mensagem e o nome do cliente
							 */
							if (recv(ns, &mensagem, sizeof(struct Mensagem), 0) == -1) 
							{
								perror("Recv()");
								exit(6);
							}
							strcpy(mensagensSalvas->array[mensagensSalvas->count].nome, mensagem.nome);
							strcpy(mensagensSalvas->array[mensagensSalvas->count].mensagem, mensagem.mensagem);
							mensagensSalvas->count++;
							if( semop(g_sem, up, 1) == -1 ) 
							{
										fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
										exit(1);
							}
							printf("Nova mensagem inserida IP: %s Porta: %d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
						}
						else 
						{
							podeInserir = 1;
							if (send(ns, &podeInserir, sizeof(int), 0) < 0) 
							{
									perror("Send()");
									exit(5);
					    		}
						}
					break;

					case 2:
						/* 
						 * Envia todas as mensagens inseridas para o cliente
						 */
						if( semop(g_sem, down, 1) == -1 ) 
						{
							fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
							exit(1);
						}
				    		if (send(ns, mensagensSalvas, sizeof(struct GuardarMensagens), 0) < 0) 
						{
							perror("Send()");
							exit(5);
				    		}
						if( semop(g_sem, up, 1) == -1 ) 
						{
							fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
							exit(1);
						}
						printf("Exibicao de mensagens solicitada IP: %s Porta: %d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
					break;
				
					case 3:
						/* 
						 * Recebe o nome do usuário que possuirá suas mensagens deletas
		 				 */
						if (recv(ns, nome, sizeof(nome), 0) == -1) 
						{
							perror("Recv()");
							exit(6);
						}
						j = 0;
						if( semop(g_sem, down, 1) == -1 ) 
						{
							fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
							exit(1);
						}
						while(j <= mensagensSalvas->count) 
						{
							for(i = 0; i < mensagensSalvas->count; i++) 
							{
						
								if(strcmp(mensagensSalvas->array[i].nome, nome) == 0 && flag == 0) 
								{
									flag = 1;
									strcpy(mensagensExcluidas.array[mensagensExcluidas.count].nome,mensagensSalvas->array[i].nome);
									strcpy(mensagensExcluidas.array[mensagensExcluidas.count].mensagem,mensagensSalvas->array[i].mensagem);
									mensagensExcluidas.count++;
								}

								if(flag == 1) 
								{
									strcpy(mensagensSalvas->array[i].nome,mensagensSalvas->array[i+1].nome);
									strcpy(mensagensSalvas->array[i].mensagem,mensagensSalvas->array[i+1].mensagem);
								}
							}
						
							if(flag == 1)
								mensagensSalvas->count--;
							flag = 0;
							j++;
						}
						j = 0;
						/* 
						 * Envia as mensagens excluídas para o cliente 
						 */
				    		if (send(ns, &mensagensExcluidas, sizeof(struct GuardarMensagens), 0) < 0) 
						{
							perror("Send()");
							exit(5);
				    		}
						mensagensExcluidas.count = 0;
						if( semop(g_sem, up, 1) == -1 ) 
						{
							fprintf(stderr,"chamada semop() falhou, impossivel inicializar o semaforo!");
							exit(1);
						}
						printf("Nova exclusao de mensagens solicitada IP: %s Porta: %d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
					break;
				
					case 4:
						/* Fecha o socket conectado ao cliente */
						close(ns);
						/* Processo filho termina sua execucao */
						printf("[%d] Processo filho terminado com sucesso.\n", fid);
						exit(0);
					break;
				}
			}
		}
		else
		{  
			/*
			 * Processo pai 
			 */

			signal(SIGCHLD, SIG_IGN);
			if (pid > 0)
			{
			    printf("Processo filho criado: %d\n", pid);
			    /* Fecha o socket conectado ao cliente */
			    close(ns);
			}
			else
			{
			    perror("Fork()");
			    exit(7);            
			}
		}
	}
}





void  INThandler(int sig)
{	
	close(ns);
	if(fid == 0)
	{
		close(s);
		if( shmctl(g_shm_id,IPC_RMID,NULL) != 0 )
		{
			fprintf(stderr,"Impossivel remover o segmento de memoria compartilhada!\n");
		        exit(1);
		}

		if(semctl(g_sem, 0, IPC_RMID, 0) != 0) 
		{
		        fprintf(stderr,"Impossivel remover o conjunto de semaforos!\n");
		        exit(1);
		}
	}

	
	exit(0);
}
