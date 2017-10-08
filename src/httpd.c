/* your code goes here. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

const int PORT = 1111;

int main(int argc, char *argv[])
{
	int sockfd = -1, new_sockfd = -1;
	int length, rc,on = 1;
	int descriptor_rdy, end_server = FALSE, compress_array = FALSE;
	int close_connection;
	char buffer[1024];
	struct sockaddr_in server_addres;
	int timeout;
	struct pollfd fds[200]
	int fds = 1, i, j, current_size = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("error sockfd");
		exit(-1);
	}

	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if(rc < 0){
		printf("error setsockopt");
		close(sockfd);
		exit(-1);
	}

	
	rc = ioctl(sockfd, FIONBIO, (char *)&on);	
	if(rc < 0){
		printf("ioctl");
		close(sockfd);
		exit(-1);
	}
	
	memset(&server_addres, 0, sizeof(server_addres));
	server_addres.sin_family = AF_NET
	server_addres.sin_port = htons(PORT);
	server_addres.sin_addr.s_addr = INADDR_ANY;

	rc = bind(sockfd, (struct sockaddr *)&server_addres, sizeof(server_addres));
	if(rc < 0){
		printf("error bind");
		close(sockfd);
		exit(-1);
	}

	rc = listen(sockfd,32);
	if(rc < 0){
		printf("error listen");
		close(sockfd);
		exit(-1);		
	}

	
	return 0
}
