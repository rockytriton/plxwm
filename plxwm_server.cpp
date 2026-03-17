#include "plxwm_server.h"
#include "plxwm_server_output.h"
#include "plxwm_keyboard.h"
#include "plxwm_cursor.h"

namespace PlxWM {



static void seat_request_cursor(struct wl_listener *listener, void *data) {
    printf("ON seat_request_cursor\n");

}

void Server::onNewOutput(wl_listener *listener, wlr_output *output) {
    printf("ON onNewOutput\n");

	/* Configures the output created by the backend to use our allocator
	 * and our renderer. Must be done once, before commiting the output */
	wlr_output_init_render(output, allocator, renderer);

	/* The output may be disabled, switch it on. */
	wlr_output_state state;
	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	/* Some backends don't have modes. DRM+KMS does, and we need to set a mode
	 * before we can use the output. The mode is a tuple of (width, height,
	 * refresh rate), and each monitor supports only a specific set of modes. We
	 * just pick the monitor's preferred mode, a more sophisticated compositor
	 * would let the user configure it. */
	struct wlr_output_mode *mode = wlr_output_preferred_mode(output);

	if (mode != NULL) {
		wlr_output_state_set_mode(&state, mode);
	}

	/* Atomically applies the new output state. */
	wlr_output_commit_state(output, &state);
	wlr_output_state_finish(&state);

	/* Allocates and configures our state for this output */
    ServerOutput *srv_out = new ServerOutput(this, output);
    srv_out->init();

	/* Adds this to the output layout. The add_auto function arranges outputs
	 * from left-to-right in the order they appear. A more sophisticated
	 * compositor would let the user configure the arrangement of outputs in the
	 * layout.
	 *
	 * The output layout utility automatically adds a wl_output global to the
	 * display, which Wayland clients can see to find out information about the
	 * output (such as DPI, scale factor, manufacturer, etc).
	 */
	wlr_output_layout_output *l_output = wlr_output_layout_add_auto(output_layout, output);
	wlr_scene_output *scene_output = wlr_scene_output_create(scene, output);
	wlr_scene_output_layout_add_output(scene_layout, l_output, scene_output);
}

void Server::onNewInput(wl_listener *listener, wlr_input_device *device) {
    printf("ON onNewInput\n");

	/* This event is raised by the backend when a new input device becomes
	 * available. */
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		newKeyboard(device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
        /* We don't do anything special with pointers. All of our pointer handling
        * is proxied through wlr_cursor. On another compositor, you might take this
        * opportunity to do libinput configuration on the device to set
        * acceleration, etc. */
        
        wlr_cursor_attach_input_device(cursor->getCursor(), device);
		break;
	default:
		break;
	}

	/* We need to let the wlr_seat know what our capabilities are, which is
	 * communiciated to the client. In TinyWL we always have a cursor, even if
	 * there are no pointer devices, so we always include that capability. */
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;

	if (!wl_list_empty(&keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}

	wlr_seat_set_capabilities(seat, caps);
}

void Server::server_new_output(struct wl_listener *listener, void *data) {
    
    Server *server = ((Listener<Server> *)listener)->owner;
	wlr_output *output = (wlr_output *)data;

    server->onNewOutput(&((Listener<Server> *)listener)->listener, output);
}

void Server::server_new_input(struct wl_listener *listener, void *data) {
    Server *server = ((Listener<Server> *)listener)->owner;
    server->onNewInput(&((Listener<Server> *)listener)->listener, (wlr_input_device *)data);
}

static void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
    printf("ON server_new_xdg_toplevel\n");

}

static void server_new_xdg_popup(struct wl_listener *listener, void *data) {
    printf("ON server_new_xdg_popup\n");

}

static void seat_request_set_selection(struct wl_listener *listener, void *data) {
    printf("ON seat_request_set_selection\n");

}

Server::Server() {
    new_output.owner = this;
	new_output.listener.notify = server_new_output;

    new_input.owner = this;

    new_xdg_toplevel.owner = this;
    new_xdg_popup.owner = this;

	new_xdg_toplevel.listener.notify = server_new_xdg_toplevel;
	new_xdg_popup.listener.notify = server_new_xdg_popup;
}

void Server::init() {
    display = wl_display_create();
    backend = wlr_backend_autocreate(wl_display_get_event_loop(display), NULL);
    
    if (backend == NULL) {
		wlr_log(WLR_ERROR, "failed to create wlr_backend");
		return;
	}

    renderer = wlr_renderer_autocreate(backend);
	if (renderer == NULL) {
		wlr_log(WLR_ERROR, "failed to create wlr_renderer");
		return;
	}

	wlr_renderer_init_wl_display(renderer, display);

    allocator = wlr_allocator_autocreate(backend, renderer);

	if (allocator == NULL) {
		wlr_log(WLR_ERROR, "failed to create wlr_allocator");
		return;
	}

	wlr_compositor_create(display, 5, renderer);
	wlr_subcompositor_create(display);
	wlr_data_device_manager_create(display);

    output_layout = wlr_output_layout_create(display);

	wl_list_init(&outputs);

	wl_signal_add(&backend->events.new_output, &new_output.listener);

	scene = wlr_scene_create();
	scene_layout = wlr_scene_attach_output_layout(scene, output_layout);

	wl_list_init(&toplevels);
	xdg_shell = wlr_xdg_shell_create(display, 3);
	wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel.listener);
	wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup.listener);

    cursor = new Cursor(this);
    cursor->init();

	/*
	 * Configures a seat, which is a single "seat" at which a user sits and
	 * operates the computer. This conceptually includes up to one keyboard,
	 * pointer, touch, and drawing tablet device. We also rig up a listener to
	 * let us know when new input devices are available on the backend.
	 */
	wl_list_init(&keyboards);

	new_input.listener.notify = server_new_input;
	wl_signal_add(&backend->events.new_input, &new_input.listener);

	seat = wlr_seat_create(display, "seat0");
	request_cursor.listener.notify = seat_request_cursor;
	wl_signal_add(&seat->events.request_set_cursor,
			&request_cursor.listener);
	request_set_selection.listener.notify = seat_request_set_selection;
	wl_signal_add(&seat->events.request_set_selection,
			&request_set_selection.listener);

	/* Add a Unix socket to the Wayland display. */
	const char *socket = wl_display_add_socket_auto(display);
	if (!socket) {
		wlr_backend_destroy(backend);
		return;
	}

	/* Start the backend. This will enumerate outputs and inputs, become the DRM
	 * master, etc */
	if (!wlr_backend_start(backend)) {
		wlr_backend_destroy(backend);
		wl_display_destroy(display);
		return;
	}

	wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s",
			socket);
	wl_display_run(display);

    printf("OK DONE\n");
}

void Server::newKeyboard(wlr_input_device *device) {
    Keyboard *kb = new Keyboard(this, device);
    kb->init();
}

};
