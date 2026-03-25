#pragma once

#include "common.h"
#include "plxwm_signal.h"

namespace PlxWM {
class Server;

class ServerOutput {
public:
    ServerOutput() {}
    ServerOutput(Server *server, wlr_output *output);

    void onFrame(wl_listener *listener, void *data);
    void onRequestState(wl_listener *listener, void *data);
    void onDestroy(wl_listener *listener, void *data);

    wlr_output *getOutput() { return output; }

private:
	Server *server;
	wlr_output *output;

    unique_ptr<Signal<&ServerOutput::onDestroy>> destroy;
    unique_ptr<Signal<&ServerOutput::onFrame>> frame;
    unique_ptr<Signal<&ServerOutput::onRequestState>> requestState;

};
};

