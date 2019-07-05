

#include <malloc.h>

#include "server.h"

smqtt_net_status_t
local_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec);

smqtt_net_status_t
local_send(struct server_t *server, uint8_t *buffer, size_t length);

long
local_receive(struct server_t *server, uint8_t *buffer, size_t length);

smqtt_net_status_t
local_disconnect(struct server_t *server);

typedef struct local_impl_t {

} local_impl_t;



server_t *
create_local_server(void)
{
    server_t *server = (server_t *)malloc(sizeof(server_t));
    server->connect = &local_connect;
    server->send = &local_send;
    server->receive = &local_receive;
    server->disconnect = &local_disconnect;
    server->impl = (void *)malloc(sizeof(local_impl_t));
    return server;
}

smqtt_net_status_t
local_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec)
{
    local_impl_t *impl = (local_impl_t *)server->impl;


    return SMQTT_OK;
}

smqtt_net_status_t
local_send(server_t *server, uint8_t *buffer, size_t length)
{
    local_impl_t *impl = (local_impl_t *)server->impl;

    return SMQTT_OK;
}

long
local_receive(server_t *server, uint8_t *buffer, size_t length)
{
    return SMQTT_OK;
}

smqtt_net_status_t
local_disconnect(server_t *server)
{
    local_impl_t *impl = (local_impl_t *)server->impl;


    return SMQTT_OK;
}

