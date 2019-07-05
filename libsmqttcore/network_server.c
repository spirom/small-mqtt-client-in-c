

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

static bool trace = true;

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
    fd_set              read_fds;
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
    if (trace) {
        fprintf(stderr, "net: trying to connect\n");
    }
    server_impl_t *impl = (server_impl_t *)server->impl;
    if ((impl->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr,"net: could not create socket\n");
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
        fprintf(stderr,"net: failed to connect to server socket\n");
        free(impl);
        server->impl = NULL;
        return SMQTT_SOCKET;
    }

    if (timeout_msec != 0) {
        if (trace) {
            fprintf(stderr, "net: setting a socket timeout\n");
        }
        SetSocketTimeout(impl->socket, timeout_msec);
    }

    return SMQTT_OK;
}

smqtt_status_t
network_send(server_t *server, char *buffer, size_t length)
{
    server_impl_t *impl = (server_impl_t *)server->impl;
    if (trace) {
        fprintf(stderr, "net: send <<%d>> length=%lu: fixed header %x %x\n",
                impl->socket, length,
                buffer[0] & 0xff, buffer[1] & 0xff);
        for (int i = 2; i < length; i++) {
            fprintf(stderr, "%2x ", buffer[i] & 0xff);
            if ((i == length - 1) || ((i - 1) % 10 == 0)) {
                fprintf(stderr, "\n");
            }
        }
    }
    if (send(impl->socket, buffer, length, 0) < length) {
        if (trace) {
            fprintf(stderr, "net: send failed\n");
        }
        free(impl);
        server->impl = NULL;
        return SMQTT_SOCKET;
    }
    if (trace) {
        fprintf(stderr, "net: send succeeded\n");
    }
    return SMQTT_OK;
}

long
network_receive(server_t *server, char *buffer, size_t length)
{
    server_impl_t *impl = (server_impl_t *)server->impl;

    struct timeval tv;
    fprintf(stderr, "cli: sleeping 1\n");
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    FD_ZERO(&impl->read_fds);
    FD_SET(impl->socket, &impl->read_fds);
    if (trace) {
        fprintf(stderr, "net: about to select\n");
    }
    int ret = select(impl->socket+1, &impl->read_fds, NULL, NULL, &tv);
    if (ret < 0) {
        fprintf(stderr, "net: select failed\n");
        return 0;
    } else {
        if (trace) {
            fprintf(stderr, "net: out of select\n");
        }
    }

    if (trace) {
        fprintf(stderr, "net: Receiving header <<%d>>\n", impl->socket);
    }
    long sz = recv(impl->socket, buffer, 2, 0);
    if (sz < 0) {
#if 0
        printf("\nSocket recv returned %ld, errno %d %s\n",sz,errno, strerror(errno));
        close(impl->socket);
        free(impl);
        server->impl = NULL;
        return -1;
#else
        return 0;
#endif
    }

    if (trace) {
        fprintf(stderr, "net: received header: packet start %x %x\n", buffer[0] & 0xff, buffer[1] & 0xff);
    }

    size_t remaining = buffer[1];
    size_t consumed = 2;

    if (remaining > 0) {
        if (trace) {
            fprintf(stderr, "net: Receiving body <<%d>>\n", impl->socket);
        }
        sz = recv(impl->socket, buffer + 2, remaining, 0);
        if (sz < 0) {
#if 0
            printf("\nSocket recv returned %ld, errno %d %s\n",sz,errno, strerror(errno));
            close(impl->socket);
            free(impl);
            server->impl = NULL;
            return -1;
#else
            return 0;
#endif
        }
        long length = sz + consumed;
        if (trace) {
            fprintf(stderr, "net: Received:\n");
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
        if (trace) {
            fprintf(stderr, "net: (no body expected)\n");
        }
        return consumed; // hopefully 2
    }
}

smqtt_status_t
network_disconnect(server_t *server)
{
    server_impl_t *impl = (server_impl_t *)server->impl;
    if (trace) {
        fprintf(stderr, "net: disconnected\n");
    }
    close(impl->socket);
    free(impl);
    server->impl = NULL;
    return SMQTT_OK;
}

