#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>

/*
 * Servidor TCP
 */
int main(int argc, char **argv)
{
    unsigned short port;       
    char sendbuf[12];              
    char recvbuf[12];              
    struct sockaddr_in client; 
    struct sockaddr_in server; 
    int s;                     /* Socket para aceitar conexoes       */
    int ns;                    /* Socket conectado ao cliente        */
    int namelen;
    pid_t pid, fid;
    
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
	  if ((ns = accept(s, (struct sockaddr *) &client, (socklen_t *) &namelen)) == -1)
	  {
		perror("Accept()");
		exit(5);
	  }
	  
	  if ((pid = fork()) == 0) {
		/*
		 * Processo filho 
		 */
	      
		/* Fecha o socket aguardando por conexoes */
		close(s);

		/* Processo filho obtem seu proprio pid */
		fid = getpid();

		/* Recebe uma mensagem do cliente atraves do novo socket conectado */
		if (recv(ns, recvbuf, sizeof(recvbuf), 0) == -1)
		{
		    perror("Recv()");
		    exit(6);
		}
		
		printf("[%d] Recebida a mensagem do endereco IP %s da porta %d\n", fid, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		printf("[%d] Mensagem recebida do cliente: %s\n", fid, recvbuf);
		
		printf("[%d] Aguardando 10 s ...\n", fid);
		sleep(10);
		
		strcpy(sendbuf, "Resposta");
		
		/* Envia uma mensagem ao cliente atraves do socket conectado */
		if (send(ns, sendbuf, strlen(sendbuf)+1, 0) < 0)
		{
		    perror("Send()");
		    exit(7);
		}
		printf("[%d] Mensagem enviada ao cliente: %s\n", fid, sendbuf);

		/* Fecha o socket conectado ao cliente */
		close(ns);

		/* Processo filho termina sua execucao */
		printf("[%d] Processo filho terminado com sucesso.\n", fid);
		exit(0);
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
