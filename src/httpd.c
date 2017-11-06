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

void log(char ipport[15], char method[], char url[], char response[]) {	//Logs information about a request into logfile
	//Credit to einargizz@github
	FILE *logfile = NULL;
	logfile = fopen("httpd.log","a");	//File to which to write
	if(logfile == NULL) {
		perror("creating/opening of logfile failed, abandon ship!");
		exit(-1);
	}
	time_t now = time(NULL);
	struct tm *now_tm = gmtime(&now);
	char iso_8601[] = "YYYY-MM-DDThh:mm:ssTZD";
	strftime(iso_8601, sizeof iso_8601, "%FT%T%Z", now_tm);	//Mapping date to ISO 8601
	GString *msg = g_string_new(iso_8601);
	g_string_append_printf(msg, " : %s %s %s : %s", ipport, method, url, response);	//Adding all other info to log entry
	fprintf(logfile, "%s\n", msg->str);	//Write log entry into output buffer
	fflush(logfile);					//Flush to definitely write out
	g_string_free(msg, 1);				//Free allocated space
	return;
}

int main(int argc, char *argv[])
{
	//A base html page to fill in and respond with
	char html[256] = "HTTP/1.1 200 OK\r\n"
	"Content-Type: text/html; charset=UTF-8\r\n\r\n"
	"<!DOCTYPE html>\r\n"
	"<html><head><title>First</title></head><body>Hello, world!</body></html>\r\n\r\n";

	int sockfd = -1;					//file descriptor, to be defined and tossed aside
	int rc = 1;							//return code to be set to return value of various functions to error check
	char buffer[1024];					//char buffer to read requests
	struct sockaddr_in server_address;	//struct containing ip address, port number and more
	struct sockaddr_in client_address;	
	int client = 0;						//client file descriptor, to be defined
	socklen_t address_length;			//length of ip address
	int port;							//port number
	int timeout_milliseconds = 5;		//poll() timeout in milliseconds
	struct pollfd fds[2];				//fds[0] for http and fds[1] for https if it had been implemented

	//Initialize listen socket
	for(int i = 0; i < 2; i++) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);	//init
		if(sockfd < 0){	//error?
			perror("error sockfd\n");
			exit(-1);
		}
		if(argc > 1) {								//Port number is 1111 unless specified as command line argument
			port = atoi(argv[1])+i;
		} else {
			port = 1111 + i;						//Port number of fds[1] = port number of fds[0] + 1
		}
		memset(&server_address, 0, sizeof(server_address));	//erase data from server_address
		server_address.sin_family = AF_INET;				//set attributes
		server_address.sin_port = htons(port);
		server_address.sin_addr.s_addr = INADDR_ANY;
		rc = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));	//bind socket
		if(rc < 0){	//error?
			perror("error bind\n");
			close(sockfd);
			exit(-1);
		}

		rc = listen(sockfd, 32);	//start listening on socket
		if(rc < 0){	//error?
			perror("error listen\n");
			close(sockfd);
			exit(-1);
		} else {
			fds[i].fd = sockfd; 	//fds[i].fd will take over for sockfd from here
			fds[i].events = POLLIN;	
			printf("Server now listening on port %d with fd %d\n", port, fds[i].fd);
		}
	}

	while(1) {
		int ret = poll(fds, 2, timeout_milliseconds);	//Wait for activity on sockets
		if(ret > 0) {									//If there's activity
			for(int i = 0; i < 2; i++) {	
				if(fds[i].revents & POLLIN) {
					address_length = sizeof(client_address);	//Accept client's connection and set file descriptor for connection
					client = accept(fds[i].fd, (struct sockaddr *)&client_address, &address_length);
					char addr_str[10];			//Convert and save client ip address in xxx.xxx.xxx.xxx format
					inet_ntop(AF_INET, &(client_address.sin_addr), addr_str, INET_ADDRSTRLEN);
					if(client > 0) {	//if accept() successful, read request into buffer
						recv(client, buffer, 1024, 0);
					}
					if(strncasecmp(buffer, "GET /favicon", 12) == 0) {	//there's no favicon, 404 beep boop
						char response[256] = "HTTP/1.1 404 NOT FOUND\r\n";
						write(client, response, strlen(response));
					} else if (strncasecmp(buffer, "GET", 3) == 0) {	//If it is a get request
						char new_body[1024] = {0};		//Completely empty string to be filled out and returned
						strcpy(new_body, html);			//Copy base html into return string
						char *url;
						url = strstr(buffer, "Host");
						url += 6;						//Find (pointer to) index of the url within the request string
						char *end;						
						end = strstr(buffer, "Connection");	
						end -=  url;					//Find index of the end of the url + 2
						char site_body[256];			//Html body to be built here
						strcpy(&site_body, "http://");	//Start with http://
						strncat(&site_body, url, end-7);//Add url without port number
						char uri[200];
						strcpy(&uri, site_body);		//Save what's in body so far (url)
						strcat(&site_body, " ");		//Space
						printf("%s\n", uri);
						strncat(&site_body, addr_str, 15);	//Add ip address
						strcat(&site_body, ":");		//Colon
						char portno[10];				//Client port number to be saved here
						snprintf(portno, 6, "%d", client_address.sin_port);	//length 6 to be safe
						strncat(&site_body, portno, 5);	//Port numbere added
						strcat(&site_body, "</body></html>\r\n\r\n");	//Close html tags
						strncpy(&new_body[121], site_body, strlen(site_body));	//Add body to return string
						write(client, new_body, strlen(new_body));	//Respond to request with return string
						char ipport[21];	//Ip address and port number to be saved here
						strncpy(&ipport, addr_str, 15);
						strcat(&ipport, ":");
						strncat(&ipport, portno, 5);
						log(ipport, "GET", uri, "200");	//Send all pertinent information to log() to be logged
					} else if(strncasecmp(buffer, "POST", 4) == 0) {	//If an http post request
						char new_body[1024] = {0};	//Completely empty string to be filled out and returned
						char *url;
						url = strstr(buffer, "Host");
						strcpy(new_body, html);		//Start with base html
						url += 6;					//Find (pointer to) index of the url within the request string
						char *end;
						end = strstr(buffer, "Connection");
						end -=  url;				//Find index of the end of the url + 2
						char *site;
						site = strstr(url, ":");	//Find index of where url ends
						char site_body[256];		//Html body to be built here
						strcpy(&site_body, "http://");
						strncat(&site_body, url, end-7);
						strncat(&site_body, " ", 1);
						strncat(&site_body, addr_str, 15);
						strncat(&site_body, site, 5);	//Build body
						char *accept;
						accept = strstr(buffer, "Accept-Language");	//Find index of language
						char *post;
						post = strstr(accept, "POST");				//Find index of "POST" within the request
						char *length;
						length = strstr(post, "Content-Length");	//Find index of string representing length of content - 16
						length += 16;								//Skip forward to the good stuff
						char *numend;
						numend = strstr(length, "\n");				//Find index of end of that string
						char *content = numend;
						numend -= length;							//Length of string representing length
						char content_length[10];					//String into which to save that string
						memcpy(content_length, length, numend);		//Save it
						int content_len = atoi(content_length);		//Convert to integer
						content += 2;								//Skip ahead 2, to next line (beginning of content)
						strcat(&site_body, " ");
						strcat(&site_body, content);
						strcat(&site_body, "</body></html>\r\n\r\n");//Copy the content in and close html tags
						strncpy(&new_body[121], site_body, strlen(site_body));	//Copy all of that into response string
						write(client, new_body, strlen(new_body));	//Respond
					} else if(strncasecmp(buffer, "HEAD", 4) == 0) {	//If a head request
						char response[256] = "HTTP/1.1 200 OK\r\n"
						"Content-Type: text/html; charset=UTF-8\r\n\r\n";
						write(client, response, strlen(response));	//Return the header with status code 200 OK
					} else {	//If some other http request, return 501 not implemented
						char response[256] = "HTTP/1.1 501 NOT IMPLEMENTED\r\n";
						write(client, response, strlen(response));
					}
					printf("Shutting client portal...\n");
					shutdown(client, SHUT_RDWR);
					close(client);
					client = -1;	//Close and end iteration
				}
			}
		}
	}
}
