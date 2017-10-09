/* your code goes here. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>

#define PORT 1111

int main(int argc, char *argv[])
{
	char html[256] = "HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html; charset=UTF-8\r\n\r\n"
	"<!DOCTYPE html>\r\n"
	"<html><head><title>First</title></head><body>Hello, world!</body></html>\r\n\r\n";

	char *dir;
	int sockfd = -1, new_sockfd = -1;
	int length, rc, on = 1;
	int descriptor_rdy, end_server = 0, compress_array = 0;
	int close_connection;
	char buffer[1024];
	struct sockaddr_in server_addres;
	int timeout;
	struct pollfd fds[200];
	int fd = 1;
	int i, j, current_size = 0;

	//Default directory /
	dir = getenv("PWD");

	//Initialize listen socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		printf("error sockfd\n");
		exit(-1);
	}

	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if(rc < 0){
		printf("error setsockopt\n");
		close(sockfd);
		exit(-1);
	}

	
	rc = ioctl(sockfd, FIONBIO, (char *)&on);	
	if(rc < 0){
		printf("error ioctl\n");
		close(sockfd);
		exit(-1);
	}
	
	memset(&server_addres, 0, sizeof(server_addres));
	server_addres.sin_family = AF_INET;
	server_addres.sin_port = htons(PORT);
	server_addres.sin_addr.s_addr = INADDR_ANY;
	rc = bind(sockfd, (struct sockaddr *)&server_addres, sizeof(server_addres));
	if(rc < 0){
		printf("error bind\n");
		close(sockfd);
		exit(-1);
	}

	rc = listen(sockfd,32);
	if(rc < 0){
		printf("error listen\n");
		close(sockfd);
		exit(-1);		
	} else {
		printf("Server now listening on port %d\n", PORT);
	}
	int client;
	struct sockaddr_in client_address;
	socklen_t address_length;	
	while(1) {
		address_length = sizeof(client_address);
		client = accept (sockfd, (struct sockaddr *) &client_address, &address_length);
			if (client > 0) {
				if(fork() == 0) {
					fprintf(stdout, "Connection established\n");
					recv(client, buffer, 1024, 0);
					printf(buffer);
					//printf("%d\n", client_address);
					if(buffer)
						write(client, html, sizeof(html)-1);
						printf("Shutting down...\n");
					shutdown(client, SHUT_RDWR);
					close(client);
					exit(0);	
				}		
			}
	}
}
