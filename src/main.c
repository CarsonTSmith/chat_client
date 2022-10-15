#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// connection port
#define PORT 8085

// max amount of data that can be sent
#define BUF_SIZE 1024


void *rd_from_socket(void *arg)
{
	char buf[BUF_SIZE];
	int sockfd = *(int *)arg;
	
	while (1) {
		if (read(sockfd, buf, BUF_SIZE) < 1) {
			printf("Server disconnected, exiting\n");
			exit(-1);
		}

		printf("%s\n", buf);
		memset(buf, 0, sizeof(buf));
	}

	return NULL;
}

/*
 * Function writes message to the server
 *
 * The first character send will always be a 1
 * which will signal the beginning of a message.
 * This is so we can tell if a msg is new or part
 * of another msg.
 */
void write_to_socket(const int sockfd, const char *username)
{
	char *str = NULL;
	char fullstr[BUF_SIZE];
	size_t n = 0;

	while (1) {
		getline(&str, &n, stdin);
		snprintf(fullstr, BUF_SIZE, "1%s: %s", username, str);
		if (write(sockfd, fullstr, strlen(fullstr) + 1) < 0) {
			printf("Server disconnected, exiting\n");
			exit(-1);
		}

		memset(fullstr, 0, sizeof(fullstr));
	}
}

// client
int main(int argc, char *argv[])
{
	int srvrfd, clientfd;
	struct sockaddr_in serv_addr;
	pthread_t rdthrd;

	if ((srvrfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if ((clientfd = connect(srvrfd, (struct sockaddr*)&serv_addr, 
	     sizeof(serv_addr))) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}

	pthread_create(&rdthrd, NULL, &rd_from_socket, &srvrfd);

	// argv[1] is username
	write_to_socket(srvrfd, argv[1] == NULL ? "Anonymous" : argv[1]);

	// closing the connected socket
	close(clientfd);

	return 0;
}
