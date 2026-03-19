#include "plxwm_server.h"
#include "plxwm_server_output.h"
#include "plxwm_keyboard.h"
#include "plxwm_cursor.h"
#include "plxwm_appwindow.h"

namespace PlxWM {


AppWindow *Server::getWindowAt(double lx, double ly, double *sx, double *sy) {
	wlr_scene_node *node = wlr_scene_node_at(&scene->tree.node, lx, ly, sx, sy);

	if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER) {
		return NULL;
	}

	struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
	struct wlr_scene_surface *scene_surface =
		wlr_scene_surface_try_from_buffer(scene_buffer);
	if (!scene_surface) {
		return NULL;
	}

	struct wlr_scene_tree *tree = node->parent;
	while (tree != NULL && tree->node.data == NULL) {
		tree = tree->node.parent;
	}

	return (AppWindow *)tree->node.data;
}

void Server::focus(AppWindow *wnd) {
	if (wnd == NULL) {
		return;
	}

	wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	wlr_surface *surface = wnd->getSurface();

	if (prev_surface == surface) {
		/* Don't re-focus an already focused surface. */
		return;
	}

	if (prev_surface) {
		/*
		 * Deactivate the previously focused surface. This lets the client know
		 * it no longer has focus and the client will repaint accordingly, e.g.
		 * stop displaying a caret.
		 */
		wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);

		if (prev_toplevel != NULL) {
			wlr_xdg_toplevel_set_activated(prev_toplevel, false);
		}
	}

	wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

	/* Move the toplevel to the front */
	wlr_scene_node_raise_to_top(&wnd->getSceneTree()->node);

	wl_list_remove(wnd->getLink());
	wl_list_insert(&appWindows, wnd->getLink());

	/* Activate the new surface */
	wlr_xdg_toplevel_set_activated(wnd->getXdgTopLevel(), true);


	/*
	 * Tell the seat to have the keyboard enter this surface. wlroots will keep
	 * track of this and automatically send key events to the appropriate
	 * clients without additional work on your part.
	 */
	if (keyboard != NULL) {
		wlr_seat_keyboard_notify_enter(seat, surface,
			keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
	}
	printf("Focus set (%p to %p)\n", prev_surface, surface);
}

void Server::onRequestCursor(wl_listener *listener, wlr_seat_pointer_request_set_cursor_event *event) {

	/* This event is raised by the seat when a client provides a cursor image */
	struct wlr_seat_client *focused_client = seat->pointer_state.focused_client;

	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. */
	if (focused_client == event->seat_client) {
		/* Once we've vetted the client, we can tell the cursor to use the
		 * provided surface as the cursor image. It will set the hardware cursor
		 * on the output that it's currently on and continue to do so as the
		 * cursor moves between outputs. */
		wlr_cursor_set_surface(cursor->getCursor(), event->surface,
				event->hotspot_x, event->hotspot_y);
	}
}

void Server::onNewOutput(wl_listener *listener, wlr_output *output) {
	printf("onNewOutput\n");
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

void Server::onNewAppWindow(wl_listener *listener, wlr_xdg_toplevel *xdg_toplevel) {

    printf("ON onNewAppWindow\n");

	AppWindow *aw = new AppWindow(this, xdg_toplevel);
	
	printf("NEW WND: %p\n", aw);
}

void Server::onNewPopup(wl_listener *listener, wlr_xdg_popup *event) {
    printf("ON server_new_xdg_popup\n");

/*
	// * This event is raised when a client creates a new popup. 
	struct wlr_xdg_popup *xdg_popup = data;

	struct tinywl_popup *popup = calloc(1, sizeof(*popup));
	popup->xdg_popup = xdg_popup;

	// We must add xdg popups to the scene graph so they get rendered. The
	//// * wlroots scene graph provides a helper for this, but to use it we must
	// * provide the proper parent scene node of the xdg popup. To enable this,
	// * we always set the user data field of xdg_surfaces to the corresponding
	// * scene node. 
	struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
	assert(parent != NULL);
	struct wlr_scene_tree *parent_tree = parent->data;
	xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

	popup->commit.notify = xdg_popup_commit;
	wl_signal_add(&xdg_popup->base->surface->events.commit, &popup->commit);

	popup->destroy.notify = xdg_popup_destroy;
	wl_signal_add(&xdg_popup->events.destroy, &popup->destroy);
*/
}

void Server::onSetSelection(struct wl_listener *listener, wlr_seat_request_set_selection_event *data) {
    printf("ON seat_request_set_selection\n");

	/* This event is raised by the seat when a client wants to set the selection,
	 * usually when the user copies something. wlroots allows compositors to
	 * ignore such requests if they so choose, but in tinywl we always honor
	 
	struct tinywl_server *server = wl_container_of(
			listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event *event = data;
	wlr_seat_set_selection(server->seat, event->source, event->serial); */
}

Server::Server() {
    new_output.owner = this;

	new_output.listener.notify = NOTIFIER(Server, wlr_output, onNewOutput);

    new_input.owner = this;

    new_xdg_toplevel.owner = this;
    new_xdg_popup.owner = this;

	new_xdg_toplevel.listener.notify = NOTIFIER(Server, wlr_xdg_toplevel, onNewAppWindow);
	new_xdg_popup.listener.notify = NOTIFIER(Server, wlr_xdg_popup, onNewPopup);




	request_cursor.owner = this;
	request_set_selection.owner = this;
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

	wl_list_init(&appWindows);
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

	new_input.listener.notify = NOTIFIER(Server, wlr_input_device, onNewInput);
	wl_signal_add(&backend->events.new_input, &new_input.listener);
	
	seat = wlr_seat_create(display, "seat0");

	printf("SET SEAT: %p to %p\n", this, seat);

	request_cursor.listener.notify = NOTIFIER(Server, wlr_seat_pointer_request_set_cursor_event, onRequestCursor);

	wl_signal_add(&seat->events.request_set_cursor,
			&request_cursor.listener);
	request_set_selection.listener.notify = NOTIFIER(Server, wlr_seat_request_set_selection_event, onSetSelection);
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
