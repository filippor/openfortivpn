/*
 * cookieRetriever.c
 *
 *  Created on: 28 apr 2024
 *      Author: filippor
 */
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "http.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_REQUEST_SIZE 4096
// Function to parse HTTP request and extract parameter "id"
char* parse_request(const char *request) {
	char *id = NULL;
	char *token = strtok((char*) request, " ");
	while (token != NULL) {
		if (strstr(token, "id=") != NULL) {
			// Found "id=" parameter
			id = strchr(token, '=') + 1;
			break;
		}
		token = strtok(NULL, " ");
	}
	return id;
}

char* retrieve_id_with_external_browser(struct vpn_config *cfg) {
	int sockfd, newsockfd, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char buffer[MAX_REQUEST_SIZE];

	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Error opening socket");
		exit(1);
	}
	// Initialize server address structure
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(cfg->listen_port);

	// Bind socket to address
	if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error on binding");
		exit(1);
	}

	// Listen for incoming connections
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	log_debug("Server listening on port %d to retrieve the id\n",
			cfg->listen_port);

	char data[512];
	char *url = data;
	sprintf(url, "https://%s:%d/remote/saml/start?redirect=1",
			cfg->gateway_host, cfg->gateway_port);

	if (cfg->realm[0] != '\0') {
		strcat(url, "&realm=");
		char *dt = url + strlen(url);
		url_encode(dt, cfg->realm);
	}
	log_info("open this address: %s\n", url);

	// Accept incoming connections
	newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
	if (newsockfd < 0) {
		log_error("Error on accept");
		return NULL;
	}

	// Read HTTP request from client
	bzero(buffer, MAX_REQUEST_SIZE);
	read(newsockfd, buffer, MAX_REQUEST_SIZE - 1);
	log_debug("Received HTTP request:\n%s\n", buffer);

	// Parse request and extract parameter "id"
	char *id = strdup(parse_request(buffer));
	if (id != NULL) {
		log_debug("Extracted id: %s\n", id);
	} else {
		log_error("id parameter not found\n");
	}
	// Close sockets
	close(newsockfd);
	close(sockfd);

	return id;
}

