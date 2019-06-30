#include "messages.h"
#include "messages_internal.h"
#include "smqttc_mt_internal.h"
#include "smqttc_mt.h"
#include "server.h"

#include "test_macros.h"

test_result_t
test_connect_disconnect_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                   "test_client_1", "user1", "password1",30u, true,
                                   false, QoS0, false, NULL, NULL,
                                   &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    return result;
}

test_result_t
test_ping_v3()
{
    test_result_t result;
    init_result(&result);

    smqtt_mt_client_t *client1 = NULL;
    smqtt_mt_status_t status =
            smqtt_mt_connect_internal(REMOTE_SERVER, "127.0.0.1", 1883,
                                      "test_client_1", "user1", "password1",30u, true,
                                      false, QoS0, false, NULL, NULL,
                                      &client1);

    ASSERT_TRUE(status == SMQTT_MT_OK, result)
    ASSERT_TRUE(client1 != NULL, result)

    status = smqtt_mt_ping(client1, 500);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    status = smqtt_mt_disconnect(client1);
    ASSERT_TRUE(status == SMQTT_MT_OK, result)

    return result;
}