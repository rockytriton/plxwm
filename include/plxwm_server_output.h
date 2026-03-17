#pragma once

#include "common.h"

namespace PlxWM {
class Server;

class ServerOutput {
public:
    ServerOutput() {}
    ServerOutput(Server *server, wlr_output *output);
    void init();

    void onFrame(wl_listener *listener, void *data);
    void onRequestState(wl_listener *listener, void *data);
    void onDestroy(wl_listener *listener, void *data);

    static void output_frame(struct wl_listener *listener, void *data);

    static void output_request_state(struct wl_listener *listener, void *data);

    static void output_destroy(struct wl_listener *listener, void *data);

private:
	wl_list link;
	Server *server;
	wlr_output *output;
	Listener<ServerOutput> frame;
	Listener<ServerOutput> request_state;
	Listener<ServerOutput> destroy;
};
};

