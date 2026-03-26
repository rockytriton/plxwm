#include "plxwm_server.h"
#include "plxwm_wayland.h"

#include "plxwm-ctrl-protocol.h"

namespace PlxWM {

void ctrl_logout(wl_client *client, wl_resource *resource) {
	printf("Logout Message Received\n");

    Server *server = (Server *)wl_resource_get_user_data(resource);

    server->logout(); 
}

const struct plxwm_ctrl_interface ctrl_impl = {
    .logout = ctrl_logout,
};

void bindProtocol(wl_client *client, void *data, uint32_t version, uint32_t id) {
    Server *server = static_cast<Server*>(data);
    wl_resource *resource = wl_resource_create(client, &plxwm_ctrl_interface, version, id);
    
    wl_resource_set_implementation(resource, &ctrl_impl, server, NULL);
}

void registerServerProtocol(Server *server) {
    wl_global_create(server->getDisplay(), &plxwm_ctrl_interface, 1, server, bindProtocol);
}

}