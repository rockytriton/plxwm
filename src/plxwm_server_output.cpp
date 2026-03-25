#include "plxwm_server_output.h"
#include "plxwm_server.h"

namespace PlxWM {

ServerOutput::ServerOutput(Server *server, wlr_output *output) {
    this->server = server;
    this->output = output;

	destroy = make_unique<Signal<&ServerOutput::onDestroy>>(this, &output->events.destroy);
	requestState = make_unique<Signal<&ServerOutput::onRequestState>>(this, &output->events.request_state);
	frame = make_unique<Signal<&ServerOutput::onFrame>>(this, &output->events.frame);
}

void ServerOutput::onFrame(wl_listener *listener, void *data) {
    //printf("onFrame\n");

	/* This function is called every time an output is ready to display a frame,
	 * generally at the output's refresh rate (e.g. 60Hz). */
	wlr_scene *scene = server->getScene();

	struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
		scene, this->output);

	// Render the scene if needed and commit the output 
	wlr_scene_output_commit(scene_output, NULL);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(scene_output, &now); 
}

void ServerOutput::onRequestState(wl_listener *listener, void *data) {
	const wlr_output_event_request_state *event = (wlr_output_event_request_state *)data;
	wlr_output_commit_state(output, event->state);
}

void ServerOutput::onDestroy(wl_listener *listener, void *data) {
	frame->cleanup();
	requestState->cleanup();
	destroy->cleanup();
}


};
