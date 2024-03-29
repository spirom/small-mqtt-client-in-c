

#ifndef SMQTTC_SERVER_H
#define SMQTTC_SERVER_H

#include <stddef.h>
#include <stdint.h>

#include "smqttc.h"

typedef enum {
    SMQTT_NET_OK,
    SMQTT_NET_SOCKET
} smqtt_net_status_t;



typedef enum {
    LOCAL_SERVER, REMOTE_SERVER
} server_mode_t;

typedef struct server_t {
    smqtt_net_status_t (*connect)(struct server_t *this,
            char *hostname, uint16_t port, uint16_t timeout_msec);
    smqtt_net_status_t (*send)(struct server_t *this, uint8_t *buffer, size_t length);
    long (*receive)(struct server_t *this, uint8_t *buffer, size_t length);
    smqtt_net_status_t (*disconnect)(struct server_t *this);
    int *impl;
} server_t;

server_t *
create_network_server(void);

smqtt_net_status_t
server_connect(server_t *server,
        char *hostname, uint16_t port, uint16_t timeout_msec);

smqtt_net_status_t server_send(server_t *server, uint8_t *buffer, size_t length);

long server_receive(server_t *server, uint8_t *buffer, size_t length);

smqtt_net_status_t server_disconnect(server_t *server);

#endif //SMQTTC_SERVER_H
