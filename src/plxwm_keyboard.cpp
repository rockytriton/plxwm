#include "plxwm_keyboard.h"
#include "plxwm_server.h"

namespace PlxWM {

Keyboard::Keyboard(Server *server, wlr_input_device *device) {
	keyboard = wlr_keyboard_from_input_device(device);
    this->server = server;
    this->device = device;

	/* We need to prepare an XKB keymap and assign it to the keyboard. This
	 * assumes the defaults (e.g. layout = "us"). */
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
		XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(keyboard, 25, 600);

	modifiers = make_unique<Signal<&Keyboard::onModifiers>>(this, &keyboard->events.modifiers);
	key = make_unique<Signal<&Keyboard::onKey>>(this, &keyboard->events.key);
	destroy = make_unique<Signal<&Keyboard::onDestroy>>(this, &device->events.destroy);

	wlr_seat_set_keyboard(server->getSeat(), keyboard);
}

bool Keyboard::handleKeyBinding(xkb_keysym_t sym) {
	/*
	 * Here we handle compositor keybindings. This is when the compositor is
	 * processing keys, rather than passing them on to the client for its own
	 * processing.
	 *
	 * This function assumes Alt is held down.
	*/

    printf("KB: %d, %d\n", XKB_KEY_Escape, sym);

	switch (sym) {
	case XKB_KEY_Escape:
		wl_display_terminate(server->getDisplay());
		break;
	case XKB_KEY_F1:
		printf("KEY F1\n");

		setenv("WAYLAND_DISPLAY", server->getSocket(), true);
		if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", "konsole", (void *)NULL);
		}break;
	case XKB_KEY_F2:
		printf("KEY F1\n");

		setenv("WAYLAND_DISPLAY", server->getSocket(), true);
		if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", "firefox", (void *)NULL);
		}break;
	case XKB_KEY_F3:
		printf("KEY F1\n");

		setenv("WAYLAND_DISPLAY", server->getSocket(), true);
		if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", "code", (void *)NULL);
		}break;
		
	default:
		return false;
	} 
    
	return true;
}


void Keyboard::onModifiers(wl_listener *listener, void *data) {
    printf("onModifiers\n");

	// This event is raised when a modifier key, such as shift or alt, is
	// pressed. We simply communicate this to the client. 

	/*
	 * A seat can only have one keyboard, but this is a limitation of the
	 * Wayland protocol - not wlroots. We assign all connected keyboards to the
	 * same seat. You can swap out the underlying wlr_keyboard like this and
	 * wlr_seat handles this transparently.
	 */
	wlr_seat_set_keyboard(server->getSeat(), keyboard);

	/* Send modifiers to the client. */
	wlr_seat_keyboard_notify_modifiers(server->getSeat(), &keyboard->modifiers);
}

void Keyboard::onKey(wl_listener *listener, wlr_keyboard_key_event *event) {
    printf("onKey\n");

	// This event is raised when a key is pressed or released. 
	wlr_seat *seat = server->getSeat();

	// Translate libinput keycode -> xkbcommon 
	uint32_t keycode = event->keycode + 8;

	// Get a list of keysyms based on the keymap for this keyboard 
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->xkb_state, keycode, &syms);

	bool handled = false;

	uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard);

	if ((modifiers & WLR_MODIFIER_ALT) && !(modifiers & WLR_MODIFIER_CTRL) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		// If alt is held down and this button was _pressed_, we attempt to
		// process it as a compositor keybinding. 
		for (int i = 0; i < nsyms; i++) {
			handled = handleKeyBinding(syms[i]);
		}
	}

	if (!handled) {
		printf("PASSING ON\n");
		// Otherwise, we pass it along to the client.
		wlr_seat_set_keyboard(seat, keyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
	}
}

void Keyboard::onDestroy(wl_listener *listener, void *data) {
    printf("onDestroy\n");

	/* This event is raised by the keyboard base wlr_input_device to signal
	 * the destruction of the wlr_keyboard. It will no longer receive events
	 * and should be destroyed.
	 */
    
	modifiers->cleanup();
	key->cleanup();
	destroy->cleanup();
	
	//wl_list_remove(&link);
}


}
