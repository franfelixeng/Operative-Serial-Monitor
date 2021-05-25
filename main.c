#include <stdio.h> // Standard input/output definitions
#include <stdlib.h>
#include <string.h> // String function definitions
#include <unistd.h> // for usleep()
#include <signal.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <pthread.h>
#include <fcntl.h>

#include "arduino-serial-lib.h"

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

gboolean flag_auto_scroll = False;

int fd = -1;
char port[21] = "/dev/";
int baudrate = 0;
pthread_t ref_reading;

void error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

int refresh_ports()
{
    char *ports[10];
    int n;

    gtk_combo_box_text_remove_all(cbt_ports);

    n = serialport_devices_ports(ports, 10);

    for (int i = 0; i < n; i++)
    {
        gtk_combo_box_text_append_text(cbt_ports, ports[i]);
        free(ports[i]);
    }

    return n;
}

void on_cbt_ports_changed()
{
    char *p;
    p = gtk_combo_box_text_get_active_text(cbt_ports);
    strncat(port, p, 20);
}

void on_cbt_baud_rate_changed()
{
    char *str_baud;
    str_baud = gtk_combo_box_text_get_active_text(cbt_baudrate);
    baudrate = (int)strtol(str_baud, (char **)strchr(str_baud, ' '), 10);
}

// run in gtk_main() environment

gboolean actualize_scrol(gpointer data)
{   
    double value;
    value = gtk_adjustment_get_upper (scrol_adjustment_screen);
    gtk_adjustment_set_value(scrol_adjustment_screen, value);
    return False;
}

gboolean on_read_usb(gpointer data)
{
    
    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(buffer_screen, &iter, EOF);
    gtk_text_buffer_insert(buffer_screen, &iter, (char *)data, -1);
    
    if(flag_auto_scroll)
    {
        g_idle_add(actualize_scrol,NULL);
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
            refresh_ports();
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

void on_cb_auto_scroll_toggled(GtkToggleButton *togglebutton, gpointer  user_data)
{
   flag_auto_scroll = gtk_toggle_button_get_active(togglebutton);
}

void on_button_connect_clicked()
{
    if (fd == -1)
    {
        fd = serialport_init(port, baudrate);
        if (fd == -1)
        {
            error("couldn't open port");
            //TODO maybe show info
            return;
        }
        printf("opened port %s\n", port);
        gtk_button_set_image(button_connect, im_disconnect);
        gtk_button_set_label(button_connect, "connected");

        pthread_create(&ref_reading, NULL, thread_reading, NULL);
    }
    else if (fd != -1)
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

void on_entry_port_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos,
                              GdkEvent *event, gpointer user_data)
{
    int n;
    char *ports[10];
    const char *text;
    uint8_t isMenber = 0;
    if (icon_pos == GTK_ENTRY_ICON_PRIMARY)
    {
        gtk_combo_box_text_remove_all(cbt_ports);

        n = serialport_devices_ports(ports, 10);

        if (n > 0)
        {
            text = gtk_entry_get_text(entry);
            for (int i = 0; i < n; i++)
            {
                if (strcmp(text, ports[i]) == 0)
                {
                    isMenber = 1;
                }
                gtk_combo_box_text_append_text(cbt_ports, ports[i]);
                free(ports[i]);
            }
        }

        if (!isMenber)
        {
            gtk_entry_set_text(entry, "");
        }
    }
}

void css_set(GtkCssProvider *cssProvider, GtkWidget *g_widgete);

char *getRegistros_intervalo(int data_inicial, int data_final);

int getAnguloRegistro(char a);

int main(int argc, char *argv[])
{

    //gtk_init(&argc, &argv);
    gtk_init(NULL, NULL);
    const int buf_max = 256;
    pid_t fistfork;

    char serialport[buf_max];

    char *port_selected;

    builder = gtk_builder_new_from_file("glade/SerialUI.glade");
    GtkCssProvider *provider = gtk_css_provider_new();
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

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_builder_connect_signals(builder, NULL);
    refresh_ports();
    
    css_set(provider, (GtkWidget *)tv_screen);
    css_set(provider, (GtkWidget *)tv_send);
    css_set(provider, (GtkWidget *)ab_configs);
    css_set(provider, (GtkWidget *)ab_send);

    
    gtk_widget_show(window);
    gtk_main();
    return EXIT_SUCCESS;
}

void css_set(GtkCssProvider *cssProvider, GtkWidget *g_widgete)
{
    GtkStyleContext *context = gtk_widget_get_style_context(g_widgete);

    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
}