#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdbool.h>

#include "style.h"
#include "vsalib.h"

#define WINDOW_TITLE "Vivid Screen Assistant (V1.20)"

#define COMMAND_BUFFER_SIZE 512

#define TEST_DURATION   3000
#define TEST_WHITE_CSS  "window { background-color: #fff; }"
#define TEST_BLACK_CSS  "window { background-color: #000; }"
#define TEST_RED_CSS    "window { background-color: #f00; }"
#define TEST_GREEN_CSS  "window { background-color: #0f0; }"
#define TEST_BLUE_CSS   "window { background-color: #00f; }"

typedef struct
{
  GtkWidget *radio_do_nothing;
  GtkWidget *radio_send_key;
  GtkWidget *radio_run_command;
  GtkWidget *send_key_entry;
  GtkWidget *send_key_button;
  GtkWidget *run_command_entry;
  GtkWidget *run_command_button;
} ButtonActionEditor;


char command_buffer[COMMAND_BUFFER_SIZE];

bool auto_test = false;

GtkWidget * window;
GtkWidget * rotate_left_button;
GtkWidget * rotate_normal_button;
GtkWidget * rotate_right_button;
GtkWidget * rotate_inverted_button;
GtkWidget * brightness_slider;
GtkWidget * test_window = NULL;

GtkWidget * run_test(gpointer data);

ButtonActionEditor left_button_action_editor;
ButtonActionEditor center_button_action_editor;
ButtonActionEditor right_button_action_editor;


void load_css(void) 
{
  GtkCssProvider *provider = gtk_css_provider_new();
  GdkDisplay *display = gdk_display_get_default();
  GdkScreen *screen = gdk_display_get_default_screen(display);
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_css_provider_load_from_data(provider, style_css, -1, NULL);
  g_object_unref(provider);
}


int get_display_rotation()
{
  if (run_command("cat ~/.config/autostart/vsa_rotate.desktop > /dev/null 2>&1") < 0)
  {
    sprintf(output_buffer, "right");
    return 5;
  }
  return run_command("cat ~/.config/autostart/vsa_rotate.desktop | grep Exec= | sed s/\"Exec=xrandr --output DSI-1 --rotate \"//");
}


gboolean update_rotate_buttons(gpointer data)
{
  get_display_rotation();
  if (strncmp("left", output_buffer, 4) == 0)
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rotate_left_button), TRUE);
  }
  else if (strncmp("normal", output_buffer, 6) == 0)
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rotate_normal_button), TRUE);
  }
  else if (strncmp("right", output_buffer, 5) == 0)
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rotate_right_button), TRUE);
  }
  else if (strncmp("inverted", output_buffer, 8) == 0)
  {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rotate_inverted_button), TRUE);
  }
  return FALSE;
}


int rotate_display(const char * rotation)
{
  run_command("mkdir ~/.config/autostart");
  run_command("printf ~");
  sprintf(command_buffer, "%s/.config/autostart/vsa_rotate.desktop", output_buffer);
  FILE *file = fopen(command_buffer, "w");
  if (file == NULL) {
    printf("Unable to create the file: %s\n", command_buffer);
    return -3;
  }
  fprintf(file, "[Desktop Entry]\nType=Application\nName=Rotate Screen\nExec=xrandr --output DSI-1 --rotate %s", rotation);
  fclose(file);
  
  sprintf(command_buffer, "xrandr --output DSI-1 --rotate %s", rotation);
  return run_command(command_buffer);
}


int rotate_touchscreen(const char * rotation)
{
  run_command("sudo chmod 776 /etc/X11/xorg.conf.d/40-libinput.conf");
  
  char *left = "# 90 left\nSection \"InputClass\"\n        Identifier \"libinput touchscreen catchall\"\n        MatchIsTouchscreen \"on\"\n        MatchDevicePath \"/dev/input/event*\"\n        Option \"TransformationMatrix\" \"0 -1 1 1 0 0 0 0 1\"\n        Driver \"libinput\"\nEndSection";

  char *normal = "# 0 normal\nSection \"InputClass\"\n        Identifier \"libinput touchscreen catchall\"\n        MatchIsTouchscreen \"on\"\n        MatchDevicePath \"/dev/input/event*\"\n        Option \"TransformationMatrix\" \"1 0 0 0 1 0 0 0 1\"\n        Driver \"libinput\"\nEndSection";

  char *right = "# 270 right\nSection \"InputClass\"\n        Identifier \"libinput touchscreen catchall\"\n        MatchIsTouchscreen \"on\"\n        MatchDevicePath \"/dev/input/event*\"\n        Option \"TransformationMatrix\" \"0 1 0 -1 0 1 0 0 1\"\n        Driver \"libinput\"\nEndSection";
  
  char *inverted = "# 180 inverted\nSection \"InputClass\"\n        Identifier \"libinput touchscreen catchall\"\n        MatchIsTouchscreen \"on\"\n        MatchDevicePath \"/dev/input/event*\"\n        Option \"TransformationMatrix\" \"-1 0 1 0 -1 1 0 0 1\"\n        Driver \"libinput\"\nEndSection";
  
  sprintf(command_buffer, "sudo echo '%s' > /etc/X11/xorg.conf.d/40-libinput.conf", left);
  if (strcasecmp(rotation, "normal") == 0)
  {
    sprintf(command_buffer, "sudo echo '%s' > /etc/X11/xorg.conf.d/40-libinput.conf", normal);
  }
  else if (strcasecmp(rotation, "right") == 0)
  {
    sprintf(command_buffer, "sudo echo '%s' > /etc/X11/xorg.conf.d/40-libinput.conf", right);
  }
  else if (strcasecmp(rotation, "inverted") == 0)
  {
    sprintf(command_buffer, "sudo echo '%s' > /etc/X11/xorg.conf.d/40-libinput.conf", inverted);
  }

  run_command(command_buffer);
}


void orientation_button_clicked(GtkToggleButton *button, gpointer data)
{
  const char *name = gtk_widget_get_name(GTK_WIDGET(button));
  
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
  {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK_CANCEL,
      "You will restart the X session (unsaved work will be lost), proceed?");
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK)
    {
      gtk_widget_destroy(dialog);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
      return;
    }
    gtk_widget_destroy(dialog);
    
    GList *buttons = gtk_container_get_children(GTK_CONTAINER(data));
    GList *iter;
    for (iter = buttons; iter != NULL; iter = g_list_next(iter))
    {
      if (GTK_IS_TOGGLE_BUTTON(iter->data) && iter->data != button)
      {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(iter->data), FALSE);
      }
    }
    g_list_free(buttons);

    rotate_display(name);
    
    rotate_touchscreen(name);
    
    run_command("sudo systemctl restart display-manager");
  }
}


int get_max_brightness()
{
  sprintf(command_buffer, "cat /sys/class/backlight/backlight/max_brightness");
  int length = run_command(command_buffer);
  if (length > 0)
  {
    return atoi(output_buffer);
  }
  return 100;
}


int get_current_brightness()
{
  sprintf(command_buffer, "cat /sys/class/backlight/backlight/brightness");
  int length = run_command(command_buffer);
  if (length > 0)
  {
    return atoi(output_buffer);
  }
  return 0;
}


int set_brightness(int brightness)
{
  run_command("sudo chmod 666 /sys/class/backlight/backlight/brightness");
  sprintf(command_buffer, "echo %d > /sys/class/backlight/backlight/brightness", brightness);
  return run_command(command_buffer);
}


void adjust_brightness(GtkRange *range, gpointer user_data) {
  gdouble value = gtk_range_get_value(range);
  set_brightness((int)value);
}


void brightness_button_clicked(GtkToggleButton *button, gpointer data)
{
  int percentage = GPOINTER_TO_UINT(data);
  gtk_range_set_value(GTK_RANGE(brightness_slider), 255 * percentage / 100);
}


gboolean destory_test_window() {
  if (auto_test && test_window != NULL)
  {
    const char *css = gtk_widget_get_name(test_window);
    if (strcmp(css, TEST_WHITE_CSS) == 0)
    {
      run_test(TEST_BLACK_CSS);
    }
    else if (strcmp(css, TEST_BLACK_CSS) == 0)
    {
      run_test(TEST_RED_CSS);
    }
    else if (strcmp(css, TEST_RED_CSS) == 0)
    {
      run_test(TEST_GREEN_CSS);
    }
    else if (strcmp(css, TEST_GREEN_CSS) == 0)
    {
      run_test(TEST_BLUE_CSS);
    }
    else if (strcmp(css, TEST_BLUE_CSS) == 0)
    {
      auto_test = false;
    }
  }
  if (!auto_test)
  {
    gdk_window_set_cursor(
      gdk_screen_get_root_window(gdk_screen_get_default()), 
      gdk_cursor_new_for_display(gdk_display_get_default(), GDK_LEFT_PTR));
    gtk_widget_destroy(test_window);
    test_window = NULL;
  }
  return G_SOURCE_REMOVE;
}


GtkWidget * run_test(gpointer data)
{
  const char * css = (const char *)data;
  
  if (test_window == NULL)
  {
    test_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(test_window));
    gtk_window_set_decorated(GTK_WINDOW(test_window), FALSE);
    gtk_window_set_hide_titlebar_when_maximized(GTK_WINDOW(test_window), TRUE);
    
    GdkCursor *blank_cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR);
    gdk_window_set_cursor(gdk_screen_get_root_window(gdk_screen_get_default()), blank_cursor);
    g_object_unref(blank_cursor);
  }
  
  gtk_widget_set_name(test_window, css);

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider, css, -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context(test_window);
  gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(provider);

  g_timeout_add(TEST_DURATION, (GSourceFunc)destory_test_window, NULL);
  gtk_widget_show_all(test_window);
  
  return test_window;
}


void test_button_clicked(GtkButton *button, gpointer data)
{
  auto_test = false;
  run_test(data);
}


void auto_button_clicked(GtkButton *button, gpointer data)
{
  auto_test = true;
  run_test(TEST_WHITE_CSS);
}


void write_button_action(FILE * file, ButtonActionEditor * editor)
{
  if (file != NULL && editor != NULL)
  {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editor->radio_send_key)))
    {
      fprintf(file, "key:%s\n", gtk_entry_get_text(GTK_ENTRY(editor->send_key_entry)));
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editor->radio_run_command)))
    {
      fprintf(file, "cmd:%s\n", gtk_entry_get_text(GTK_ENTRY(editor->run_command_entry)));
    }
    else
    {
      fprintf(file, "non:\n");
    }
  }
}

void load_button_actions()
{
  FILE *file = fopen(PATH_BUTTON_ACTIONS_CFG, "r");
  if (!file)
  {
    perror("Error opening file button_actions.cfg");
    return;
  }
  int line_number = 0;
  char line[1024];
  while (fgets(line, sizeof(line), file) != NULL)
  {
    line_number++;
    size_t pos = strcspn(line, "\n");
    line[pos] = '\0';
    ButtonActionEditor *editor = NULL;
    if (line_number == LEFT_BUTTON_ACTION_LINE)
    {
      editor = &left_button_action_editor;
    }
    else if (line_number == CENTER_BUTTON_ACTION_LINE)
    {
      editor = &center_button_action_editor;
    }
    else if (line_number == RIGHT_BUTTON_ACTION_LINE)
    {
      editor = &right_button_action_editor;
    }
    if (editor != NULL)
    {
      char *type = strtok(line, ":");
      if (type != NULL)
      {
        if (strcmp(type, "non") == 0)
        {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editor->radio_do_nothing), TRUE);
        }
        else if (strcmp(type, "key") == 0)
        {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editor->radio_send_key), TRUE);
          char *value = strtok(NULL, ":");
          if (value != NULL)
          {
            gtk_entry_set_text(GTK_ENTRY(editor->send_key_entry), value);
          }
        }
        else if (strcmp(type, "cmd") == 0)
        {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editor->radio_run_command), TRUE);
          char *value = strtok(NULL, ":");
          if (value != NULL)
          {
            gtk_entry_set_text(GTK_ENTRY(editor->run_command_entry), value);
          }
        }
      }
    }
  }
  fclose(file);
}

void save_button_actions()
{
  FILE *file = fopen(PATH_BUTTON_ACTIONS_CFG, "w");
  if (file == NULL) {
    fprintf(stderr, "Can not open file button_actions.cfg for writing\n");
    return;
  }
  write_button_action(file, &left_button_action_editor);
  write_button_action(file, &center_button_action_editor);
  write_button_action(file, &right_button_action_editor);
  fclose(file);
}

void none_button_action_selected(GtkToggleButton *togglebutton, gpointer data)
{
  ButtonActionEditor *editor = (ButtonActionEditor *)data;
  gtk_widget_set_sensitive(editor->send_key_entry, FALSE);
  gtk_entry_set_text(GTK_ENTRY(editor->send_key_entry), "");
  gtk_widget_set_sensitive(editor->send_key_button, FALSE);
  gtk_widget_set_sensitive(editor->run_command_entry, FALSE);
  gtk_entry_set_text(GTK_ENTRY(editor->run_command_entry), "");
  gtk_widget_set_sensitive(editor->run_command_button, FALSE);
  save_button_actions();
}


void key_button_action_selected(GtkToggleButton *togglebutton, gpointer data)
{
  ButtonActionEditor *editor = (ButtonActionEditor *)data;
  gtk_widget_set_sensitive(editor->send_key_entry, TRUE);
  gtk_widget_set_sensitive(editor->send_key_button, TRUE);
  gtk_widget_set_sensitive(editor->run_command_entry, FALSE);
  gtk_entry_set_text(GTK_ENTRY(editor->run_command_entry), "");
  gtk_widget_set_sensitive(editor->run_command_button, FALSE);
  save_button_actions();
}


void command_button_action_selected(GtkToggleButton *togglebutton, gpointer data)
{
  ButtonActionEditor *editor = (ButtonActionEditor *)data;
  gtk_widget_set_sensitive(editor->send_key_entry, FALSE);
  gtk_entry_set_text(GTK_ENTRY(editor->send_key_entry), "");
  gtk_widget_set_sensitive(editor->send_key_button, FALSE);
  gtk_widget_set_sensitive(editor->run_command_entry, TRUE);
  gtk_widget_set_sensitive(editor->run_command_button, TRUE);
  save_button_actions();
}


void menu_item_clicked(GtkMenuItem *menu_item, gpointer value) {
  GtkWidget *send_key_entry = g_object_get_data(G_OBJECT(menu_item), "entry");
  gtk_entry_set_text(GTK_ENTRY(send_key_entry), value);
  free(value);
}


void send_key_button_clicked(GtkWidget *widget, gpointer data)
{
  ButtonActionEditor *editor = (ButtonActionEditor *)data;
  
  GtkWidget *menu = gtk_menu_new();
  
  // load known_keys.cfg and create menu items
  char line[1024];
  FILE *file = fopen(PATH_KNOWN_KEYS_CFG, "r");
  if (file == NULL)
  {
    perror("Error opening known_keys.cfg file");
    return;
  }
  while (fgets(line, sizeof(line), file))
  {
    char *name = strtok(line, ":");
    if (name != NULL)
    {
      char *key = strtok(NULL, ":");
      if (key != NULL)
      {
        size_t pos = strcspn(key, "\n");
        key[pos] = '\0';
        char *value = (char *)malloc(strlen(key) + 1);
        strcpy(value, key);
        
        GtkWidget *menu_item = gtk_menu_item_new_with_label(name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        
        g_object_set_data(G_OBJECT(menu_item), "entry", editor->send_key_entry);
        g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(menu_item_clicked), value);
      }
    }
  }
  fclose(file);

  gtk_widget_show_all(menu);
  gtk_menu_popup_at_widget(GTK_MENU(menu), editor->send_key_button, GDK_GRAVITY_NORTH_EAST, GDK_GRAVITY_NORTH_WEST, NULL);
}


gboolean filter_executables(const GtkFileFilterInfo *info, gpointer user_data) {
  const gchar *filename = info->filename;
  gboolean is_executable = g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE);
  return is_executable;
}

void run_command_button_clicked(GtkWidget *widget, gpointer data)
{
  ButtonActionEditor *editor = (ButtonActionEditor *)data;
  
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
			"Choose a command",
			GTK_WINDOW(window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"_Cancel",
			GTK_RESPONSE_CANCEL,
			"_Select",
			GTK_RESPONSE_ACCEPT,
			NULL);
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, filter_executables, NULL, NULL);
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
  
  gint res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT) {
    char *filename;
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    filename = gtk_file_chooser_get_filename(chooser);
    gtk_entry_set_text(GTK_ENTRY(editor->run_command_entry), filename);
    g_free(filename);
  }
  
  gtk_widget_destroy(dialog);
}


void on_entry_changed(GtkEditable *editable, gpointer user_data) {
  save_button_actions();
}


void create_button_action_editor(ButtonActionEditor * editor, GtkWidget * container, char * title)
{
  GtkWidget *button_frame = gtk_frame_new(title);    
  gtk_frame_set_label_align(GTK_FRAME(button_frame), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(container), button_frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(button_frame), 10);
  
  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
  gtk_container_set_border_width(GTK_CONTAINER(button_box), 10);
  gtk_container_add(GTK_CONTAINER(button_frame), button_box);
  
  editor->radio_do_nothing = gtk_radio_button_new_with_label(NULL, "Do nothing");
  
  gtk_box_pack_start(GTK_BOX(button_box), editor->radio_do_nothing, FALSE, FALSE, 0);
  
  GtkWidget *send_key_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_box_pack_start(GTK_BOX(button_box), send_key_box, TRUE, TRUE, 0);
  
  editor->radio_send_key = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(editor->radio_do_nothing), "Send a hotkey");
  
  gtk_box_pack_start(GTK_BOX(send_key_box), editor->radio_send_key, FALSE, FALSE, 0);
  
  editor->send_key_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(send_key_box), editor->send_key_entry, TRUE, TRUE, 0);
  
  editor->send_key_button = gtk_button_new_with_label("...");
  gtk_box_pack_start(GTK_BOX(send_key_box), editor->send_key_button, FALSE, FALSE, 0);
  
  GtkWidget *run_command_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  gtk_box_pack_start(GTK_BOX(button_box), run_command_box, TRUE, TRUE, 0);
 
  editor->radio_run_command = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(editor->radio_do_nothing), "Run a command");
  gtk_box_pack_start(GTK_BOX(run_command_box), editor->radio_run_command, FALSE, FALSE, 0);
  
  editor->run_command_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(run_command_box), editor->run_command_entry, TRUE, TRUE, 0);
  
  editor->run_command_button = gtk_button_new_with_label("...");
  gtk_box_pack_start(GTK_BOX(run_command_box), editor->run_command_button, FALSE, FALSE, 0);  
  
  g_signal_connect(G_OBJECT(editor->radio_do_nothing), "toggled", G_CALLBACK(none_button_action_selected), editor);
  
  g_signal_connect(G_OBJECT(editor->radio_send_key), "toggled", G_CALLBACK(key_button_action_selected), editor);
  g_signal_connect(G_OBJECT(editor->send_key_entry), "changed", G_CALLBACK(on_entry_changed), NULL);
  g_signal_connect(G_OBJECT(editor->send_key_button), "clicked", G_CALLBACK(send_key_button_clicked), editor);
  
  g_signal_connect(G_OBJECT(editor->radio_run_command), "toggled", G_CALLBACK(command_button_action_selected), editor);
  g_signal_connect(G_OBJECT(editor->run_command_entry), "changed", G_CALLBACK(on_entry_changed), NULL);
  g_signal_connect(G_OBJECT(editor->run_command_button), "clicked", G_CALLBACK(run_command_button_clicked), editor);
}






int main(int argc, char *argv[])
{
  gtk_init(&argc, &argv);
  
  load_css();

  // main window
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
  gtk_widget_set_size_request(window, 400, 200);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
  
  // notebook (tabs)
  GtkWidget *notebook = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(window), notebook);
  
  // Screen tab
  GtkWidget *screen_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), screen_tab, gtk_label_new("Screen"));

  // orientation
  GtkWidget *orientation_frame = gtk_frame_new("Orientation");
  gtk_frame_set_label_align(GTK_FRAME(orientation_frame), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(screen_tab), orientation_frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(orientation_frame), 10);

  GtkWidget *orientation_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(orientation_box), 10);
  gtk_container_add(GTK_CONTAINER(orientation_frame), orientation_box);

  rotate_right_button = gtk_toggle_button_new();
  gtk_widget_set_name(rotate_right_button, "right");
  gtk_box_pack_start(GTK_BOX(orientation_box), rotate_right_button, FALSE, FALSE, 0);
  gtk_button_set_image(GTK_BUTTON(rotate_right_button), gtk_image_new_from_file ("/usr/share/icons/hicolor/48x48/apps/vsa_3.png"));

  rotate_normal_button = gtk_toggle_button_new();
  gtk_widget_set_name(rotate_normal_button, "normal");
  gtk_box_pack_start(GTK_BOX(orientation_box), rotate_normal_button, FALSE, FALSE, 0);
  gtk_button_set_image(GTK_BUTTON(rotate_normal_button), gtk_image_new_from_file ("/usr/share/icons/hicolor/48x48/apps/vsa_0.png"));

  rotate_left_button = gtk_toggle_button_new();
  gtk_widget_set_name(rotate_left_button, "left");
  gtk_box_pack_start(GTK_BOX(orientation_box), rotate_left_button, FALSE, FALSE, 0);
  gtk_button_set_image(GTK_BUTTON(rotate_left_button), gtk_image_new_from_file ("/usr/share/icons/hicolor/48x48/apps/vsa_2.png"));
  
  rotate_inverted_button = gtk_toggle_button_new();
  gtk_widget_set_name(rotate_inverted_button, "inverted");
  gtk_box_pack_start(GTK_BOX(orientation_box), rotate_inverted_button, FALSE, FALSE, 0);
  gtk_button_set_image(GTK_BUTTON(rotate_inverted_button), gtk_image_new_from_file ("/usr/share/icons/hicolor/48x48/apps/vsa_1.png"));

  update_rotate_buttons(NULL);
  
  g_signal_connect(rotate_left_button, "toggled", G_CALLBACK(orientation_button_clicked), orientation_box);
  g_signal_connect(rotate_normal_button, "toggled", G_CALLBACK(orientation_button_clicked), orientation_box);
  g_signal_connect(rotate_right_button, "toggled", G_CALLBACK(orientation_button_clicked), orientation_box);
  g_signal_connect(rotate_inverted_button, "toggled", G_CALLBACK(orientation_button_clicked), orientation_box);
  
  // brightness
  GtkWidget *brightness_frame = gtk_frame_new("Brightness");
  gtk_frame_set_label_align(GTK_FRAME(brightness_frame), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(screen_tab), brightness_frame, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(brightness_frame), 10);

  GtkWidget *brightness_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_set_border_width(GTK_CONTAINER(brightness_box), 10);
  gtk_container_add(GTK_CONTAINER(brightness_frame), brightness_box);

  int brightness_max = get_max_brightness();
  int brightness_min = brightness_max / 5;
  int brightness = get_current_brightness();
  brightness_slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, brightness_min, brightness_max, 1);
  gtk_range_set_value(GTK_RANGE(brightness_slider), brightness);
  gtk_widget_set_hexpand(brightness_slider, TRUE);
  gtk_box_pack_start(GTK_BOX(brightness_box), brightness_slider, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(brightness_slider), "value-changed", G_CALLBACK(adjust_brightness), NULL);

  GtkWidget *brightness_steps_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(brightness_steps_box), 10);
  gtk_container_add(GTK_CONTAINER(brightness_box), brightness_steps_box);

  GtkWidget *button_p20 = gtk_button_new_with_label("20%");
  gtk_widget_set_name(button_p20, "p20");
  gtk_box_pack_start(GTK_BOX(brightness_steps_box), button_p20, TRUE, TRUE, 0);
  g_signal_connect(button_p20, "clicked", G_CALLBACK(brightness_button_clicked), GUINT_TO_POINTER(20));
  
  GtkWidget *button_p40 = gtk_button_new_with_label("40%");
  gtk_widget_set_name(button_p40, "p40");
  gtk_box_pack_start(GTK_BOX(brightness_steps_box), button_p40, TRUE, TRUE, 0);
  g_signal_connect(button_p40, "clicked", G_CALLBACK(brightness_button_clicked), GUINT_TO_POINTER(40));
  
  GtkWidget *button_p60 = gtk_button_new_with_label("60%");
  gtk_widget_set_name(button_p60, "p60");
  gtk_box_pack_start(GTK_BOX(brightness_steps_box), button_p60, TRUE, TRUE, 0);
  g_signal_connect(button_p60, "clicked", G_CALLBACK(brightness_button_clicked), GUINT_TO_POINTER(60));
  
  GtkWidget *button_p80 = gtk_button_new_with_label("80%");
  gtk_widget_set_name(button_p80, "p80");
  gtk_box_pack_start(GTK_BOX(brightness_steps_box), button_p80, TRUE, TRUE, 0);
  g_signal_connect(button_p80, "clicked", G_CALLBACK(brightness_button_clicked), GUINT_TO_POINTER(80));

  GtkWidget *button_p100 = gtk_button_new_with_label("100%");
  gtk_widget_set_name(button_p100, "p100");
  gtk_box_pack_start(GTK_BOX(brightness_steps_box), button_p100, TRUE, TRUE, 0);
  g_signal_connect(button_p100, "clicked", G_CALLBACK(brightness_button_clicked), GUINT_TO_POINTER(100));

  // test
  GtkWidget *test_frame = gtk_frame_new("Test");
  gtk_frame_set_label_align(GTK_FRAME(test_frame), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(screen_tab), test_frame, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(test_frame), 10);

  GtkWidget *test_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(test_box), 10);
  gtk_container_add(GTK_CONTAINER(test_frame), test_box);

  GtkWidget *white_button = gtk_button_new_with_label("████");
  gtk_widget_set_name(white_button, "white");
  gtk_box_pack_start(GTK_BOX(test_box), white_button, FALSE, FALSE, 0);
  g_signal_connect(white_button, "clicked", G_CALLBACK(test_button_clicked), TEST_WHITE_CSS);
  
  GtkWidget *black_button = gtk_button_new_with_label("████");
  gtk_widget_set_name(black_button, "black");
  gtk_box_pack_start(GTK_BOX(test_box), black_button, FALSE, FALSE, 0);
  g_signal_connect(black_button, "clicked", G_CALLBACK(test_button_clicked), TEST_BLACK_CSS);
  
  GtkWidget *red_button = gtk_button_new_with_label("████");
  gtk_widget_set_name(red_button, "red");
  gtk_box_pack_start(GTK_BOX(test_box), red_button, FALSE, FALSE, 0);
  g_signal_connect(red_button, "clicked", G_CALLBACK(test_button_clicked), TEST_RED_CSS);
  
  GtkWidget *green_button = gtk_button_new_with_label("████");
  gtk_widget_set_name(green_button, "green");
  gtk_box_pack_start(GTK_BOX(test_box), green_button, FALSE, FALSE, 0);
  g_signal_connect(green_button, "clicked", G_CALLBACK(test_button_clicked), TEST_GREEN_CSS);
    
  GtkWidget *blue_button = gtk_button_new_with_label("████");
  gtk_widget_set_name(blue_button, "blue");
  gtk_box_pack_start(GTK_BOX(test_box), blue_button, FALSE, FALSE, 0);
  g_signal_connect(blue_button, "clicked", G_CALLBACK(test_button_clicked), TEST_BLUE_CSS);
  
  GtkWidget *auto_button = gtk_button_new_with_label("Auto");
  gtk_box_pack_start(GTK_BOX(test_box), auto_button, FALSE, FALSE, 0);
  g_signal_connect(auto_button, "clicked", G_CALLBACK(auto_button_clicked), NULL);
  
  // Touch Buttons tab
  GtkWidget *buttons_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buttons_tab, gtk_label_new("Touch Buttons"));
  
  
  // left button
  create_button_action_editor(&left_button_action_editor, buttons_tab, "Button ☰");
  
  // center button
  create_button_action_editor(&center_button_action_editor, buttons_tab, "Button ○");
  
  // right button
  create_button_action_editor(&right_button_action_editor, buttons_tab, "Button ᐸ");
  
  // load the button actions
  load_button_actions();
  

  g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}
