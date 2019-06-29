
#include <stdint.h>

#include "smqttc.h"
#include "server.h"

smqtt_status_t server_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec)
{
    return server->connect(server, hostname, port, timeout_msec);
}

smqtt_status_t server_send(server_t *server, char *buffer, size_t length)
{
    return server->send(server, buffer, length);
}

long server_receive(server_t *server, char *buffer, size_t length)
{
    return server->receive(server, buffer, length);
}

smqtt_status_t server_disconnect(server_t *server)
{
    return server->disconnect(server);
}
