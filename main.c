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
GtkScrolledWindow *scrolledw_screen;
GtkAdjustment *scrol_adjustment_screen;
GtkActionBar *ab_configs;
GtkActionBar *ab_send;
GtkBuilder *builder;

GtkCssProvider *provid_font;
GtkCssProvider *provid_color;

gboolean flag_auto_scroll = False;

int fd = -1;
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

    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(buffer_screen, &iter);
    gtk_text_buffer_insert(buffer_screen, &iter, (char *)data, -1);

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

void *thread_reading(void *arg)
{
    char *text;
    int n;
    for (;;)
    {
        text = serialport_read(fd);

        if (text == NULL) // error or press disconnect
        {
            if (fd != -1)
            {
                serialport_close(fd);
                fd = -1;
            }
            //TODO put message
            printf("disconnected\n");
            ref_reading = 0;
            g_idle_add(on_disconnect, NULL);
            break;
        }
        if (text[0] == '\0') //disconnected board
        {
            if (fd != -1)
            {
                serialport_close(fd);
                fd = -1;
            }
            printf("disconnected\n");
            ref_reading = 0;
            g_idle_add(on_disconnect, NULL);
            break;
        }
        else
        {
            g_idle_add(on_read_usb, text);
        }
    }
}

void on_cb_auto_scroll_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    flag_auto_scroll = gtk_toggle_button_get_active(togglebutton);
}

void on_tv_send_by_return()
{
    GtkTextBuffer *tb;
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    char *text;
    if (fd > 0)
    {
        tb = gtk_text_view_get_buffer(tv_send);
        gtk_text_buffer_get_start_iter(tb, &iter_start);
        gtk_text_buffer_get_end_iter(tb, &iter_end);
        text = gtk_text_buffer_get_text(tb, &iter_start, &iter_end, True);
        serialport_write(fd, text);
        gtk_text_buffer_set_text(tb, "", 0);
    }
}

void on_button_send_clicked()
{
    GtkTextBuffer *tb;
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    char *text;
    if (fd > 0)
    {
        tb = gtk_text_view_get_buffer(tv_send);
        gtk_text_buffer_get_start_iter(tb, &iter_start);
        gtk_text_buffer_get_end_iter(tb, &iter_end);
        text = gtk_text_buffer_get_text(tb, &iter_start, &iter_end, True);
        serialport_write(fd, text);
        gtk_text_buffer_set_text(tb, "", 0);
    }
}

void on_button_connect_clicked(GtkButton *button, gpointer user_data)
{
    if (fd == -1)
    {
        GtkEntry *entry = (GtkEntry *)user_data;

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

        fd = serialport_init(serial_port, baud);
        if (fd == -1)
        {
            error("couldn't open port");
            return;
        }

        printf("opened port %s\n", serial_port);
        gtk_button_set_image(button_connect, im_disconnect);
        gtk_button_set_label(button_connect, "connected");

        pthread_create(&ref_reading, NULL, thread_reading, NULL);
    }
    else if (fd > 0)
    {
        pthread_cancel(ref_reading);

        if (serialport_close(fd) != 0)
        {
            error("don't close");
        }
        fd = -1;
        gtk_combo_box_text_remove_all(cbt_ports);
        gtk_button_set_image(button_connect, im_connect);
        gtk_button_set_label(button_connect, "connect");
    }
}

void on_fb_screen_font_set(GtkFontButton *font_button, gpointer data)
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
    css_set(provid_font, (GtkWidget *)tv_screen);
}

void on_cbutton_font_color_set(GtkColorButton *cbutton, gpointer user_data)
{
    gchar cssText[100] = {0};
    GtkColorChooser *colorChooser = GTK_COLOR_CHOOSER(cbutton);

    GdkRGBA rgba;

    gtk_color_chooser_get_rgba(colorChooser, &rgba);

    sprintf(cssText, ".TextScreen text{color:rgba(%.1lf%%,%.1lf%%,%.1lf%%,%.1lf);}",
            rgba.red*100, rgba.green*100, rgba.blue*100, rgba.alpha);

    printf("%s\n", cssText);
    gtk_css_provider_load_from_data(provid_color, cssText, -1, NULL);
    css_set(provid_color, (GtkWidget *)tv_screen);
}

void on_entry_port_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos,
                              GdkEvent *event, gpointer user_data)
{
    if (icon_pos == GTK_ENTRY_ICON_PRIMARY)
    {
        refresh_ports(cbt_ports, entry);
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

    scrolledw_screen = (GtkScrolledWindow *)GTK_WIDGET(gtk_builder_get_object(builder, "scrolledw_screen"));
    scrol_adjustment_screen = (GtkAdjustment *)gtk_scrolled_window_get_vadjustment(scrolledw_screen);

    tv_screen = (GtkTextView *)GTK_WIDGET(gtk_builder_get_object(builder, "tv_screen"));
    tv_send = (GtkTextView *)GTK_WIDGET(gtk_builder_get_object(builder, "tv_send"));
    buffer_screen = gtk_text_view_get_buffer(tv_screen);

    cbt_ports = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_ports"));
    cbt_baudrate = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_baud_rate"));
    button_connect = (GtkButton *)GTK_WIDGET(gtk_builder_get_object(builder, "button_connect"));

    ab_configs = (GtkActionBar *)GTK_WIDGET(gtk_builder_get_object(builder, "ab_configs"));
    ab_send = (GtkActionBar *)GTK_WIDGET(gtk_builder_get_object(builder, "ab_send"));

    im_connect = GTK_WIDGET(gtk_builder_get_object(builder, "im_connect"));
    im_disconnect = GTK_WIDGET(gtk_builder_get_object(builder, "im_disconnect"));

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
