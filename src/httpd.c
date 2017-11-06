#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <glib.h>

void log(char ipport[15], char method[], char url[], char response[]) {
	printf("%s, %s, %s, %s", ipport, method, url, response);
	//Credit to einargizz
	FILE *logfile = NULL;
	logfile = fopen("httpd.log","a");
	if(logfile == NULL) {
		perror("creating/opening of logfile failed, abandon ship!");
		exit(-1);
	}
	time_t now = time(NULL);
	struct tm *now_tm = gmtime(&now);
	char iso_8601[] = "YYYY-MM-DDThh:mm:ssTZD";
	strftime(iso_8601, sizeof iso_8601, "%FT%T%Z", now_tm);
	GString *msg = g_string_new(iso_8601);
	g_string_append_printf(msg, " : %s %s %s : %s", ipport, method, url, response);
	fprintf(logfile, "%s\n", msg->str);
	fflush(logfile);
	g_string_free(msg, 1);
	return;
}

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
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	int client = 0;
	socklen_t address_length;
	int port;
	int timeout_milliseconds = 5;
	struct pollfd fds[2];	//fds[0] for http and fds[1] for https

	//Default directory /
	dir = getenv("PWD");

	//Initialize listen socket
	for(int i = 0; i < 2; i++) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0){
			perror("error sockfd\n");
			exit(-1);
		}
		if(argc > 1) {
			port = atoi(argv[1])+i;
		} else {
			port = 1111 + i;
		}
		memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(port);
		server_address.sin_addr.s_addr = INADDR_ANY;
		rc = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
		if(rc < 0){
			perror("error bind\n");
			close(sockfd);
			exit(-1);
		}

		rc = listen(sockfd, 32);
		if(rc < 0){
			perror("error listen\n");
			close(sockfd);
			exit(-1);
		} else {
			fds[i].fd = sockfd;
			fds[i].events = POLLIN;
			printf("Server now listening on port %d with fd %d\n", port, fds[i].fd);
		}
	}

	while(1) {
		int ret = poll(fds, 2, timeout_milliseconds);
		//printf("Hello\n");
		if(ret > 0) {
			for(int i = 0; i < 2; i++) {
				printf("%d - %d\n",i, fds[i].events);
				if(fds[i].revents & POLLIN) {
					address_length = sizeof(client_address);
					client = accept(fds[i].fd, (struct sockaddr *)&client_address, &address_length);
					printf("%d, %d\n", client_address.sin_addr, client_address.sin_port);
					char addr_str[10];
					inet_ntop(AF_INET, &(client_address.sin_addr), addr_str, INET_ADDRSTRLEN);
					//printf("%s\n", addr_str);
					if(client > 0) {
						recv(client, buffer, 1024, 0);
					}
					if(fds[i].revents & POLLHUP) {
						printf("She hung up on me!\n");
					}
					if(strncasecmp(buffer, "GET /favicon", 12) == 0) {
						char response[256] = "HTTP/1.1 404 NOT FOUND\r\n";
						write(client, response, strlen(response));
					}else if(strncasecmp(buffer, "GET", 3) == 0) {
						//printf("%s\n", buffer);
						char new_body[1024] = {0};
						strcpy(new_body, html);
						char *url;
						url = strstr(buffer, "Host");
						url += 6;
						char *end;
						end = strstr(buffer, "Connection");
						end -=  url;
						char site_body[256];
						strcpy(&site_body, "http://");
						strncat(&site_body, url, end-7);
						strcat(&site_body, " ");
						strncat(&site_body, addr_str, 15);
						strcat(&site_body, ":");
						char portno[10];
						snprintf(portno, 6, "%d", client_address.sin_port);
						strncat(&site_body, portno, 5);
						strcat(&site_body, "</body></html>\r\n\r\n");
						strncpy(&new_body[121], site_body, strlen(site_body));
						write(client, new_body, strlen(new_body));
						char ipport[20];
						strncpy(&ipport, addr_str, 15);
						strcat(&ipport, ":");
						strncat(&ipport, portno, 5);
						printf("%s\n", ipport);
						log(ipport, "GET", "http://localhost", "200");
					} else if(strncasecmp(buffer, "POST", 4) == 0) {
						char new_body[1024] = {0};
						char *url;
						url = strstr(buffer, "Host");
						strcpy(new_body, html);
						url += 6;
						char *end;
						end = strstr(buffer, "Connection");
						end -=  url;
						char *site;
						site = strstr(url, ":");
						char site_body[256];
						strcpy(&site_body, "http://");
						strncat(&site_body, url, end-7);
						strncat(&site_body, " ", 1);
						strncat(&site_body, addr_str, 15);
						strncat(&site_body, site, 5);
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
						char response[256] = "HTTP/1.1 501 NOT IMPLEMENTED\r\n";
						write(client, response, strlen(response));
					}
					printf("Shutting client portal...\n");
					shutdown(client, SHUT_RDWR);
					close(client);
					client = -1;
				}
			}
		}
	}
}
