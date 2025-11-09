#include "raylib.h"
#include "bluetooth.h"
#include "dict.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#define ATH_SQ1TW "/org/bluez/hci0/dev_74_45_CE_95_0A_82"
#define VARMILLO  "/org/bluez/hci0/dev_F3_06_1B_9B_89_22"
#define OBJECT_PATH_LENGTH 37

#define BUTTON_WIDTH 600
#define BUTTON_HEIGHT 40
#define MAX_DEVICES 100

typedef struct Button {
  int x;
  int y;
  int width;
  int height;
  char text[50];
  char *link_name;
  Color active_color;
  Color inactive_color;
  gboolean active;
  void (*on_click)(char *);
} Button;

typedef struct Link {
  GDBusProxy *proxy;
  Button *button;
} Link;

Dictionary *link_dict;
static Link* global_link_array;
static Button* global_button_array;
static GDBusProxy** global_device_proxy_array;

static void on_properties_changed(GDBusProxy *proxy, GVariant *changed_properties, char** invalid_properties, gpointer user_data);

static void on_button_click(char * link_name) {
  Link *L = (Link *)dict_find_kv(link_name, link_dict);
  if (L == NULL) return;

  if (L->button->active) {
    printf("Trying to disconnect %s\n", L->button->link_name);
    disconnect_proxy_device(L->proxy, NULL);
  }
  else {
    printf("Trying to connect %s\n", L->button->link_name);
    connect_proxy_device(L->proxy, NULL);
  }
}

static void create_device_objects(GVariant *result, Button *button, Link *linker, GDBusProxy **device_proxy) {
  GVariantIter iter;
  GVariant *value;
  gchar *key;
  int i = 0;

  g_variant_iter_init(&iter, result);
  while (g_variant_iter_loop(&iter, "{&o@a{sa{sv}}}", &key, &value)) {
    GVariantIter iiter;
    gchar *interface_name;
    GVariant *interface_value;
    if (strlen(key) == OBJECT_PATH_LENGTH) {

      button[i].x = 10; 
      button[i].y = 10 + (BUTTON_HEIGHT + 10) * i;
      button[i].width = BUTTON_WIDTH; 
      button[i].height = BUTTON_HEIGHT;
      button[i].link_name = key; 
      button[i].active_color = GREEN; 
      button[i].inactive_color = WHITE; 
      button[i].active = false;
      button[i].on_click = on_button_click;

      device_proxy[i] = get_proxy_for_device(key);
      subscribe_to_properties_changed_signal(device_proxy[i], G_CALLBACK(on_properties_changed));

      linker[i].proxy = device_proxy[i];
      linker[i].button = &button[i];
      sprintf(button[i].text, key);

      g_variant_iter_init(&iiter, value);
      g_print("Item '%s' has type '%s'\n", key, g_variant_get_type_string(value));
      while (g_variant_iter_loop(&iiter, "{&s@a{sv}}", &interface_name, &interface_value)) {
        g_print("\tItem '%s' has type '%s'\n", interface_name, g_variant_get_type_string(interface_value));
        GVariantIter iiiter;
        gchar *property_name;
        GVariant *property_value;
        g_variant_iter_init(&iiiter, interface_value);
        while (g_variant_iter_loop(&iiiter, "{sv}", &property_name, &property_value)) {
          g_print("\t\tItem '%s' has type '%s'\n", property_name, g_variant_get_type_string(property_value));
          // I'm looping here because I do not yet know how to directly get the properties I would like out of this data structure :(
          if (strcmp(property_name, "Name") == 0) {
            sprintf(button[i].text, g_variant_get_string(property_value, NULL));
          }
          else if (strcmp(property_name, "Connected") == 0) {
            button[i].active = g_variant_get_boolean(property_value);
          }
        }
        g_variant_unref(property_value);
      }
      i++;
      g_variant_unref(interface_value);
    }
  }
  g_variant_unref(value);
}

static void on_properties_changed(GDBusProxy *proxy, GVariant *changed_properties, char** invalid_properties, gpointer user_data) {
  g_print ("Properties changed on proxy at path %s, interface %s\n",
    g_dbus_proxy_get_object_path (proxy),
    g_dbus_proxy_get_interface_name (proxy)
  );

  GVariantDict *dict = g_variant_dict_new(changed_properties);
  gchar *key = "Connected";
  gboolean success;
  gboolean boolean_value;

  success = g_variant_dict_lookup(dict, key, "b", &boolean_value);
  if (success) {
    const char* device_path = g_dbus_proxy_get_object_path(proxy);

    Link *L = (Link *)dict_find_kv(device_path, link_dict);
    if (L != NULL) {
      L->button->active = boolean_value;
    }

    if (boolean_value) {
      g_print("Device %s connected\n", device_path);
    }
    else {
      g_print("Device %s disconnected\n", device_path);
    }
  } else {
      g_print("Key '%s' not found or value not a string.\n", key);
  }

  g_variant_dict_unref(dict);
}

static void bluez_property_value(const gchar *key, GVariant *value)
{
	const gchar *type = g_variant_get_type_string(value);

	g_print("\t%s : ", key);
	switch(*type) {
		case 'o':
		case 's':
			g_print("%s\n", g_variant_get_string(value, NULL));
			break;
		case 'b':
			g_print("%d\n", g_variant_get_boolean(value));
			break;
		case 'u':
			g_print("%d\n", g_variant_get_uint32(value));
			break;
		case 'a':
		/* TODO Handling only 'as', but not array of dicts */
			if(g_strcmp0(type, "as"))
				break;
			g_print("\n");
			const gchar *uuid;
			GVariantIter i;
			g_variant_iter_init(&i, value);
			while(g_variant_iter_next(&i, "s", &uuid))
				g_print("\t\t%s\n", uuid);
			break;
		default:
			g_print("Other\n");
			break;
	}
}

static void on_device_appeared(GDBusConnection *sig, const gchar *sender_name, const gchar *object_path, const gchar *interface, const gchar *signal_name, GVariant *parameters, gpointer user_data) {
  (void)sig;
  (void)sender_name;
  (void)object_path;
  (void)interface;
  (void)signal_name;
  (void)user_data;

  GVariantIter *interfaces;
  const char *object;
  const gchar *interface_name;
  GVariant *properties;

  int i = link_dict->size;

  g_variant_get(parameters, "(&oa{sa{sv}})", &object, &interfaces);
  while (g_variant_iter_next(interfaces, "{&s@a{sv}}", &interface_name, &properties)) {
    if (g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "device")) {

      gchar *object_name = object;

      const gchar *property_name;
      GVariantIter ii;
      GVariant *prop_val;
      g_variant_iter_init(&ii, properties);
      while (g_variant_iter_next(&ii, "{&sv}", &property_name, &prop_val)) {
        if (strcmp(property_name, "Name") == 0) {
          g_print("Name: %s\n", g_variant_get_string(prop_val, NULL));
          object_name = g_variant_get_string(prop_val, NULL);
        }
      }
      g_print("Device [ %s ] appeared!\n", object_name);

      Link *link = (Link *)dict_find_kv(object, link_dict);

      if (link == NULL) {
        global_button_array[i].x = 10;
        global_button_array[i].y = 10 + (BUTTON_HEIGHT + 10) * i;
        global_button_array[i].width = BUTTON_WIDTH;
        global_button_array[i].height = BUTTON_HEIGHT;
        global_button_array[i].link_name = object;
        global_button_array[i].active_color = GREEN;
        global_button_array[i].inactive_color = WHITE;
        global_button_array[i].active = false;
        global_button_array[i].on_click = on_button_click;

        global_device_proxy_array[i] = get_proxy_for_device(global_button_array->link_name);
        subscribe_to_properties_changed_signal(global_device_proxy_array[i], G_CALLBACK(on_properties_changed));

        global_link_array[i].proxy = global_device_proxy_array[i];
        global_link_array[i].button = &global_button_array[i];
        sprintf(global_button_array[i].text, object_name);

        dict_insert_kv(global_link_array[i].button->link_name, &global_link_array[i], link_dict);
      }

      g_variant_unref(prop_val);
    }
    g_variant_unref(properties);
  }
}

static void on_device_disappeared(GDBusConnection *sig, const gchar *sender_name, const gchar *object_path, const gchar *interface, const gchar *signal_name, GVariant *parameters, gpointer user_data) {
  (void)sig;
  (void)sender_name;
  (void)object_path;
  (void)interface;
  (void)signal_name;

  GVariantIter *interfaces;
  const char *object;
  const gchar *interface_name;

  g_variant_get(parameters, "(&oas)", &object, &interfaces);
  while(g_variant_iter_next(interfaces, "s", &interface_name)) {
    if(g_strstr_len(g_ascii_strdown(interface_name, -1), -1, "device")) {
      g_print("\nDevice %s disappeared\n", object);
      // Remove proxy, unsubscribe to properties changed, fix multiple clicks being registered
      // dict_delete_kv(object, link_dict);
    }
  }
}

gboolean mouse_in_bounds(int x1, int y1, int x2, int y2) {
  Vector2 mouse_position = GetMousePosition();

  return ((int)mouse_position.x > x1 && (int)mouse_position.x < x2
      && (int)mouse_position.y > y1 && (int)mouse_position.y < y2);
}

int main(void)
{
  Dictionary D = dict_new(MAX_DEVICES);
  link_dict = &D;
  GDBusProxy *device_manager_proxy = get_proxy_for_device_manager("/");
  GDBusProxy *device_adapter_proxy = get_proxy_for_object("/org/bluez/hci0", "org.bluez.Adapter1");

  GError *error;
  error = NULL;

  // Query ObjectManager for managed devices
  GVariant *result = NULL;
  result = g_dbus_proxy_call_sync(
    device_manager_proxy,
    "GetManagedObjects",
    NULL,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    NULL,
    &error
  );

  if (result == NULL) {
    perror("Result was NULL");
    exit(1);
  }

  int devices = 0;

  // Get the number of devices required
  GVariantIter iter;
  GVariant *value;
  gchar *key;

  result = g_variant_get_child_value(result, 0);
  g_variant_iter_init(&iter, result);
  while (g_variant_iter_loop(&iter, "{&o@a{sa{sv}}}", &key, &value)) {
    if (strlen(key) == OBJECT_PATH_LENGTH) devices++;
  }
  // ----------------------------------------

  printf("Devices: %d\n", devices);

  Button *button = (Button *)malloc(MAX_DEVICES * sizeof(Button));
  global_button_array = button;
  Link *linker = (Link *)malloc(MAX_DEVICES * sizeof(Link));
  global_link_array = linker;
  GDBusProxy **device_proxy;
  device_proxy = malloc(MAX_DEVICES * sizeof(GDBusProxy *));
  global_device_proxy_array = device_proxy;

  if (button == NULL || linker == NULL || device_proxy == NULL) {
    printf("Could not allocate memory\n");
  }

  create_device_objects(result, button, linker, device_proxy);
  for (int i = 0; i < devices; i++) {
    dict_insert_kv(linker[i].button->link_name, &linker[i], link_dict);
  }

  error = NULL;
  guint iface_added;
  guint iface_removed;

  iface_added = g_dbus_connection_signal_subscribe(
    g_dbus_proxy_get_connection(device_manager_proxy),
    "org.bluez",
    "org.freedesktop.DBus.ObjectManager",
    "InterfacesAdded",
    NULL,
    NULL,
    G_DBUS_SIGNAL_FLAGS_NONE,
    on_device_appeared,
    NULL,
    NULL
  );

  iface_removed = g_dbus_connection_signal_subscribe(
    g_dbus_proxy_get_connection(device_manager_proxy),
    "org.bluez",
    "org.freedesktop.DBus.ObjectManager",
    "InterfacesRemoved",
    NULL,
    NULL,
    G_DBUS_SIGNAL_FLAGS_NONE,
    on_device_disappeared,
    NULL,
    NULL
  );

  error = NULL;

  g_dbus_proxy_call_sync(
    device_adapter_proxy,
    "StartDiscovery",
    NULL,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    NULL,
    &error
  );

  if (error != NULL) {
    g_print("Could not start discovery..\n");
  }


  InitWindow(BUTTON_WIDTH + 20, 800, "raylib window");
  while (!WindowShouldClose()) {

    BeginDrawing();

    ClearBackground(BLACK);

    for (int i = 0; i < (int)link_dict->size; i++) {
      Link *L = (Link *)link_dict->dict[i].value;
      Button *button = L->button;

      if (IsMouseButtonPressed(0)) {
        if (mouse_in_bounds(button->x, button->y, button->width, button->y + button->height))
          button->on_click(button->link_name);
      }

      DrawRectangleLines(
        button->x,
        button->y,
        button->width,
        button->height,
        button->active ? button->active_color : button->inactive_color
      );
      DrawText(button->text, button->x + 40, button->y, 20, WHITE);
    }

    EndDrawing();

  }

  error = NULL;

  g_dbus_proxy_call_sync(
    device_adapter_proxy,
    "StopDiscovery",
    NULL,
    G_DBUS_CALL_FLAGS_NONE,
    -1,
    NULL,
    &error
  );

  if (error != NULL) {
    g_print("Could not stop discovery..\n");
  }

  free(button);
  free(linker);
  free(device_proxy);
  g_dbus_connection_signal_unsubscribe(
    g_dbus_proxy_get_connection(device_manager_proxy),
    iface_added
  );
  g_dbus_connection_signal_unsubscribe(
    g_dbus_proxy_get_connection(device_manager_proxy),
    iface_removed
  );

  free(link_dict->dict);
  CloseWindow();

  return 0;
}
