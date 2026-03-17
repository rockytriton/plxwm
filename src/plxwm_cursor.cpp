#include "plxwm_cursor.h"
#include "plxwm_server.h"

namespace PlxWM {

enum tinywl_cursor_mode {
	TINYWL_CURSOR_PASSTHROUGH,
	TINYWL_CURSOR_MOVE,
	TINYWL_CURSOR_RESIZE,
};

tinywl_cursor_mode cursor_mode;


void Cursor::onMove(wlr_pointer_motion_event *event) {

    wlr_cursor_move(cursor, &event->pointer->base,
			event->delta_x, event->delta_y);

    uint32_t time = event->time_msec;

	/* If the mode is non-passthrough, delegate to those functions. */
	if (cursor_mode == TINYWL_CURSOR_MOVE) {
		
        struct tinywl_toplevel *toplevel = server->getTopLevel();

        wlr_scene_node_set_position(&toplevel->scene_tree->node,
            cursor->x - grab_x,
            cursor->y - grab_y);

		return;
	} else if (cursor_mode == TINYWL_CURSOR_RESIZE) {
		//TODO process_cursor_resize(server);
		return;
	}

	/* Otherwise, find the toplevel under the pointer and send the event along. */
	double sx, sy;
	struct wlr_seat *seat = server->getSeat();
	struct wlr_surface *surface = NULL;
	
    //TODO: struct tinywl_toplevel *toplevel = desktop_toplevel_at(server,
		//	server->cursor->x, server->cursor->y, &surface, &sx, &sy);

	//if (!toplevel) {
		/* If there's no toplevel under the cursor, set the cursor image to a
		 * default. This is what makes the cursor image appear when you move it
		 * around the screen, not over any toplevels. */
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
	//}

	if (surface) {
		/*
		 * Send pointer enter and motion events.
		 *
		 * The enter event gives the surface "pointer focus", which is distinct
		 * from keyboard focus. You get pointer focus by moving the pointer over
		 * a window.
		 *
		 * Note that wlroots will avoid sending duplicate enter/motion events if
		 * the surface has already has pointer focus or if the client is already
		 * aware of the coordinates passed.
		 */
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		/* Clear pointer focus so future button events and such are not sent to
		 * the last client to have the cursor over it. */
		wlr_seat_pointer_clear_focus(seat);
	}
}


void Cursor::server_cursor_motion(struct wl_listener *listener, void *data) {
    printf("ON server_cursor_motion\n");
	/* This event is forwarded by the cursor when a pointer emits a _relative_
	 * pointer motion event (i.e. a delta) */
    Cursor *kb = ((Listener<Cursor> *)listener)->owner;
    
	struct wlr_pointer_motion_event *event = (wlr_pointer_motion_event *)data;

    kb->onMove(event);

	/* The cursor doesn't move unless we tell it to. The cursor automatically
	 * handles constraining the motion to the output layout, as well as any
	 * special configuration applied for the specific input device which
	 * generated the event. You can pass NULL for the device if you want to move
	 * the cursor around without any input. */
	

	//process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(
		struct wl_listener *listener, void *data) {
    printf("ON server_cursor_motion_absolute\n");

}

static void server_cursor_button(struct wl_listener *listener, void *data) {
    printf("ON server_cursor_button\n");

}

static void server_cursor_axis(struct wl_listener *listener, void *data) {
    printf("ON server_cursor_axis\n");

}

void Cursor::server_cursor_frame(struct wl_listener *listener, void *data) {
    printf("ON server_cursor_frame\n");

    Cursor *kb = ((Listener<Cursor> *)listener)->owner;
	wlr_seat_pointer_notify_frame(kb->server->getSeat());
    //kb->onKey(&((Listener<Cursor> *)listener)->listener, (wlr_keyboard_key_event *)data);
}

Cursor::Cursor(Server *server) {
    this->server = server;

	cursor_motion.owner = this;
	cursor_motion_absolute.owner = this;
	cursor_button.owner = this;
	cursor_axis.owner = this;
	cursor_frame.owner = this;
}

void Cursor::init() {

	/*
	 * Creates a cursor, which is a wlroots utility for tracking the cursor
	 * image shown on screen.
	 */
	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, server->getOutputLayout());

	/* Creates an xcursor manager, another wlroots utility which loads up
	 * Xcursor themes to source cursor images from and makes sure that cursor
	 * images are available at all scale factors on the screen (necessary for
	 * HiDPI support). */
	cursor_mgr = wlr_xcursor_manager_create(NULL, 24);


	/*
	 * wlr_cursor *only* displays an image on screen. It does not move around
	 * when the pointer moves. However, we can attach input devices to it, and
	 * it will generate aggregate events for all of them. In these events, we
	 * can choose how we want to process them, forwarding them to clients and
	 * moving the cursor around. More detail on this process is described in
	 * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html.
	 *
	 * And more comments are sprinkled throughout the notify functions above.
	 */
	cursor_mode = TINYWL_CURSOR_PASSTHROUGH;
	cursor_motion.listener.notify = server_cursor_motion;
	wl_signal_add(&cursor->events.motion, &cursor_motion.listener);
	cursor_motion_absolute.listener.notify = server_cursor_motion_absolute;
	wl_signal_add(&cursor->events.motion_absolute,
			&cursor_motion_absolute.listener);
	cursor_button.listener.notify = server_cursor_button;
	wl_signal_add(&cursor->events.button, &cursor_button.listener);
	cursor_axis.listener.notify = server_cursor_axis;
	wl_signal_add(&cursor->events.axis, &cursor_axis.listener);
	cursor_frame.listener.notify = server_cursor_frame;
	wl_signal_add(&cursor->events.frame, &cursor_frame.listener);

}

}
