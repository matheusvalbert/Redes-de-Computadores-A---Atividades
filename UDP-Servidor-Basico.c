#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*
 * Servidor UDP
 */
int main(argc, argv)
int argc;
char **argv;
{
   int sockint,s, namelen, client_address_size;
   struct sockaddr_in client, server;
   char buf[32];

   /*
    * Cria um socket UDP (dgram). 
    */
   if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
       perror("socket()");
       exit(1);
   }

   /*
    * Define a qual endere�o IP e porta o servidor estar� ligado.
    * Porta = 0 -> faz com que seja utilizada uma porta qualquer livre.
    * IP = INADDDR_ANY -> faz com que o servidor se ligue em todos
    * os endere�os IP
    */
   server.sin_family      = AF_INET;   /* Tipo do endere�o             */
   server.sin_port        = htons(atoi(argv[1]));         /* Escolhe uma porta dispon�vel */
   server.sin_addr.s_addr = INADDR_ANY;/* Endere�o IP do servidor      */

   /*
    * Liga o servidor � porta definida anteriormente.
    */
   if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
   {
       perror("bind()");
       exit(1);
   }

   /* Consulta qual porta foi utilizada. */
   namelen = sizeof(server);
   if (getsockname(s, (struct sockaddr *) &server, &namelen) < 0)
   {
       perror("getsockname()");
       exit(1);
   }

   /* Imprime qual porta foi utilizada. */
   printf("Porta utilizada � %d\n", ntohs(server.sin_port));

   /*
    * Recebe uma mensagem do cliente.
    * O endere�o do cliente ser� armazenado em "client".
    */
	   do{
	   client_address_size = sizeof(client);
	   if(recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &client,
		    &client_address_size) <0)
	   {
	       perror("recvfrom()");
	       exit(1);
	   }
	   if(sendto(s, buf, (strlen(buf)+1), 0, (struct sockaddr *) &client,
	   sizeof(client)) <0)
	   {
	      perror("sendto()");
	      exit(2);
	   }
	   /*
	    * Imprime a mensagem recebida, o endere�o IP do cliente
	    * e a porta do cliente 
	    */
	   printf("Recebida a mensagem %s do endere�o IP %s da porta %d\n",buf,inet_ntoa(client.sin_addr),ntohs(client.sin_port));
	   }while(1);

   /*
    * Fecha o socket.
    */

   close(s);
}
