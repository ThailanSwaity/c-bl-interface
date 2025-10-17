#include "raylib.h"
#include "bluetooth.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>

#define ATH_SQ1TW "/org/bluez/hci0/dev_74_45_CE_95_0A_82"
#define VARMILLO  "/org/bluez/hci0/dev_F3_06_1B_9B_89_22"

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 100
#define BUTTONS 2

static gboolean ATH_SQ1TW_connected = false;
static gboolean VARMILLO_connected  = false;

struct Button {
  int x;
  int y;
  int width;
  int height;
  char text[25];
  Color active_color;
  Color inactive_color;
  gboolean connected;
  GDBusProxy *proxy;
};

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

    if (strcmp(device_path, ATH_SQ1TW) == 0) {
      ATH_SQ1TW_connected = boolean_value;
    }
    else if (strcmp(device_path, VARMILLO) == 0) {
      VARMILLO_connected = boolean_value;
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
  GError *error;
  error = NULL;

  GDBusProxy *ATH_SQ1TW_proxy = get_proxy_for_device(ATH_SQ1TW);
  subscribe_to_properties_changed_signal(ATH_SQ1TW_proxy, G_CALLBACK(on_properties_changed));
  ATH_SQ1TW_connected = is_device_connected(ATH_SQ1TW_proxy);

  GDBusProxy *VARMILLO_proxy = get_proxy_for_device(VARMILLO);
  subscribe_to_properties_changed_signal(VARMILLO_proxy, G_CALLBACK(on_properties_changed));
  VARMILLO_connected = is_device_connected(VARMILLO_proxy);

  Vector2 mouse_position;
  char output_buffer[256];

  struct Button ATH_SQ1TW_button = {
    10,
    10,
    BUTTON_WIDTH,
    BUTTON_HEIGHT,
    "ATH_SQ1TW",
    GREEN,
    WHITE,
    ATH_SQ1TW_connected,
    ATH_SQ1TW_proxy
  };

  struct Button VARMILLO_button = {
    10,
    BUTTON_HEIGHT + 20,
    BUTTON_WIDTH,
    BUTTON_HEIGHT,
    "VARMILLO",
    GREEN,
    WHITE,
    VARMILLO_connected,
    VARMILLO_proxy
  };

  struct Button buttons[BUTTONS] = { ATH_SQ1TW_button, VARMILLO_button };

  gboolean button_1_clicked = false;
  gboolean button_2_clicked = false;

  InitWindow(BUTTON_WIDTH + 20, 2 * BUTTON_HEIGHT - 5, "raylib window");
  while (!WindowShouldClose()) {

    buttons[0].connected = ATH_SQ1TW_connected;
    buttons[1].connected = VARMILLO_connected;

    if (IsMouseButtonPressed(0)) {
      for (int i = 0; i < BUTTONS; i++) {
        if (buttons[i].proxy == NULL) continue;
        if (mouse_in_bounds(buttons[i].x, buttons[i].y, buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height)) {
          if (buttons[i].connected) {
            disconnect_proxy_device(buttons[i].proxy, NULL);
          }
          else {
            connect_proxy_device(buttons[i].proxy, NULL);
          }
        }
      }
    }

    BeginDrawing();

    ClearBackground(BLACK);

    for (int i = 0; i < BUTTONS; i++) {
      DrawRectangleLines(
        buttons[i].x,
        buttons[i].y,
        buttons[i].width,
        buttons[i].height,
        buttons[i].connected ? buttons[i].active_color : buttons[i].inactive_color
      );
      DrawText(buttons[i].text, buttons[i].x + 40, buttons[i].y + buttons[i].height / 2, 20, WHITE);
    }

    EndDrawing();

  }

  g_object_unref(ATH_SQ1TW_proxy);
  g_object_unref(VARMILLO_proxy);
  CloseWindow();

  return 0;
}
