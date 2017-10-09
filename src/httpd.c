/* your code goes here. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	char html[256] = "HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html; charset=UTF-8\r\n\r\n"
	"<!DOCTYPE html>\r\n"
	"<html><head><title>First</title></head><body>Hello, world!</body></html>\r\n\r\n";

	char *dir;
	int sockfd = -1;
	int rc, on = 1;
	char buffer[1024];
	struct sockaddr_in server_addres;
	struct sockaddr_in client_address;
	socklen_t address_length;
	int client = 0;
	int port;
	struct timeval timeout;
	timeout.tv_sec = 30;
	timeout.tv_usec = 0;

	//Default directory /
	dir = getenv("PWD");

	//Initialize listen socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("error sockfd\n");
		exit(-1);
	}

	rc = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
	if(rc < 0){
		perror("error setsockopt\n");
		close(sockfd);
		exit(-1);
	}

	rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	if(rc < 0){
		perror("error setsockopt timeout\n");
		close(sockfd);
		exit(-1);
	}
	
	rc = ioctl(sockfd, FIONBIO, (char *)&on);	
	if(rc < 0){
		perror("error ioctl\n");
		close(sockfd);
		exit(-1);
	}

	if(argc > 1) {
		port = atoi(argv[1]);
	} else {
		port = 1111;
	}
	memset(&server_addres, 0, sizeof(server_addres));
	server_addres.sin_family = AF_INET;
	server_addres.sin_port = htons(port);
	server_addres.sin_addr.s_addr = INADDR_ANY;
	rc = bind(sockfd, (struct sockaddr *)&server_addres, sizeof(server_addres));
	if(rc < 0){
		perror("error bind\n");
		close(sockfd);
		exit(-1);
	}

	rc = listen(sockfd,32);
	if(rc < 0){
		perror("error listen\n");
		close(sockfd);
		exit(-1);		
	} else {
		printf("Server now listening on port %d\n", port);
	}

	while(1) {
		address_length = sizeof(client_address);
		client = accept(sockfd, (struct sockaddr *)&client_address, &address_length);
			if (client > 0) {
				//Doesn't matter if single- or multithreaded, works the same(aside from thread safety). No use repeating the code.
				if(fork() == 0) {
					fprintf(stdout, "Connection established\n");
					recv(client, buffer, 1024, 0);
					if(strncasecmp(buffer, "GET /favicon", 12) == 0) {
						char response[256] = "HTTP/1.1 404 NOT FOUND\r\n";
						write(client, response, strlen(response));
					} else if(strncasecmp(buffer, "GET", 3) == 0) {
						char new_body[1024];
						strcpy(new_body, html);
						char *url;
						url = strstr(buffer, "Host");
						url += 6;
						char *end;
						end = strstr(buffer, "Connection");
						end -=  url;
						char site_body[256];
						strcpy(&site_body, "http://");
						strncat(&site_body, url, end-2);
						strcat(&site_body, "</body></html>\r\n\r\n");
						strncpy(&new_body[121], site_body, strlen(site_body));
						//printf("%s", inet_ntop(client_address.sin_addr));						
						write(client, new_body, strlen(new_body));
					} else if(strncasecmp(buffer, "POST", 4) == 0) {
						char new_body[1024];
						strcpy(new_body, html);
						printf(buffer);
						char *url;
						url = strstr(buffer, "Host");
						url += 6;
						char *end;
						end = strstr(buffer, "Connection");
						end -=  url;
						char site_body[256];
						strcpy(&site_body, "http://");
						strncat(&site_body, url, end-2);
						char *accept;
						accept = strstr(buffer, "Accept-Language");
						char *post;
						post = strstr(accept, "POST");
						char *length;
						length = strstr(post, "Content-Length");
						length += 16;
						char *numend;
						numend = strstr(length, "\n");
						char *content = numend;
						numend -= length;
						char content_length[10];
						memcpy(content_length, length, numend);
						int content_len = atoi(content_length);
						content += 2;
						strcat(&site_body, " ");
						strcat(&site_body, content);
						strcat(&site_body, "</body></html>\r\n\r\n");
						strncpy(&new_body[121], site_body, strlen(site_body));
						write(client, new_body, strlen(new_body));
					} else if(strncasecmp(buffer, "HEAD", 4) == 0) {
						char response[256] = "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n\r\n";
						write(client, response, strlen(response));
					} else {
						char response[256] = "HTTP/1.1 400 BAD REQUEST\r\n";
						write(client, response, strlen(response));
					}
					printf("Shutting down...\n");
					shutdown(client, SHUT_RDWR);
					close(client);
					client = -1;
					exit(0);	
				}
			}
	}
}
