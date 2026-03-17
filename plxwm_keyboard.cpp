#include "plxwm_keyboard.h"
#include "plxwm_server.h"

namespace PlxWM {

Keyboard::Keyboard(Server *server, wlr_input_device *device) {
	keyboard = wlr_keyboard_from_input_device(device);
    this->server = server;
    this->device = device;

    modifiers.owner = this;
    key.owner = this;
    destroy.owner = this;
}


static void keyboard_handle_modifiers(wl_listener *listener, void *data) {
    Keyboard *kb = ((Listener<Keyboard> *)listener)->owner;
    kb->onModifiers(&((Listener<Keyboard> *)listener)->listener);

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
	default:
		return false;
	} 
    
	return true;
}

static void keyboard_handle_key(
		struct wl_listener *listener, void *data) {
    Keyboard *kb = ((Listener<Keyboard> *)listener)->owner;
    kb->onKey(&((Listener<Keyboard> *)listener)->listener, (wlr_keyboard_key_event *)data);

}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
    Keyboard *kb = ((Listener<Keyboard> *)listener)->owner;
    kb->onDestroy(&((Listener<Keyboard> *)listener)->listener);
}


void Keyboard::init() {
    
	/* We need to prepare an XKB keymap and assign it to the keyboard. This
	 * assumes the defaults (e.g. layout = "us"). */
	struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
		XKB_KEYMAP_COMPILE_NO_FLAGS);

	wlr_keyboard_set_keymap(keyboard, keymap);
	xkb_keymap_unref(keymap);
	xkb_context_unref(context);
	wlr_keyboard_set_repeat_info(keyboard, 25, 600);

	/* Here we set up listeners for keyboard events. */
	modifiers.listener.notify = keyboard_handle_modifiers;
	wl_signal_add(&keyboard->events.modifiers, &modifiers.listener);
	key.listener.notify = keyboard_handle_key;
	wl_signal_add(&keyboard->events.key, &key.listener);
	destroy.listener.notify = keyboard_handle_destroy;
	wl_signal_add(&device->events.destroy, &destroy.listener);

	wlr_seat_set_keyboard(server->getSeat(), keyboard);

	/* And add the keyboard to our list of keyboards */
	wl_list_insert(server->getKeyboards(), &link);
}


void Keyboard::onModifiers(wl_listener *listener) {
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

	if ((modifiers & WLR_MODIFIER_ALT) && event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		// If alt is held down and this button was _pressed_, we attempt to
		// process it as a compositor keybinding. 
		for (int i = 0; i < nsyms; i++) {
			handled = handleKeyBinding(syms[i]);
		}
	}

	if (!handled) {
		// Otherwise, we pass it along to the client. 
		wlr_seat_set_keyboard(seat, keyboard);
		wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
	}
}

void Keyboard::onDestroy(wl_listener *listener) {
    printf("onDestroy\n");

	/* This event is raised by the keyboard base wlr_input_device to signal
	 * the destruction of the wlr_keyboard. It will no longer receive events
	 * and should be destroyed.
	 */
     
	wl_list_remove(&modifiers.listener.link);
	wl_list_remove(&key.listener.link);
	wl_list_remove(&destroy.listener.link);
	wl_list_remove(&link);
}


}