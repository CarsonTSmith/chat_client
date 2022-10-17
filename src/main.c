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
#define BUFSZ 8192
#define HEADERSZ 8

#define EXIT_CLIENT_SEQ "!!exit"
#define EXIT_CLIENT_SEQ_LEN (sizeof(EXIT_CLIENT_SEQ)/sizeof(EXIT_CLIENT_SEQ[0]))

static void *rd_from_socket(void *arg)
{
	char buf[BUFSZ];
	int sockfd = *(int *)arg;
	
	while (1) {
		if (read(sockfd, buf, BUFSZ) < 1) {
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
static void write_to_socket(const int sockfd, const char *username)
{
	char *msg = NULL;
	char msg_w_name[BUFSZ], check_exit[EXIT_CLIENT_SEQ_LEN + 1];
	char msgsz[HEADERSZ + 1]; // + 1 for null char
	char fullmsg[BUFSZ];
	size_t n = 0;

	while (1) {
		getline(&msg, &n, stdin);
		snprintf(check_exit, EXIT_CLIENT_SEQ_LEN, "%s", msg);
		if (!strcmp(EXIT_CLIENT_SEQ, check_exit))
			break;
		
		msg[strlen(msg) - 1] = '\0'; // remove '\n'	
		snprintf(msg_w_name, BUFSZ, "%s: %s", username, msg);
		snprintf(msgsz, HEADERSZ + 1, "%08d", // 08 b/c HEADERSZ is 8
                         (int)(strlen(msg_w_name) + 1));
		snprintf(fullmsg, BUFSZ, "%s%s", msgsz, msg_w_name);
		if (write(sockfd, fullmsg, strlen(fullmsg) + 1) < 0) {
			printf("Server disconnected, exiting\n");
			exit(-1);
		}

		memset(fullmsg, 0, sizeof(fullmsg));
	}
}

static void greeting()
{
	printf("Welcome to the server\n"
		"You can exit by typing: !!exit\n");
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

	greeting();
	pthread_create(&rdthrd, NULL, &rd_from_socket, &srvrfd);

	// argv[1] is username
	write_to_socket(srvrfd, argv[1] == NULL ? "Anonymous" : argv[1]);

	// closing the connected socket
	close(clientfd);

	return 0;
}
