#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include "config/config.h"
#include "binance/binance.h"
#include "binance/pairs.h"
#include "server/server.h"
#include "storage/circular_buffer.h"

static int interrupted = 0;
static struct lws_context *context = NULL;

void sigint_handler(int sig) {
    (void)sig;
    interrupted = 1;
}

static struct lws_protocols protocols[] = {
    { "server-protocol", callback_server, sizeof(struct per_session_data__server), 4096, 0, NULL, 0 },
    { "binance-protocol", callback_binance, 0, 4096, 0, NULL, 0 },
    { NULL, NULL, 0, 0, 0, NULL, 0 }
};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);

    signal(SIGINT, sigint_handler);

    const char *config_file = "config.json";
    if (argc > 1) {
        config_file = argv[1];
    }

    if (load_config(config_file) != 0) {
        return 1;
    }

    // Fetch and update top pairs
    // if (update_config_with_top_pairs() != 0) {
    //     lwsl_err("Failed to fetch top pairs, using config.json defaults\n");
    // }

    init_storage();

    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    info.port = app_config.listen_port;

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("lws init failed\n");
        return 1;
    }

    fetch_historical_data(context);

    struct lws_client_connect_info cc_binance;
    memset(&cc_binance, 0, sizeof cc_binance);
    cc_binance.context = context;
    cc_binance.address = app_config.binance_host;
    cc_binance.port = 443;
    cc_binance.path = app_config.binance_path;
    cc_binance.host = cc_binance.address;
    cc_binance.origin = cc_binance.address;
    cc_binance.ssl_connection = LCCSCF_USE_SSL;
    cc_binance.protocol = "binance-protocol";
    cc_binance.pwsi = &wsi_binance;

    while (!interrupted) {
        if (!wsi_binance) {
            lws_client_connect_via_info(&cc_binance);
        }
        lws_service(context, 1000);
    }

    lws_context_destroy(context);
    free_storage();
    free_config();
    return 0;
}