#include "plxwm_server_output.h"
#include "plxwm_server.h"

namespace PlxWM {



ServerOutput::ServerOutput(Server *server, wlr_output *output) {
    this->server = server;
    this->output = output;

	frame.owner = this;
	request_state.owner = this;
	destroy.owner = this;
}

void ServerOutput::init() {
    
	/* Sets up a listener for the frame event. */
	frame.listener.notify = NOTIFIER(ServerOutput, void, onFrame);
	wl_signal_add(&output->events.frame, &frame.listener);

	/* Sets up a listener for the state request event. */
	request_state.listener.notify = NOTIFIER(ServerOutput, void, onRequestState);
	wl_signal_add(&output->events.request_state, &request_state.listener);

	/* Sets up a listener for the destroy event. */
	destroy.listener.notify = NOTIFIER(ServerOutput, void, onDestroy);
	wl_signal_add(&output->events.destroy, &destroy.listener);

	wl_list_insert(server->getOutputs(), &link);

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
    printf("onRequestState\n");

	/* This function is called when the backend requests a new state for
	 * the output. For example, Wayland and X11 backends request a new mode
	 * when the output window is resized. */
	const wlr_output_event_request_state *event = (wlr_output_event_request_state *)data;
	wlr_output_commit_state(output, event->state);
}

void ServerOutput::onDestroy(wl_listener *listener, void *data) {
    printf("onDestroy\n");
	wl_list_remove(&frame.listener.link);
	wl_list_remove(&request_state.listener.link);
	wl_list_remove(&destroy.listener.link);
	wl_list_remove(&link);

	//free(output);
}


};
