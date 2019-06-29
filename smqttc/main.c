

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "smqttc.h"
#include "protocol.h"

void parse_options(int argc, char** argv);

void print_usage_and_exit()
{
    fprintf(stderr, "Usage:\n\n");
    fprintf(stderr, "\tsmqttc [ -P <message> | -S ] <options> <host>[:<port>] <clientid> <topic>\n\n" );
    fprintf(stderr, "Common options:\n");
    fprintf(stderr, "\t-u <userid>\t(default none)\n");
    fprintf(stderr, "\t-p <password>\t(default none)\n");
    fprintf(stderr, "\t-k <keep alive seconds>\t(default 30)\n");
    fprintf(stderr, "\t-q [ 0 | 1 | 2 ]\t(qos: default 0))\n");
    fprintf(stderr, "\t-c\t(enable clean session -- default off)\n");
    fprintf(stderr, "\t-W [ 0 | 1 | 2 ] [ Y | N ] <topic> <message>\n");
    fprintf(stderr, "\t\tlast will qos -- 0 (default), 1 or 2\n");
    fprintf(stderr, "\t\tlast will retain -- Y or N (default)\n");
    fprintf(stderr, "\t\tlast will topic and message\n");
    exit(1);
}

#define ADVANCE_ARG(sw) { \
    arg++; \
    if (arg > argc) {\
        fprintf(stderr, "Missing argument to -%c switch\n", sw); \
        print_usage_and_exit(); \
    } \
}

int main (int argc, char** argv)
{
    if (argc > 1) {
        enum {PUBLISH, SUBSCRIBE} operation = PUBLISH;
        char *user = NULL;
        char *pw = NULL;
        uint32_t keep_alive_sec = 30;
        QoS qos = QoS0;
        bool clean_session = false;
        bool last_will = false;
        QoS last_will_qos = QoS0;
        bool last_will_retain = false;
        char *last_will_topic = NULL;
        char *last_will_message = NULL;
        char *publish_message = NULL;
        char *host = "127.0.0.1";
        uint16_t port = 1883u;
        char *client_id = NULL;
        char *topic = NULL;
	    int arg = 1;
	    // first find out whether publishing or subscribing
	    if (!strcmp(argv[arg], "-P")) {
	        operation = PUBLISH;
	        arg++;
	        if (arg >= argc) {
	            fprintf(stderr, "missing message to publish\n");
	            print_usage_and_exit();
	        }
	        publish_message = argv[arg];
	        arg++;
	    } else if (!strcmp(argv[arg], "-S")) {
	        operation = SUBSCRIBE;
	        arg++;
	    } else {
	        print_usage_and_exit();
	    }
	    // then get the options
	    while (arg < argc && argv[arg][0] == '-') {
            char sw = argv[arg][1];
            switch (sw) {
                case 'u': ADVANCE_ARG(sw)
                    user = argv[arg];
                    break;
                case 'p': ADVANCE_ARG(sw)
                    pw = argv[arg];
                    break;
                case 'k': ADVANCE_ARG(sw)
                    keep_alive_sec = strtoul(argv[arg], NULL, 10);
                    if (errno != ENOMSG) {
                        fprintf(stderr, "incorrect argument to -k\n");
                        print_usage_and_exit();
                    }
                    break;
                case 'q': ADVANCE_ARG(sw)
                    char *q = argv[arg];
                    switch (*q) {
                        case '0':
                            break;
                        case '1':
                            qos = QoS1;
                            break;
                        case '2':
                            qos = QoS2;
                            break;
                        default:
                            fprintf(stderr, "incorrect argument to -q\n");
                            print_usage_and_exit();
                    }
                case 'c':
                    clean_session = true;
                    break;
                case 'W':
                    last_will = true;
                    ADVANCE_ARG(sw)
                    char *wq = argv[arg];
                    switch (*wq) {
                        case '0':
                            break;
                        case '1':
                            last_will_qos = QoS1;
                            break;
                        case '2':
                            last_will_qos = QoS2;
                            break;
                        default:
                            fprintf(stderr, "incorrect QoS argument to -W\n");
                            print_usage_and_exit();
                    }
                    ADVANCE_ARG(sw)
                    char *wr = argv[arg];
                    switch (*wr) {
                        case 'Y':
                            last_will_retain = true;
                            break;
                        case 'N':
                            break;
                        default:
                            fprintf(stderr, "incorrect retain argument to -W\n");
                            print_usage_and_exit();
                    }
                    ADVANCE_ARG(sw)
                    last_will_topic = argv[arg];
                    ADVANCE_ARG(sw)
                    last_will_message = argv[arg];
                default:
                    fprintf(stderr, "Unrecognized switch %c\n", sw);
                    print_usage_and_exit();
            }
            arg++;
        }
        // then get positionals
        if (arg + 2 >= argc) {
            fprintf(stderr, "host[:port], client id and topic required after options\n");
            print_usage_and_exit();
        } else {
            char *host_port = argv[arg];
            char *pos = strstr(host_port, ":");
            if (pos == NULL) {
                host = host_port;
            } else {
                host = strchr(host_port, ':');
                port = strtoul(pos+1, NULL, 10);
            }
            client_id = argv[arg+1];
            topic = argv[arg+2];
        }

        // connect

        smqtt_client_t *client = NULL;
        smqtt_status_t stat =
                smqtt_connect(host, port,
                              client_id, user, pw,
                              keep_alive_sec, clean_session,
                              last_will, last_will_qos, last_will_retain,
                              last_will_topic, last_will_message,
                              &client);

        if (stat != SMQTT_OK) {
            fprintf(stderr, "Failed to connect\n");
            exit(1);
        }

        switch (operation) {
            case PUBLISH: {
                if (smqtt_publish(client, topic, publish_message, QoS1, false) != SMQTT_OK) {
                    fprintf(stderr, "publish failed\n");
                } else {
                    printf("Sent message\n");
                }

                smqtt_disconnect(client);
                break;
            }

            case SUBSCRIBE: {
                const char *topics[] = {topic, NULL};
                const QoS qoss[] = {qos};
                smqtt_status_t stat = smqtt_subscribe(client, topics, qoss);

                if (stat != SMQTT_OK) {
                    fprintf(stderr, "Failed to Subscribe\n");
                    exit(1);
                } else {
                    printf("Subscribed\n");
                }

                while (true) {
                    uint16_t packet_id = 0u; // so we can check it
                    char *message;
                    uint16_t message_size;
                    smqtt_status_t stat =
                            smqtt_get_next_message(client, &packet_id,
                                    &message, &message_size);
                    if (stat != SMQTT_OK) {
                        fprintf(stderr, "Failed to retrieve message\n");
                        exit(1);
                    } else {
                        fprintf(stdout, "Got message [%.*s]\n", message_size, message);
                    }
                }
                break;
            }

        }

    } else {
        print_usage_and_exit();
    }


    return 0;
}



