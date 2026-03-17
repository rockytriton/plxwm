#include "plxwm_server.h"

using PlxWM::Server;

int main(int argc, char *argv[]) {
	//wlr_log_init(WLR_DEBUG, NULL);

    Server srv;
    srv.init();

    printf("DONE MAIN\n");
}
