#include <stdio.h> // Standard input/output definitions
#include <stdlib.h>
#include <string.h> // String function definitions
#include <unistd.h> // for usleep()
#include <signal.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <pthread.h>
#include <fcntl.h>
#include <locale.h>
#include "arduino-serial-lib.h"
#include "helper.h"

GtkWidget *window;
GtkWidget *im_connect;
GtkWidget *im_disconnect;
GtkComboBoxText *cbt_ports;
GtkComboBoxText *cbt_baudrate;
GtkButton *button_connect;
GtkTextView *tv_screen;
GtkTextView *tv_send;
GtkTextBuffer *buffer_screen;
GtkAdjustment *scrol_adjustment_screen;
GtkBuilder *builder;
GtkCssProvider *provid_font;
GtkCssProvider *provid_color;

gboolean flag_auto_scroll = False;
gboolean flag_log_color = False;
int fd_port = -1;
int baudrate = 0;
pthread_t ref_reading;

// run in gtk_main() environment
gboolean actualize_scrol(gpointer data)
{
    double value;
    value = gtk_adjustment_get_upper(scrol_adjustment_screen);
    gtk_adjustment_set_value(scrol_adjustment_screen, value);
    return False;
}

gboolean on_read_usb(gpointer data)
{
    struct TextToPrint *to_print = (struct TextToPrint *)data;
    GtkTextMark *mark;
    GtkTextIter iter;
    char log_color[15] = {0};

    gtk_text_buffer_get_end_iter(buffer_screen, &iter);

    if (to_print->color == NULL)
    {
        gtk_text_buffer_insert(buffer_screen, &iter, to_print->text, -1);
        free(to_print->text);
        free(to_print);
    }
    else
    {
        if (strcmp(to_print->color, LOG_COLOR_RED) == 0)
        {
            strcpy(log_color, "log-red");
        }
        else if (strcmp(to_print->color, LOG_COLOR_YELLOW) == 0)
        {
            strcpy(log_color, "log-yellow");
        }
        else if (strcmp(to_print->color, LOG_COLOR_GREEN) == 0)
        {
            strcpy(log_color, "log-green");
        }
        else if (strcmp(to_print->color, LOG_COLOR_CYAN) == 0)
        {
            strcpy(log_color, "log-cyan");
        }
        else if (strcmp(to_print->color, LOG_COLOR_GRAY) == 0)
        {
            strcpy(log_color, "log-gray");
        }

        gtk_text_buffer_insert_with_tags_by_name(buffer_screen, &iter, to_print->text,
                                                 -1, log_color, NULL);

        free(to_print->text);
        free(to_print->color);
        free(to_print);
    }

    if (flag_auto_scroll)
    {
        g_idle_add(actualize_scrol, NULL);
    }

    return False;
}

gboolean on_disconnect(gpointer data)
{
    gtk_button_set_image(button_connect, im_connect);
    gtk_button_set_label(button_connect, "connect");
}

gboolean treat_to_print(char *text, char *color, gboolean started)
{
    char *init_color;
    char *end_color;
    size_t size;
    struct TextToPrint *to_print;
    if (started)
    {
        if (strlen(text) == 0)
        {
            return True;
        }

        to_print = malloc(sizeof(struct TextToPrint));
        to_print->text = NULL;
        to_print->color = NULL;

        if ((end_color = strstr(text, LOG_RESET_COLOR)) != NULL)
        {

            size = end_color - text;
            to_print->text = calloc((size + 1), sizeof(char));
            to_print->color = malloc(3 * sizeof(char));
            strncpy(to_print->text, text, size);
            strcpy(to_print->color, color);
            g_idle_add(on_read_usb, to_print);
            return treat_to_print(end_color + strlen(LOG_RESET_COLOR), color, False);
        }
        else
        {
            size = strlen(text);
            to_print->text = calloc((size + 1), sizeof(char));
            to_print->color = malloc(3 * sizeof(char));
            strncpy(to_print->text, text, size);
            strcpy(to_print->color, color);
            g_idle_add(on_read_usb, to_print);
            return True;
        }
    }
    else
    {

        if (strlen(text) == 0)
        {
            return False;
        }

        if ((init_color = strstr(text, LOG_COLOR_INIT)) != NULL)
        {
            if ((size = init_color - text) == 0)
            {
                strncpy(color, init_color + strlen(LOG_COLOR_INIT), 2);
                return treat_to_print(strchr(init_color + 1, 'm') + 1, color, True);
            }
            to_print = malloc(sizeof(struct TextToPrint));
            to_print->color = NULL;
            to_print->text = calloc((size + 1), sizeof(char));
            strncpy(to_print->text, text, size);
            g_idle_add(on_read_usb, to_print);
            return treat_to_print(init_color, color, False);
        }
        else
        {
            to_print = malloc(sizeof(struct TextToPrint));
            to_print->color = NULL;
            size = strlen(text);
            to_print->text = calloc((size + 1), sizeof(char));
            strncpy(to_print->text, text, size);
            g_idle_add(on_read_usb, to_print);
            return False;
        }
    }
}

void *thread_reading(void *arg)
{

    struct TextToPrint *to_print;

    char *text;
    char *complete_text;
    int n;
    char *init_color;
    char *end_color;
    char color[3] = {0};
    int size;
    gboolean started_color = False;
    char *dup2;

    for (;;)
    {
        text = serialport_read(fd_port);

        if (text == NULL) // error or press disconnect
        {
            if (fd_port != -1)
            {
                serialport_close(fd_port);
                fd_port = -1;
            }
            printf("disconnected\n");
            ref_reading = 0;
            g_idle_add(on_disconnect, NULL);
            break;
        }
        if (text[0] == '\0') //disconnected board
        {
            if (fd_port != -1)
            {
                serialport_close(fd_port);
                fd_port = -1;
            }
            printf("disconnected\n");
            ref_reading = 0;
            g_idle_add(on_disconnect, NULL);
            break;
        }
        else
        {

            if (flag_log_color)
            {

                started_color = treat_to_print(text, color, started_color);

            }
            else
            {
                to_print = malloc(sizeof(struct TextToPrint));
                to_print->text = NULL;
                to_print->color = NULL;
                to_print->text = calloc(strlen(text) + 1, sizeof(char));
                strcpy(to_print->text, text);
                g_idle_add(on_read_usb, to_print);
            }
        }
    }
}

void on_cb_auto_scroll_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    flag_auto_scroll = gtk_toggle_button_get_active(togglebutton);
}

void on_cb_log_color_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    flag_log_color = gtk_toggle_button_get_active(togglebutton);
    serialport_color_mode(flag_log_color);
}

void on_tv_send_by_return()
{
    GtkTextBuffer *tb;
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    char *text;
    if (fd_port > 0)
    {
        tb = gtk_text_view_get_buffer(tv_send);
        gtk_text_buffer_get_start_iter(tb, &iter_start);
        gtk_text_buffer_get_end_iter(tb, &iter_end);
        text = gtk_text_buffer_get_text(tb, &iter_start, &iter_end, True);
        serialport_write(fd_port, text);
        gtk_text_buffer_set_text(tb, "", 0);
    }
}

void on_button_send_clicked(GtkButton *button, gpointer tv_send_data)
{
    GtkTextBuffer *tb;
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    char *text;
    if (fd_port > 0)
    {
        tb = gtk_text_view_get_buffer((GtkTextView *)tv_send_data);
        gtk_text_buffer_get_start_iter(tb, &iter_start);
        gtk_text_buffer_get_end_iter(tb, &iter_end);
        text = gtk_text_buffer_get_text(tb, &iter_start, &iter_end, True);
        serialport_write(fd_port, text);
        gtk_text_buffer_delete(tb, &iter_start, &iter_end);
    }
}

void on_button_clear_clicked(GtkButton *button, gpointer tv_screen_data)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer((GtkTextView *)tv_screen_data);
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    gtk_text_buffer_get_start_iter(buffer, &iter_start);
    gtk_text_buffer_get_end_iter(buffer, &iter_end);

    gtk_text_buffer_delete(buffer, &iter_start, &iter_end);
}

void on_button_connect_clicked(GtkButton *button, gpointer cbt_port_entry)
{
    if (fd_port == -1)
    {
        GtkEntry *entry = (GtkEntry *)cbt_port_entry;

        char serial_port[30] = "/dev/";
        char *p = NULL;
        char *str_baud;
        int baud;

        refresh_ports(cbt_ports, entry);

        p = gtk_combo_box_text_get_active_text(cbt_ports);

        if (strcmp(p, "") == 0)
        {
            printf("No port\n");
            return;
        }

        str_baud = gtk_combo_box_text_get_active_text(cbt_baudrate);

        if (strcmp(str_baud, "") == 0)
        {
            printf("No baud\n");
            return;
        }

        strncat(serial_port, p, 20);
        baud = (int)strtol(str_baud, (char **)strchr(str_baud, ' '), 10);

        fd_port = serialport_init(serial_port, baud);
        if (fd_port == -1)
        {
            error("couldn't open port");
            return;
        }

        printf("opened port %s\n", serial_port);
        gtk_button_set_image(button_connect, im_disconnect);
        gtk_button_set_label(button_connect, "connected");

        pthread_create(&ref_reading, NULL, thread_reading, NULL);
    }
    else if (fd_port > 0)
    {
        pthread_cancel(ref_reading);

        if (serialport_close(fd_port) != 0)
        {
            error("don't close port");
        }
        fd_port = -1;
        gtk_button_set_image(button_connect, im_connect);
        gtk_button_set_label(button_connect, "connect");
    }
}

void on_fb_screen_font_set(GtkFontButton *font_button, gpointer tv_screen_data)
{
    const gchar *font;
    char f[60] = {0};
    gchar cssText[100] = {0};
    struct CssFont cssFont;
    char style[30] = "";
    char weight[30] = "";

    GtkFontChooser *chooser = GTK_FONT_CHOOSER(font_button);
    font = gtk_font_chooser_get_font(chooser);

    strcpy(f, font);

    int n = get_data_font(f, &cssFont);

    if (n < 2)
    {
        error("error getting font");
        return;
    }

    if (cssFont.style != NULL)
    {
        sprintf(style, "font-style:%s;", cssFont.style);
    }
    if (cssFont.weight != NULL)
    {
        sprintf(weight, "font-weight:%s;", cssFont.weight);
    }

    sprintf(cssText, ".TextScreen{font-size:%spt;font-family:%s;%s%s}",
            cssFont.size, cssFont.famyly, style, weight);

    printf("%s\n", cssText);

    gtk_css_provider_load_from_data(provid_font, cssText, -1, NULL);
    css_set(provid_font, (GtkWidget *)tv_screen_data);
}

void on_cbutton_font_color_set(GtkColorButton *cbutton, gpointer tv_screen_data)
{
    gchar cssText[100] = {0};
    GtkColorChooser *colorChooser = GTK_COLOR_CHOOSER(cbutton);

    GdkRGBA rgba;

    gtk_color_chooser_get_rgba(colorChooser, &rgba);

    sprintf(cssText, ".TextScreen text{color:rgba(%.1lf%%,%.1lf%%,%.1lf%%,%.1lf);}",
            rgba.red * 100, rgba.green * 100, rgba.blue * 100, rgba.alpha);

    printf("%s\n", cssText);
    gtk_css_provider_load_from_data(provid_color, cssText, -1, NULL);
    css_set(provid_color, (GtkWidget *)tv_screen_data);
}

void on_entry_port_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos,
                              GdkEvent *event, gpointer cbt_ports_data)
{
    if (icon_pos == GTK_ENTRY_ICON_PRIMARY)
    {
        refresh_ports((GtkComboBoxText *)cbt_ports_data, entry);
    }
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    setlocale(LC_NUMERIC, "en_US.utf8");
    const int buf_max = 256;
    pid_t fistfork;

    char serialport[buf_max];

    char *port_selected;

    builder = gtk_builder_new_from_file("glade/SerialUI.glade");
    GtkCssProvider *provider = gtk_css_provider_new();
    provid_font = gtk_css_provider_new();
    provid_color = gtk_css_provider_new();

    gtk_css_provider_load_from_path(GTK_CSS_PROVIDER(provider), "glade/styles.css", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "w_fist"));

    GtkScrolledWindow *scrolledw_screen = (GtkScrolledWindow *)GTK_WIDGET(gtk_builder_get_object(builder, "scrolledw_screen"));
    scrol_adjustment_screen = (GtkAdjustment *)gtk_scrolled_window_get_vadjustment(scrolledw_screen);

    tv_screen = (GtkTextView *)GTK_WIDGET(gtk_builder_get_object(builder, "tv_screen"));
    tv_send = (GtkTextView *)GTK_WIDGET(gtk_builder_get_object(builder, "tv_send"));

    cbt_ports = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_ports"));
    cbt_baudrate = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_baud_rate"));
    button_connect = (GtkButton *)GTK_WIDGET(gtk_builder_get_object(builder, "button_connect"));

    GtkActionBar *ab_configs = (GtkActionBar *)GTK_WIDGET(gtk_builder_get_object(builder, "ab_configs"));
    GtkActionBar *ab_send = (GtkActionBar *)GTK_WIDGET(gtk_builder_get_object(builder, "ab_send"));

    im_connect = GTK_WIDGET(gtk_builder_get_object(builder, "im_connect"));
    im_disconnect = GTK_WIDGET(gtk_builder_get_object(builder, "im_disconnect"));

    buffer_screen = gtk_text_view_get_buffer(tv_screen);

    gtk_text_buffer_create_tag(buffer_screen, "log-red", "foreground", "red", NULL);
    gtk_text_buffer_create_tag(buffer_screen, "log-yellow", "foreground", "#cccc00", NULL);
    gtk_text_buffer_create_tag(buffer_screen, "log-green", "foreground", "green", NULL);
    gtk_text_buffer_create_tag(buffer_screen, "log-cyan", "foreground", "#00cccc", NULL);
    gtk_text_buffer_create_tag(buffer_screen, "log-gray", "foreground", "gray", NULL);

    gtk_builder_connect_signals(builder, NULL);

    refresh_ports(cbt_ports, NULL);

    css_set(provider, (GtkWidget *)tv_screen);
    css_set(provider, (GtkWidget *)tv_send);
    css_set(provider, (GtkWidget *)ab_configs);
    css_set(provider, (GtkWidget *)ab_send);

    g_signal_new("serial-send", G_TYPE_FROM_INSTANCE(tv_send),
                 G_SIGNAL_ACTION, 0, NULL, NULL, NULL, G_TYPE_NONE, 0, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    g_signal_connect(tv_send, "serial-send", G_CALLBACK(on_tv_send_by_return), NULL);

    gtk_window_set_title((GtkWindow *)window, "USB Serial");
    gtk_widget_show(window);
    gtk_main();
    return EXIT_SUCCESS;
}
