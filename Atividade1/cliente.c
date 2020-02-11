#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio_ext.h>

/*
 * Cliente UDP
 */
int main(argc, argv)
int argc;
char **argv;
{


   int s, s2,server_address_size;
   unsigned short port;
   struct sockaddr_in server, receive;
   char buf[250];

   /* 
    * O primeiro argumento (argv[1]) � o endere�o IP do servidor.
    * O segundo argumento (argv[2]) � a porta do servidor.
    */
   if(argc != 3)
   {
      printf("Use: %s enderecoIP porta\n",argv[0]);
      exit(1);
   }
  port = htons(atoi(argv[2]));

   /*
    * Cria um socket UDP (dgram).
    */
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
       perror("socket()");
       exit(1);
   }

   /* Define o endere�o IP e a porta do servidor */
   server.sin_family      = AF_INET;            /* Tipo do endere�o         */
   server.sin_port        = port;               /* Porta do servidor        */
   server.sin_addr.s_addr = inet_addr(argv[1]); /* Endere�o IP do servidor  */

/* Imprime qual porta foi utilizada. */
   printf("Porta utilizada eh %d\n", ntohs(server.sin_port));

	while(1)
	{
		printf("> ");
		__fpurge(stdin);
		fgets(buf, sizeof(buf), stdin);
		if(buf[strlen(buf)-1] == '\n')
			buf[strlen(buf)-1] = '\0';
		if(strcmp(buf,"exit") ==0)
		{	
			close(s);
			return 0;
		}

		/* Envia a mensagem no buffer para o servidor */
		if (sendto(s, buf, strlen(buf)+1, 0, (struct sockaddr *)&server, sizeof(server)) < 0)
		{
			perror("sendto()");
			exit(2);
		}
		
		server_address_size = sizeof(server);
		if(recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &server,&server_address_size) <0)
		{
			perror("recvfrom()");
			exit(1);
		}
		printf("Recebida a mensagem %s do endere�o IP %s da porta %d\n",buf,inet_ntoa(server.sin_addr),ntohs(server.sin_port));
	}


   return 0;
}