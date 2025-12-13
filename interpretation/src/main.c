#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <cjson/cJSON.h>

static int interrupted = 0;
static struct lws_context *context;
static cJSON *global_state = NULL;

#define PORT_COUNT 5
static int ports[PORT_COUNT] = {6061, 6062, 6063, 6064, 6065};

typedef struct {
    int port;
    struct lws_context *ctx;
    struct lws_sorted_usec_list sul;
} client_t;

static client_t clients[PORT_COUNT];

static void sigint_handler(int sig) {
    (void)sig;
    interrupted = 1;
}

static void connect_client(struct lws_sorted_usec_list *sul);

static void schedule_reconnect(client_t *c, int us) {
    lws_sul_schedule(c->ctx, 0, &c->sul, connect_client, us);
}

static void connect_client(struct lws_sorted_usec_list *sul) {
    client_t *c = lws_container_of(sul, client_t, sul);
    struct lws_client_connect_info ccinfo = {0};
    
    ccinfo.context = c->ctx;
    ccinfo.address = "localhost";
    ccinfo.port = c->port;
    ccinfo.path = "/";
    ccinfo.host = "localhost";
    ccinfo.origin = "localhost";
    ccinfo.protocol = "server-protocol";
    ccinfo.pwsi = NULL;
    ccinfo.userdata = c;
    ccinfo.opaque_user_data = c;

    if (!lws_client_connect_via_info(&ccinfo)) {
        lwsl_err("Failed to initiate connection to port %d, retrying...\n", c->port);
        schedule_reconnect(c, 1000 * 1000);
    }
}

static void merge_json(cJSON *target, cJSON *source) {
    cJSON *symbol_node = NULL;
    cJSON_ArrayForEach(symbol_node, source) {
        char *symbol = symbol_node->string;
        cJSON *target_symbol = cJSON_GetObjectItem(target, symbol);
        if (!target_symbol) {
            target_symbol = cJSON_CreateObject();
            cJSON_AddItemToObject(target, symbol, target_symbol);
        }

        cJSON *tf_node = NULL;
        cJSON_ArrayForEach(tf_node, symbol_node) {
            char *tf = tf_node->string;
            cJSON_DeleteItemFromObject(target_symbol, tf);
            cJSON_AddItemToObject(target_symbol, tf, cJSON_Duplicate(tf_node, 1));
        }
    }
}

static int callback_interpretation(struct lws *wsi, enum lws_callback_reasons reason,
                                   void *user, void *in, size_t len) {
    client_t *c = NULL;
    
    if (user) {
        c = (client_t *)user;
    } else {
        c = (client_t *)lws_get_opaque_user_data(wsi);
    }

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            lwsl_user("Connected to port %d\n", c ? c->port : -1);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (in && len > 0) {
                char *msg = malloc(len + 1);
                memcpy(msg, in, len);
                msg[len] = '\0';

                cJSON *json = cJSON_Parse(msg);
                if (json) {
                    if (!global_state) global_state = cJSON_CreateObject();
                    merge_json(global_state, json);
                    
                    char *output = cJSON_PrintUnformatted(global_state);
                    if (output) {
                        printf("%s\n", output);
                        free(output);
                    }
                    cJSON_Delete(json);
                }
                free(msg);
            }
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            lwsl_err("Connection Error on port %d\n", c ? c->port : -1);
            if (c) schedule_reconnect(c, 1000 * 1000);
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:
            lwsl_user("Connection Closed on port %d\n", c ? c->port : -1);
            if (c) schedule_reconnect(c, 1000 * 1000);
            break;

        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "server-protocol",
        callback_interpretation,
        0,
        4096,
    },
    { NULL, NULL, 0, 0 }
};

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    signal(SIGINT, sigint_handler);

    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return 1;
    }

    // Initialize and connect clients
    for (int i = 0; i < PORT_COUNT; i++) {
        clients[i].port = ports[i];
        clients[i].ctx = context;
        schedule_reconnect(&clients[i], 100); 
    }

    while (!interrupted) {
        lws_service(context, 1000);
    }

    if (global_state) cJSON_Delete(global_state);
    lws_context_destroy(context);

    return 0;
}
