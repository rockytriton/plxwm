#include "plxwm_server.h"
#include "plxwm_server_output.h"
#include "plxwm_keyboard.h"
#include "plxwm_cursor.h"
#include "plxwm_appwindow.h"
#include "plxwm_layerwindow.h"
#include "plxwm_popup.h"
#include "plxwm_wayland.h"

namespace PlxWM {


wlr_cursor *Server::getCursor() { 
	return cursor->getCursor(); 
}

wlr_output *Server::getActiveOutput() { return outputs[0]->getOutput(); }

wlr_surface *Server::getSurfaceAt(double lx, double ly, double *sx, double *sy) {
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

	return scene_surface->surface;
}

AppWindow *Server::getWindowAt(double lx, double ly, double *sx, double *sy) {
	wlr_scene_node *node = wlr_scene_node_at(&scene->tree.node, lx, ly, sx, sy);

	//printf("GETNODE: %p\n", node);

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

	if (tree == nullptr) {
		return nullptr;
	}
	
	return (AppWindow *)tree->node.data;
}

void Server::processResize() {

	AppWindow *wnd = grabbedWindow;

	double border_x = getCursor()->x - grab_x;
	double border_y = getCursor()->y - grab_y;
	int new_left = grab_geobox.x;
	int new_right = grab_geobox.x + grab_geobox.width;
	int new_top = grab_geobox.y;
	int new_bottom = grab_geobox.y + grab_geobox.height;

	if (resize_edges & WLR_EDGE_TOP) {
		new_top = border_y;
		if (new_top >= new_bottom) {
			new_top = new_bottom - 1;
		}
	} else if (resize_edges & WLR_EDGE_BOTTOM) {
		new_bottom = border_y;
		if (new_bottom <= new_top) {
			new_bottom = new_top + 1;
		}
	}
	if (resize_edges & WLR_EDGE_LEFT) {
		new_left = border_x;
		if (new_left >= new_right) {
			new_left = new_right - 1;
		}
	} else if (resize_edges & WLR_EDGE_RIGHT) {
		new_right = border_x;
		if (new_right <= new_left) {
			new_right = new_left + 1;
		}
	}

	struct wlr_box *geo_box = &wnd->getXdgTopLevel()->base->geometry;
	wlr_scene_node_set_position(&wnd->getSceneTree()->node,
		new_left - geo_box->x, new_top - geo_box->y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(wnd->getXdgTopLevel(), new_width, new_height);

}

void Server::focus(AppWindow *wnd) {
	if (wnd == NULL) {
		return;
	}

	wlr_surface *prev_surface = seat->keyboard_state.focused_surface;
	wlr_surface *surface = wnd->getSurface();

	if (prev_surface == surface) {
		return;
	}

	if (prev_surface) {
		wlr_xdg_toplevel *prev_toplevel = wlr_xdg_toplevel_try_from_wlr_surface(prev_surface);

		if (prev_toplevel != NULL) {
			wlr_xdg_toplevel_set_activated(prev_toplevel, false);
		}
	}

	wlr_keyboard *keyboard = wlr_seat_get_keyboard(seat);

	wlr_scene_node_raise_to_top(&wnd->getSceneTree()->node);

	wlr_xdg_toplevel_set_activated(wnd->getXdgTopLevel(), true);

	if (keyboard != NULL) {
		wlr_seat_keyboard_notify_enter(seat, surface,
			keyboard->keycodes, keyboard->num_keycodes, &keyboard->modifiers);
	}
	
}

void Server::onRequestCursor(wl_listener *listener, wlr_seat_pointer_request_set_cursor_event *event) {

	struct wlr_seat_client *focused_client = seat->pointer_state.focused_client;

	if (focused_client == event->seat_client) {
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

	// Set scale to 2.0 for 4K monitors, or 1.5 for 1440p
	wlr_output_state_set_scale(&state, 1.5);
	
	wlr_output_state_set_enabled(&state, true);

	/* Some backends don't have modes. DRM+KMS does, and we need to set a mode
	 * before we can use the output. The mode is a tuple of (width, height,
	 * refresh rate), and each monitor supports only a specific set of modes. We
	 * just pick the monitor's preferred mode, a more sophisticated compositor
	 * would let the user configure it. */

	//FILE *fp = fopen("./log.log", "wb");

	struct wlr_output_mode *mode = wlr_output_preferred_mode(output);

	if (mode != NULL) {
		//fprintf(fp, "Pref Mode: %d x %d\n", mode->width, mode->height);
		wlr_output_state_set_mode(&state, mode);
	} else {
		//fprintf(fp, "No pref mode\n");

		if (wl_list_empty(&output->modes)) {
			//fprintf(fp, "MODES EMPTY\n");
		}
	}

	wl_list_for_each(mode, &output->modes, link) {
		//fprintf(fp, "A MODE Mode: %d x %d\n", mode->width, mode->height);
	}

	//fclose(fp);


	/* Atomically applies the new output state. */
	wlr_output_commit_state(output, &state);
	wlr_output_state_finish(&state);

	/* Allocates and configures our state for this output */
    ServerOutput *srv_out = new ServerOutput(this, output);
	
	outputs.push_back(srv_out);

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

	if (!keyboards.empty()) {
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

	if (event->parent == nullptr) {
		printf("SKIPPING NULL PARENT\n");
		return;
	}

	Popup *popup = new Popup(this, event, nullptr, nullptr);
}

void Server::onSetSelection(struct wl_listener *listener, wlr_seat_request_set_selection_event *data) {
    printf("ON seat_request_set_selection\n");
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
	new_layer_surface.owner = this;
}

void Server::onNewLayerSurface(wl_listener *listener, wlr_layer_surface_v1 *surface) {
	printf("ON NEW LAYER SURFACE: %p - %p\n", surface, this);

	layers.push_back(new LayerWindow(this, surface));
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

	wl_signal_add(&backend->events.new_output, &new_output.listener);

	scene = wlr_scene_create();
	scene_layout = wlr_scene_attach_output_layout(scene, output_layout);

	xdg_shell = wlr_xdg_shell_create(display, 3);
	wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel.listener);
	wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup.listener);

    cursor = new Cursor(this);

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

	auto layer_shell = wlr_layer_shell_v1_create(display, 4);

	new_layer_surface.listener.notify = NOTIFIER(Server, wlr_layer_surface_v1, onNewLayerSurface);
	wl_signal_add(&layer_shell->events.new_surface,
			&new_layer_surface.listener);


	/* Add a Unix socket to the Wayland display. */
	socket = wl_display_add_socket_auto(display);
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

	registerServerProtocol(this);

	setenv("WAYLAND_DISPLAY", getSocket(), true);

	if (fork() == 0) {
		execl("/bin/sh", "/bin/sh", "-c", "/data2/git/plxwm/plxwm-panel", (void *)NULL);
	}

	wl_display_run(display);

    printf("OK DONE\n");
}

void Server::newKeyboard(wlr_input_device *device) {
    Keyboard *kb = new Keyboard(this, device);
	keyboards.push_back(kb);
}

void Server::arrangeLayers() {
    struct wlr_output *output = outputs[0]->getOutput(); // Assuming single monitor for now
    struct wlr_box full_area = {0};
    wlr_output_effective_resolution(output, &full_area.width, &full_area.height);
    
    struct wlr_box usable_area = full_area;

    for (auto *layer_win : layers) {
        auto *scene_surface = layer_win->getSceneSurface();
        auto *surface = layer_win->getSurface();

        wlr_scene_layer_surface_v1_configure(scene_surface, &full_area, &usable_area);

        wlr_layer_surface_v1_configure(surface, full_area.width, surface->pending.desired_height);

        int panel_height = surface->current.actual_height;

        if (panel_height == 0) panel_height = 40; // fallback if not yet committed

        wlr_scene_node_set_position(&scene_surface->tree->node, 0, full_area.height - panel_height);
    }
}


}
