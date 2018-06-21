//
//  http.c
//  xdag
//
//  Created by Rui Xie on 5/25/18.
//  Copyright © 2018 xrdavies. All rights reserved.
//

#include "http.h"

#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#if !defined(_WIN32) && !defined(_WIN64)
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#else

#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "url.h"
#include "../utils/log.h"
#include "../system.h"



// Simple structure to keep track of the handle, and
// of what needs to be freed later.
typedef struct {
	int socket;
	SSL *sslHandle;
	SSL_CTX *sslContext;
} connection;

connection * tcpConnect(const char* h, int port);
void tcpDisconnect(connection *c);
char* tcpRead(connection *c);
void tcpWrite(connection *c, char *text);

connection *sslConnect(const char* h, int port);
void sslDisconnect(connection *c);
char *sslRead(connection *c);
void sslWrite(connection *c, char *text);

// Establish a regular tcp connection
connection * tcpConnect(const char* h, int port)
{
	int sock = 0 ;
	struct sockaddr_in server;
	if(!strcmp(h, "any")) {
		server.sin_addr.s_addr = htonl(INADDR_ANY);
	} else if(!inet_aton(h, &server.sin_addr)) {
		struct hostent *host = gethostbyname(h);
		server.sin_addr = *((struct in_addr *) host->h_addr);
	}
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		xdag_err("Create sock error, %s", strerror(sock));
		sock = 0;
	} else {
		server.sin_family = AF_INET;
		server.sin_port = htons(port);
		
		memset(&(server.sin_zero), 0, 8);
		
		uint32_t timeout = 1000*10;
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		int error = connect(sock,(struct sockaddr *) &server, sizeof(struct sockaddr));
		if(error == -1) {
			xdag_err("Connect error, %s", strerror(error));
			sock = 0;
		}
	}
	
	if(sock) {
		connection *c = malloc(sizeof(connection));
		c->sslHandle = NULL;
		c->sslContext = NULL;
		c->socket = sock;
		return c;
	} else {
		return NULL;
	}
}

// Disconnect & free connection struct
void tcpDisconnect(connection *c)
{
	if(c->socket) {
		close(c->socket);
	}
	
	free(c);
}

char *tcpRead(connection *c)
{
	const int readSize = 1023;
	char *rc = NULL;
	size_t received = 0, count = 0;
	char buffer[1024];
	
	if(c) {
		while(1) {
			if(!rc) {
				rc = malloc(readSize * sizeof(char) + 1);
			} else {
				rc = realloc(rc,(count + 1) * readSize * sizeof(char) + 1);
			}
			
			memset(rc + count * readSize,0,readSize + 1);
			memset(buffer, 0, 1024);
			received = read(c->socket, buffer, readSize);

			if(received > 0) {
				strcat(rc, buffer);
			} else {
				break;
			}

//			if(received < readSize) {
//				break;
//			}
			
			count++;
		}
	}
	
	return rc;
}

void tcpWrite(connection *c, char *text)
{
	if(c) {
		write(c->socket, text, strlen(text));
	}
}

// Establish a connection using an SSL layer
connection *sslConnect(const char* h, int port)
{
	int sock = 0 ;
	struct sockaddr_in server;
	if(!strcmp(h, "any")) {
		server.sin_addr.s_addr = htonl(INADDR_ANY);
	} else if(!inet_aton(h, &server.sin_addr)) {
		struct hostent *host = gethostbyname(h);
		server.sin_addr = *((struct in_addr *) host->h_addr);
	}
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		xdag_err("Create sock error, %s", strerror(sock));
		sock = 0;
	} else {
		server.sin_family = AF_INET;
		server.sin_port = htons(port);
		
		memset(&(server.sin_zero), 0, 8);
		
		uint32_t timeout = 1000*10;
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		int error = connect(sock,(struct sockaddr *) &server, sizeof(struct sockaddr));
		if(error == -1) {
			xdag_err("Connect error, %s", strerror(error));
			sock = 0;
		}
	}
	
	if(sock) {
		connection *c = malloc(sizeof(connection));
		c->sslHandle = NULL;
		c->sslContext = NULL;
		c->socket = sock;
		
		// Register the error strings for libcrypto & libssl
		SSL_load_error_strings();
		
		// Register the available ciphers and digests
		SSL_library_init();
		OpenSSL_add_all_algorithms();
		
		// New context saying we are a client, and using SSL 2 or 3
		c->sslContext = SSL_CTX_new(SSLv23_client_method());
		if(c->sslContext == NULL) {
			ERR_print_errors_fp(stderr);
		}
		
		// Create an SSL struct for the connection
		c->sslHandle = SSL_new(c->sslContext);
		if(c->sslHandle == NULL) {
			ERR_print_errors_fp(stderr);
		}
		
		// Connect the SSL struct to our connection
		if(!SSL_set_fd(c->sslHandle, c->socket)) {
			ERR_print_errors_fp(stderr);
		}
		
		// Initiate SSL handshake
		if(SSL_connect(c->sslHandle) != 1) {
			ERR_print_errors_fp(stderr);
		}
		
		return c;
	} else {
		xdag_err("Creat ssl sock failed.");
		return NULL;
	}
}

// Disconnect & free connection struct
void sslDisconnect(connection *c)
{
	if(c->socket) {
		close(c->socket);
	}
	
	if(c->sslHandle) {
		SSL_shutdown(c->sslHandle);
		SSL_free(c->sslHandle);
	}
	
	if(c->sslContext) {
		SSL_CTX_free(c->sslContext);
	}
	
	free(c);
}

// Read all available text from the connection
char *sslRead(connection *c)
{
	const int readSize = 1024;
	char *rc = NULL;
	int received = 0, count = 0;
	char buffer[1025] = {0};
	
	if(c) {
		while(1) {
			if(!rc) {
				rc = malloc(readSize * sizeof(char) + 1);
			} else {
				rc = realloc(rc,(count + 1) * readSize * sizeof(char) + 1);
			}
			
			memset(rc + count * readSize,0,readSize + 1);
			memset(buffer, 0, 1024);
			received = SSL_read(c->sslHandle, buffer, readSize);
			
			if(received > 0) {
				strcat(rc, buffer);
			} else {
				break;
			}
			
//			if(received < readSize) {
//				break;
//			}
			count++;
		}
	}

	return rc;
}

// Write text to the connection
void sslWrite(connection *c, char *text)
{
	if(c) {
		SSL_write(c->sslHandle, text, (int)strlen(text));
	}
}

char *http_get(const char *url)
{
	char request[1024] = {0};
	char *resp = NULL;
	
	url_field_t *fields = url_parse(url);
	
	if(fields->host) {
		char *path = "";
		if(fields->path) {
			path = fields->path;
		}
		strcat(request, "GET /"); strcat(request, path); strcat(request, " HTTP/1.1\r\n");
		strcat(request, "HOST: "); strcat(request, fields->host); strcat(request, "\r\n");
		strcat(request, "Connection: close\r\n\r\n");

		if(!strcmp("https",fields->schema)) {
			int port = 443;
			if(fields->port) {
				port = atoi(fields->port);
			}
			
			connection *conn = sslConnect(fields->host, port);
			if(conn) {
				sslWrite(conn, request);
				resp = sslRead(conn);
				sslDisconnect(conn);
			}
		} else if(!strcmp("http", fields->schema)) {
			int port = 80;
			if(fields->port) {
				port = atoi(fields->port);
			}
			
			connection *conn = tcpConnect(fields->host, port);
			if(conn) {
				tcpWrite(conn, request);
				resp = tcpRead(conn);
				tcpDisconnect(conn);
			}
		} else {
			xdag_err("schema not supported yet! schema: %s", fields->schema);
		}
	}
	
	if(resp) {
		char *ptr = strstr(resp, "\r\n\r\n");
		if(ptr) {
			char *ori = resp;
			resp = strdup(ptr + 4);
			free(ori);
		}
	}
	
	url_free(fields);
	return resp;
}

// Very basic main: we send GET / and print the resp.
int test_https(void)
{
	connection *c;
	char *resp;
	
	c = sslConnect("raw.githubusercontent.com", 443);
	
	sslWrite(c, "GET /XDagger/xdag/master/client/netdb-white.txt HTTP/1.1\r\nHost: raw.githubusercontent.com\r\nconnection: close\r\n\r\n");
	resp = sslRead(c);
	
	printf("[content] %s\n", resp);
	
	sslDisconnect(c);
	free(resp);
	
	return 0;
}

int test_http(void)
{
	connection *c;
	char *resp;
	
	c = tcpConnect("raw.githubusercontent.com", 80);
	
	tcpWrite(c, "GET /XDagger/xdag/master/client/netdb-white.txt HTTP/1.1\r\nHost: raw.githubusercontent.com\r\nconnection: close\r\n\r\n");
	resp = tcpRead(c);
	
	printf("[content] %s\n", resp);
	
	tcpDisconnect(c);
	free(resp);
	
	return 0;
}
