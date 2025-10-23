#include <stdio.h>
#include "bluetooth.h"
#include <glib.h>
#include <gio/gio.h>

void connect_proxy_device(GDBusProxy *proxy, GAsyncReadyCallback callback) {
  GError *error;
  error = NULL;

  g_dbus_proxy_call(
    proxy,
    "Connect",
    NULL,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    NULL,
    callback,
    &error
  );
}

void disconnect_proxy_device(GDBusProxy *proxy, GAsyncReadyCallback callback) {
  GError *error;
  error = NULL;

  g_dbus_proxy_call(
    proxy,
    "Disconnect",
    NULL,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    NULL,
    callback,
    &error
  );
}

GDBusProxy* get_proxy_for_device(gchar* device_path) {
  GError *error;
  error = NULL;

  GDBusProxy *device_proxy = g_dbus_proxy_new_for_bus_sync(
    G_BUS_TYPE_SYSTEM,
    G_DBUS_PROXY_FLAGS_NONE,
    NULL,
    "org.bluez",
    device_path,
    "org.bluez.Device1",
    NULL,
    &error
  );

  if (device_proxy == NULL) {
    g_printerr("Error creating proxy: %s\n", error->message);
    g_error_free(error);
    exit(1);
  }

  return device_proxy;
}

const gchar* get_alias_for_device_proxy(GDBusProxy *proxy) {
  GVariant* variant = g_dbus_proxy_get_cached_property (
    proxy,
    "Alias"
  );

  gsize length;
  if (variant == NULL) {
    perror("Could not get device Alias");
    exit(1);
  }
  const gchar* alias = g_variant_get_string (
    variant,
    &length
  );

  return alias;
}

gboolean is_device_connected(GDBusProxy *proxy) {
  const gchar* property_name = "Connected";
  GVariant* variant = g_dbus_proxy_get_cached_property(
    proxy,
    property_name
  );
  if (variant == NULL) {
    g_printerr("Could not get device property: %s", property_name);
    exit(1);
  }
  gboolean is_connected = g_variant_get_boolean(
    variant
  );
  return is_connected;
}

void subscribe_to_properties_changed_signal(GDBusProxy* proxy, GCallback callback) {
  g_signal_connect(proxy, "g-properties-changed", callback, NULL);
}
