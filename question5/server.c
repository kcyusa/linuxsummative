/*
==========================================================
Chat Server
----------------------------------------------------------
Features:
- Accepts up to 4 clients concurrently using threads.
- Authenticates users based on username.
- Maintains and shares a list of online users.
- Handles "/list" requests from clients.
- Routes messages in the format "<target>:<message>".
- Synchronizes shared resources using mutex.
- Fixes socket race condition by allocating memory for each new client.
==========================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 4
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char username[50];
    int authenticated;
} Client;

Client clients[MAX_CLIENTS];                  // Shared list of clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect clients[] access

/* ==========================================================
   Function: send_online_users
   Purpose: Sends a list of currently online authenticated users
            to the requesting client.
========================================================== */
void send_online_users(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    char list[BUFFER_SIZE] = "Online Users:\n";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].authenticated) {
            strcat(list, clients[i].username);
            strcat(list, "\n");
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    send(client_socket, list, strlen(list), 0);
}

/* ==========================================================
   Function: find_client_socket
   Purpose: Finds the socket of a target user by username.
            Returns -1 if the user is not online.
========================================================== */
int find_client_socket(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].authenticated && strcmp(clients[i].username, username) == 0) {
            return clients[i].socket;
        }
    }
    return -1;
}

/* ==========================================================
   Thread: handle_client
   Purpose: Manages one client's authentication, commands, and
            message routing. Runs in its own thread.
========================================================== */
void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    free(arg); // Free dynamically allocated socket
    char buffer[BUFFER_SIZE];
    char username[50];

    // === AUTHENTICATION ===
    memset(username, 0, sizeof(username));
    recv(client_socket, username, sizeof(username), 0);
    username[strcspn(username, "\n")] = 0;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            strcpy(clients[i].username, username);
            clients[i].authenticated = 1;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    send(client_socket, "Authenticated\n", 14, 0);
    send_online_users(client_socket);

    // === MAIN CHAT LOOP ===
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break; // Client disconnected

        buffer[strcspn(buffer, "\n")] = 0;

        // Handle "/list" command
        if (strcmp(buffer, "/list") == 0) {
            send_online_users(client_socket);
            continue;
        }

        // Parse message "<target>:<message>"
        char target[50], message[BUFFER_SIZE];
        memset(target, 0, sizeof(target));
        memset(message, 0, sizeof(message));

        if (sscanf(buffer, "%49[^:]:%1023[^\n]", target, message) == 2) {
            int target_socket = find_client_socket(target);
            if (target_socket != -1) {
                char formatted_msg[BUFFER_SIZE + 100];
                size_t max_msg_len = sizeof(formatted_msg) - strlen("From : \n") - strlen(username) - 1;
                message[max_msg_len] = '\0';

                snprintf(formatted_msg, sizeof(formatted_msg), "From %s: %s\n", username, message);
                send(target_socket, formatted_msg, strlen(formatted_msg), 0);
            } else {
                char error_msg[] = "User offline or not found.\n";
                send(client_socket, error_msg, strlen(error_msg), 0);
            }
        } else {
            char error_msg[] = "Invalid format. Use <username>:<message>\n";
            send(client_socket, error_msg, strlen(error_msg), 0);
        }
    }

    // === HANDLE DISCONNECTION ===
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            clients[i].authenticated = 0;
            close(clients[i].socket);
            clients[i].socket = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

/* ==========================================================
   Function: main
   Purpose: Initializes the server, accepts incoming connections,
            and starts a new thread for each client.
========================================================== */
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, MAX_CLIENTS);
    printf("[Server] Listening on port 8080...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        clients[i].authenticated = 0;
    }

    // Accept clients in a loop
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        printf("[Server] New connection accepted.\n");

        pthread_mutex_lock(&clients_mutex);
        int assigned = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].socket == 0) {
                clients[i].socket = client_socket;
                clients[i].authenticated = 0;
                assigned = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (!assigned) {
            char msg[] = "Server full. Try again later.\n";
            send(client_socket, msg, strlen(msg), 0);
            close(client_socket);
        } else {
            int *new_sock = malloc(sizeof(int)); // Allocate new socket for the thread
            *new_sock = client_socket;
            pthread_t tid;
            pthread_create(&tid, NULL, handle_client, new_sock);
            pthread_detach(tid);
        }
    }

    close(server_socket);
    return 0;
}
