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

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 100
#define BUTTONS 6

typedef struct Button {
  int x;
  int y;
  int width;
  int height;
  char text[25];
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

gboolean mouse_in_bounds(int x1, int y1, int x2, int y2) {
  Vector2 mouse_position = GetMousePosition();

  return ((int)mouse_position.x > x1 && (int)mouse_position.x < x2
      && (int)mouse_position.y > y1 && (int)mouse_position.y < y2);
}

int main(void)
{
  Dictionary D = dict_new(BUTTONS);
  link_dict = &D;
  GDBusProxy *device_manager_proxy = get_proxy_for_device_manager("/");

  // Query ObjectManager for managed devices
  GError *error;
  error = NULL;

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

  printf("Variant type: %s", g_variant_get_type_string(result));

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

  Button *button = (Button *)malloc(devices * sizeof(Button));
  Link *linker = (Link *)malloc(devices * sizeof(Link));
  GDBusProxy **device_proxy;
  device_proxy = malloc(devices * sizeof(GDBusProxy *));

  if (button == NULL || linker == NULL || device_proxy == NULL) {
    printf("Could not allocate memory\n");
  }

  create_device_objects(result, button, linker, device_proxy);
  for (int i = 0; i < devices; i++) {
    dict_insert_kv(linker[i].button->link_name, &linker[i], link_dict);
  }

  InitWindow(BUTTON_WIDTH + 20, devices * (BUTTON_HEIGHT), "raylib window");
  while (!WindowShouldClose()) {

    if (IsMouseButtonPressed(0)) {
      for (int i = 0; i < (int)link_dict->size; i++) {
        Link *L = (Link *)link_dict->dict[i].value;
        Button *button = L->button;
        if (button->on_click == NULL) continue;
        if (mouse_in_bounds(button->x, button->y, button->x + button->width, button->y + button->height)) {
          button->on_click(button->link_name);
        }
      }
    }

    BeginDrawing();

    ClearBackground(BLACK);

    for (int i = 0; i < (int)link_dict->size; i++) {
      Link *L = (Link *)link_dict->dict[i].value;
      Button *button = L->button;
      DrawRectangleLines(
        button->x,
        button->y,
        button->width,
        button->height,
        button->active ? button->active_color : button->inactive_color
      );
      DrawText(button->text, button->x + 40, button->y + button->height / 2, 20, WHITE);
    }

    EndDrawing();

  }

  free(button);
  free(linker);
  free(device_proxy);

  free(link_dict->dict);
  CloseWindow();

  return 0;
}
