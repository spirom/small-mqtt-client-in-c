

#include <malloc.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "smqttc.h"
#include "server.h"

static bool trace = false;

smqtt_status_t
network_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec);

smqtt_status_t
network_send(struct server_t *server, char *buffer, size_t length);

long
network_receive(struct server_t *server, char *buffer, size_t length);

smqtt_status_t
network_disconnect(struct server_t *server);

typedef struct server_impl_t {
    int                 socket;
    struct sockaddr_in  socket_address;
} server_impl_t;

int SetSocketTimeout(int connectSocket, int milliseconds)
{
    struct timeval tv;

    tv.tv_sec = milliseconds / 1000 ;
    tv.tv_usec = ( milliseconds % 1000) * 1000  ;

    return setsockopt (connectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof tv);
}

server_t *
create_network_server(void)
{
    server_t *server = (server_t *)malloc(sizeof(server_t));
    server->connect = &network_connect;
    server->send = &network_send;
    server->receive = &network_receive;
    server->disconnect = &network_disconnect;
    server->impl = (void *)malloc(sizeof(server_impl_t));
    return server;
}

smqtt_status_t
network_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec)
{
    server_impl_t *impl = (server_impl_t *)server->impl;
    if ((impl->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr,"failed to connect: could not create socket\n");
        free(impl);
        server->impl= NULL;
        return 1;
    }
    impl->socket_address.sin_family = AF_INET;
    impl->socket_address.sin_port = htons(port); // converts the unsigned short from host byte order to network byte order
    impl->socket_address.sin_addr.s_addr = inet_addr(hostname);

    // connect
    if ((connect(impl->socket, (struct sockaddr *)&impl->socket_address,
            sizeof(impl->socket_address))) < 0) {
        fprintf(stderr,"failed to connect: to server socket\n");
        free(impl);
        server->impl = NULL;
        return SMQTT_SOCKET;
    }

    if (timeout_msec != 0) {
        SetSocketTimeout(impl->socket, timeout_msec);
    }

    return SMQTT_OK;
}

smqtt_status_t
network_send(server_t *server, char *buffer, size_t length)
{
    server_impl_t *impl = (server_impl_t *)server->impl;
    if (trace) {
        fprintf(stderr, "send: fixed header %x %x\n", buffer[0] & 0xff, buffer[1] & 0xff);
        for (int i = 2; i < length; i++) {
            fprintf(stderr, "%2x ", buffer[i] & 0xff);
            if ((i == length - 1) || ((i - 1) % 10 == 0)) {
                fprintf(stderr, "\n");
            }
        }
    }
    if (send(impl->socket, buffer, length, 0) < length) {
        free(impl);
        server->impl = NULL;
        return SMQTT_SOCKET;
    }
    return SMQTT_OK;
}

long
network_receive(server_t *server, char *buffer, size_t length)
{
    server_impl_t *impl = (server_impl_t *)server->impl;

    long sz = recv(impl->socket, buffer, 2, 0);
    if (sz < 0) {
        printf("\nSocket recv returned %ld, errno %d %s\n",sz,errno, strerror(errno));
        close(impl->socket);
        free(impl);
        server->impl = NULL;
        return -1;
    }

    if (trace) {
        fprintf(stderr, "receive: packet start %x %x\n", buffer[0] & 0xff, buffer[1] & 0xff);
    }

    size_t remaining = buffer[1];
    size_t consumed = 2;

    if (remaining > 0) {
        sz = recv(impl->socket, buffer + 2, remaining, 0);
        if (sz < 0) {
            printf("\nSocket recv returned %ld, errno %d %s\n",sz,errno, strerror(errno));
            close(impl->socket);
            free(impl);
            server->impl = NULL;
            return -1;
        }
        long length = sz + consumed;
        if (trace) {
            for (int i = 2; i < length; i++) {
                fprintf(stderr, "%2x ", buffer[i] & 0xff);
                if ((i == length - 1) || ((i - 1) % 10 == 0)) {
                    fprintf(stderr, "\n");
                }
            }
        }
        return length;
    } else {
        // trivial packet
        return consumed; // hopefully 2
    }
}

smqtt_status_t
network_disconnect(server_t *server)
{
    server_impl_t *impl = (server_impl_t *)server->impl;

    close(impl->socket);
    free(impl);
    server->impl = NULL;
    return SMQTT_OK;
}
