#ifndef SOCKLIB_H_INCLUDED
#define SOCKLIB_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <strings.h>

#define HOSTLEN 256
#define BACKLOG 1

using namespace std;

int make_server_socket(int portnum);
int make_server_socket_q(int portnum, int backlog);
int connect_to_server(char * host, int portnum);

int make_server_socket(int portnum)
{
	return make_server_socket_q(portnum, BACKLOG);
}

int make_server_socket_q(int portnum, int backlog)
{
	struct sockaddr_in saddr; /*buildouraddresshere*/
	struct hostent *hp; /*thisispartofour */
	char hostname[HOSTLEN]; /*address */
	int sock_id; /*thesocket */
	sock_id = socket(PF_INET, SOCK_STREAM, 0); /*getasocket*/
	if (sock_id == -1)
        return -1;
	/**buildaddressandbindittosocket**/
	bzero((void*)&saddr, sizeof(saddr)); /*clearoutstruct */
	gethostname(hostname, HOSTLEN); /*whereamI? */
	hp = gethostbyname(hostname); /*getinfoabouthost */
	bcopy((void*)hp->h_addr, (void*)&saddr.sin_addr, hp->h_length); /*fillinhostpart */
	saddr.sin_port = htons(portnum); /*fillinsocketport */
	saddr.sin_family = AF_INET; /*fillinaddrfamily */
	if (bind(sock_id, (struct sockaddr*)&saddr, sizeof(saddr)) != 0)
		return -1;
	/**arrangeforincomingcalls**/
	if (listen(sock_id, backlog) != 0)
		return -1;
	return sock_id;
}

int connect_to_server(char * host, int portnum)
{
	int sock;
	struct sockaddr_in servadd; /*thenumbertocall*/
	struct hostent * hp; /*usedtogetnumber*/
						 /**Step1:Getasocket**/
	sock = socket(AF_INET, SOCK_STREAM, 0); /*getaline */
	if (sock == -1)
		return -1;
	/**Step2:connecttoserver**/
	bzero(&servadd, sizeof(servadd)); /*zerotheaddress */
	hp = gethostbyname(host); /*lookuphost'sip# */
	if (hp == NULL)
		return -1;
	bcopy(hp->h_addr, (struct sockaddr*)&servadd.sin_addr, hp->h_length);
	servadd.sin_port = htons(portnum); /*fillinportnumber */
	servadd.sin_family = AF_INET; /*fillinsockettype */
	if (connect(sock, (struct sockaddr*)&servadd, sizeof(servadd)) != 0)
		return -1;
	return sock;
}



#endif // SOCKLIB_H_INCLUDED

