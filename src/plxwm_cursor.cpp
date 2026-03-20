#include "plxwm_cursor.h"
#include "plxwm_server.h"
#include "plxwm_appwindow.h"

namespace PlxWM {

void Cursor::onMove(uint32_t time) {
	if (cursor_mode == TINYWL_CURSOR_MOVE && server->getGrabbedWindow()) {
		
        AppWindow *appWindow = server->getGrabbedWindow();

        wlr_scene_node_set_position(&appWindow->getSceneTree()->node,
            cursor->x - server->getGrabX(),
            cursor->y - server->getGrabY());

		return;
	} else if (cursor_mode == TINYWL_CURSOR_RESIZE && server->getGrabbedWindow()) {
		server->processResize();
		return;
	}

	double sx, sy;
	struct wlr_seat *seat = server->getSeat();
	struct wlr_surface *surface = server->getSurfaceAt(cursor->x, cursor->y, &sx, &sy);
	
	if (surface) {
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		wlr_cursor_set_xcursor(cursor, cursor_mgr, "default");
		wlr_seat_pointer_clear_focus(seat);
	}
}


void Cursor::onMotion(wl_listener *listener, wlr_pointer_motion_event *event) {
    wlr_cursor_move(cursor, &event->pointer->base,
			event->delta_x, event->delta_y);

    onMove(event->time_msec);
}

void Cursor::onMotionAbsolute(wl_listener *listener, wlr_pointer_motion_absolute_event *event) {
	wlr_cursor_warp_absolute(cursor, &event->pointer->base, event->x, event->y);

    onMove(event->time_msec);
}

void Cursor::onButton(struct wl_listener *listener, wlr_pointer_button_event *event) {
	wlr_seat_pointer_notify_button(server->getSeat(),
			event->time_msec, event->button, event->state);
			
	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
		cursor_mode = TINYWL_CURSOR_PASSTHROUGH;
		server->setGrabbedWindow(NULL);
	} else {
		double sx, sy;

		AppWindow *wnd = server->getWindowAt(cursor->x, cursor->y, &sx, &sy);

		server->focus(wnd);
	}
}

void Cursor::onCursorAxis(wl_listener *listener, wlr_pointer_axis_event *event) {
    printf("ON server_cursor_axis\n");

	/* This event is forwarded by the cursor when a pointer emits an axis event,
	 * for example when you move the scroll wheel. 
	struct tinywl_server *server =
		wl_container_of(listener, server, cursor_axis);
	struct wlr_pointer_axis_event *event = data;
	// Notify the client with pointer focus of the axis event.  */
	wlr_seat_pointer_notify_axis(server->getSeat(),
			event->time_msec, event->orientation, event->delta,
			event->delta_discrete, event->source, event->relative_direction);
}

void Cursor::onCursorFrame(struct wl_listener *listener, void *data) {
	wlr_seat_pointer_notify_frame(server->getSeat());
}

Cursor::Cursor(Server *server) {
    this->server = server;

	cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(cursor, server->getOutputLayout());

	cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	cursor_mode = TINYWL_CURSOR_PASSTHROUGH;

	motion = make_unique<Signal<&Cursor::onMotion>>(this, &cursor->events.motion);
	motionAbsolute = make_unique<Signal<&Cursor::onMotionAbsolute>>(this, &cursor->events.motion_absolute);
	button = make_unique<Signal<&Cursor::onButton>>(this, &cursor->events.button);
	axis = make_unique<Signal<&Cursor::onCursorAxis>>(this, &cursor->events.axis);
	frame = make_unique<Signal<&Cursor::onCursorFrame>>(this, &cursor->events.frame);
}

}
