#include "raylib.h"
#include "bluetooth.h"
#include "dict.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#define ATH_SQ1TW "/org/bluez/hci0/dev_74_45_CE_95_0A_82"
#define VARMILLO  "/org/bluez/hci0/dev_F3_06_1B_9B_89_22"

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 100
#define BUTTONS 2

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

gboolean mouse_in_bounds(int x1, int y1, int x2, int y2) {
  Vector2 mouse_position = GetMousePosition();

  return ((int)mouse_position.x > x1 && (int)mouse_position.x < x2
      && (int)mouse_position.y > y1 && (int)mouse_position.y < y2);
}

int main(void)
{
  Dictionary D = dict_new(BUTTONS);
  link_dict = &D;

  GDBusProxy *ATH_SQ1TW_proxy = get_proxy_for_device(ATH_SQ1TW);
  subscribe_to_properties_changed_signal(ATH_SQ1TW_proxy, G_CALLBACK(on_properties_changed));

  GDBusProxy *VARMILLO_proxy = get_proxy_for_device(VARMILLO);
  subscribe_to_properties_changed_signal(VARMILLO_proxy, G_CALLBACK(on_properties_changed));

  connect_proxy_device(VARMILLO_proxy, NULL);

  Button ATH_SQ1TW_button = {
    10,
    10,
    BUTTON_WIDTH,
    BUTTON_HEIGHT,
    "ATH_SQ1TW",
    ATH_SQ1TW,
    GREEN,
    WHITE,
    .active = is_device_connected(ATH_SQ1TW_proxy),
    on_button_click,
  };

  Button VARMILLO_button = {
    10,
    BUTTON_HEIGHT + 20,
    BUTTON_WIDTH,
    BUTTON_HEIGHT,
    "VARMILLO",
    VARMILLO,
    GREEN,
    WHITE,
    .active = is_device_connected(VARMILLO_proxy),
    on_button_click,
  };

  Link ATH_SQ1TW_link = {
    .proxy = ATH_SQ1TW_proxy,
    .button = &ATH_SQ1TW_button,
  };

  Link VARMILLO_link = {
    .proxy = VARMILLO_proxy,
    .button = &VARMILLO_button,
  };

  dict_insert_kv(ATH_SQ1TW, &ATH_SQ1TW_link, link_dict);  
  dict_insert_kv(VARMILLO, &VARMILLO_link, link_dict);  

  InitWindow(BUTTON_WIDTH + 20, 2 * BUTTON_HEIGHT - 5, "raylib window");
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

  g_object_unref(ATH_SQ1TW_proxy);
  g_object_unref(VARMILLO_proxy);
  free(link_dict->dict);
  CloseWindow();

  return 0;
}
