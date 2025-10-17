#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <glib.h>
#include <gio/gio.h>

void connect_proxy_device(GDBusProxy *proxy, GAsyncReadyCallback callback);
void disconnect_proxy_device(GDBusProxy *proxy, GAsyncReadyCallback callback);
GDBusProxy* get_proxy_for_device(gchar* device_path);
const gchar* get_alias_for_device_proxy(GDBusProxy *proxy);
gboolean is_device_connected(GDBusProxy *proxy);
void subscribe_to_properties_changed_signal(GDBusProxy* proxy, GCallback callback);

#endif // BLUETOOTH_H
