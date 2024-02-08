#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define REMOTE_BUFFER_SIZE 65536
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// Helper functions
int hex_to_dec(char c);
void url_decode(char *dest, const char *src);
int need_proxy(const char *file_name);
static void server_failure_response(int client_socket);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    char *request_copy = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);
    strcpy(request_copy, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    
    // get the first line of http request
    char *first_line = strtok(request, "\r\n");
    char *method = strtok(first_line, " ");
    char *path = strtok(NULL, " ");

    char decoded_path[1024];
    url_decode(decoded_path, path);
    char *file_name = "index.html";
    if (strcmp(path, "/") != 0){
        file_name = decoded_path;
    }

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    if (need_proxy(file_name) == 0) {
        proxy_remote_file(app, client_socket, request_copy);
    } else {
        serve_local_file(client_socket, file_name);
    }
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

//    char response[] = "HTTP/1.0 200 OK\r\n"
//                      "Content-Type: text/plain; charset=UTF-8\r\n"
//                      "Content-Length: 15\r\n"
//                      "\r\n"
//                      "Sample response";
    
    if (path[0] == '/') path++;  // on linux file system

    FILE *file = fopen(path, "r");

    if(file == NULL){
        const char *not_found_response = "HTTP/1.0 404 Not Found\r\n"
                                         "Content-Type: text/plain; charset=UTF-8\r\n"
                                         "Content-Length: 13\r\n"
                                         "\r\n"
                                         "404 Not Found";
        send(client_socket, not_found_response, strlen(not_found_response), 0);
        return;
    }

    // get Content-Type
    char *content_type;
    if (strstr(path, ".html") || strstr(path, ".mime")){
        content_type = "text/html; charset=UTF-8";
    } else if (strstr(path, ".txt")){
        content_type = "text/plain";
    }else if (strstr(path, ".jpg")){
        content_type = "image/jpeg";
    } else{
        content_type = "application/octet-stream";
    }

    //Extract file size and content from file
    struct stat st;
    stat(path, &st);
    char *file_content = malloc(st.st_size);
    fread(file_content, 1, st.st_size, file);

    //Generate and send Header
    char header[512];
    sprintf(header, "HTTP/1.0 200 OK\r\n"
                    "Content-Type: %s\r\n"
                    "Content-Length: %lld\r\n"
                    "\r\n", content_type, st.st_size);
    send(client_socket, header, strlen(header), 0);

    //send file content
    send(client_socket, file_content, st.st_size, 0);
    fclose(file);
    free(file_content);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    
    int remote_socket = socket(AF_INET, SOCK_STREAM, 0); 
  
    struct sockaddr_in remoteServer_addr; 
  
    remoteServer_addr.sin_family = AF_INET; 
    remoteServer_addr.sin_port = htons(app->remote_port);
    remoteServer_addr.sin_addr.s_addr = inet_addr(app->remote_host);; 
  
    if (connect(remote_socket, 
                (struct sockaddr*)&remoteServer_addr, 
                sizeof(remoteServer_addr)) != 0){
        perror("Error connecting to the proxy..\n");
        close(remote_socket);
        server_failure_response(client_socket);
        return;
    }  

    // Forward request to the video server
    if (send(remote_socket, request, strlen(request), 0) == -1){
        perror("send to remote server failed");
        server_failure_response(client_socket);
    }

    char remote_buffer[REMOTE_BUFFER_SIZE];
    
    while (1){
        ssize_t remote_bytes_received = recv(remote_socket, remote_buffer, sizeof(remote_buffer), 0);
        if (remote_bytes_received == -1){
            perror("recv() from video server failed\n");
            server_failure_response(client_socket);
            break;
        }
        if (remote_bytes_received == 0){
            break;
        }
        
        // Forward response to my client (browser)
        if (send(client_socket, remote_buffer, remote_bytes_received, 0) == -1){
            perror("write failed");
            break;
        }
    }

    close(remote_socket);    
}

int hex_to_dec(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

// Function to perform URL decoding
void url_decode(char *dest, const char *src) {
    char *d = dest;
    while (*src) {
        if ((*src == '%' && src[1] == '2' && src[2] == '0') || (*src == '%' && src[1] == '2' && src[2] == '5')) {
            *d++ = hex_to_dec(src[1]) * 16 + hex_to_dec(src[2]);
            src += 3;
        }
        else {
            *d++ = *src++;
        }
    }
    *d = '\0';
}

int need_proxy(const char *file_name){
    int len = strlen(file_name);
    const char *file_extension = &file_name[len-3];
    return strcmp(file_extension, ".ts");
}

static void server_failure_response(int client_socket){
    char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
    if (send(client_socket, response, strlen(response), 0) == -1){
        perror("send in server_failure_response() failed");
    }
}