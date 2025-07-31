/*
==========================================================
Chat Client
----------------------------------------------------------
Features:
- Connects to the chat server.
- Authenticates using a username.
- Retrieves a list of online users.
- Lets user choose a chat partner.
- Supports non-blocking message reception using threads.
- Uses "/switch" to change chat partners.
- Uses "/list" to refresh the list of users.
==========================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int client_socket;
char current_chat_user[50] = "";

/* ==========================================================
   Thread: receive_messages
   Purpose: Continuously listens for incoming messages from
            the server without blocking the input loop.
========================================================== */
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];

    // Set socket to non-blocking mode
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (bytes > 0) {
            printf("\n%s", buffer);
            printf("You [%s]: ", current_chat_user);
            fflush(stdout);
        } else {
            usleep(100000); // Prevent CPU from busy-waiting
        }
    }
    return NULL;
}

/* ==========================================================
   Function: fetch_online_users
   Purpose: Requests the current list of online users from
            the server and displays it.
========================================================== */
void fetch_online_users() {
    char buffer[BUFFER_SIZE];
    send(client_socket, "/list", 5, 0);
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("\n%s\n", buffer);
}

int main() {
    struct sockaddr_in server_addr;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // === LOGIN ===
    char username[50];
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    send(client_socket, username, strlen(username), 0);

    char buffer[BUFFER_SIZE];
    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("%s", buffer); // Authenticated message

    recv(client_socket, buffer, sizeof(buffer), 0);
    printf("%s", buffer); // Online users

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);

    // === MAIN LOOP ===
    while (1) {
        fetch_online_users();
        printf("\nChoose a user to chat with: ");
        fgets(current_chat_user, sizeof(current_chat_user), stdin);
        current_chat_user[strcspn(current_chat_user, "\n")] = 0;

        printf("You are now chatting with: %s\n", current_chat_user);

        while (1) {
            char message[BUFFER_SIZE], full_message[BUFFER_SIZE+100];
            printf("You [%s]: ", current_chat_user);
            fgets(message, sizeof(message), stdin);
            message[strcspn(message, "\n")] = 0;

            if (strcmp(message, "/switch") == 0) {
                break;
            }

            snprintf(full_message, sizeof(full_message), "%s:%s\n", current_chat_user, message);
            send(client_socket, full_message, strlen(full_message), 0);
        }
    }

    close(client_socket);
    return 0;
}
