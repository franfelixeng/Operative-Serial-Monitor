#include <stdio.h> // Standard input/output definitions
#include <stdlib.h>
#include <string.h> // String function definitions
#include <unistd.h> // for usleep()
#include <signal.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <pthread.h>

#include "arduino-serial-lib.h"

GtkWidget *window;
GtkWidget *im_connect;
GtkWidget *im_disconnect;

GtkComboBoxText *cbt_ports;
GtkComboBoxText *cbt_baudrate;
GtkButton *button_connect;
GtkTextView *tv_screen;
GtkTextBuffer *buffer_screen;
GtkBuilder *builder;

int fd = -1;
char *port = NULL;
int baudrate = 0;
guint id_idle_read;
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
    port = gtk_combo_box_text_get_active_text(cbt_ports);
}

void on_cbt_baud_rate_changed()
{
    char *str_baud;
    str_baud = gtk_combo_box_text_get_active_text(cbt_baudrate);
    baudrate = (int)strtol(str_baud, (char **)strchr(str_baud, ' '), 10);
}

// run in gtk_main() environment

char text[1024];
gboolean on_read_usb(gpointer data)
{
    char c[] = {'C', 'h', 'i', 'b', 'a', 't', 'o', 't', 'e', 'x', ' ', 'F', 'e', 'l', 'i', 'x', '\r', '\n', '\0'};
    //g_strdelimit(c,"\r\n",'\n');
    //gtk_text_buffer_insert_interactive_at_cursor(buffer_screen, c, -1, True);
    gtk_text_buffer_insert_at_cursor(buffer_screen, (gchar *)data, -1);

    return False;
}

void *thread_reading(void *arg)
{
    int i = 0;

    for (;;)
    {
        while ((text[i] = serialport_read(fd)) != '\0')
        {
            i++;
        }
        if (i > 0)
        {
            id_idle_read = g_idle_add(on_read_usb, text);
            i = 0;
        }
        usleep(100000);
    }
}

void on_button_connect_clicked()
{

    if (fd == -1)
    {
        char serialPort[21] = "/dev/";
        fd = serialport_init(strncat(serialPort, port, 20), baudrate);
        if (fd == -1)
        {
            error("couldn't open port");
            //TODO maybe show info
            return;
        }
        printf("opened port %s\n", serialPort);
        gtk_button_set_image(button_connect, im_disconnect);
        gtk_button_set_label(button_connect, "connected");

        pthread_create(&ref_reading, NULL, thread_reading, NULL);
    }
    else if (fd != -1)
    {
        // g_source_remove(id_idle_read);
        if (serialport_close(fd) != 0)
        {
            error("don't close");
        }
        gtk_button_set_image(button_connect, im_connect);
        gtk_button_set_label(button_connect, "connect");
        fd = -1;
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
            port = NULL;
        }
    }
}

void css_set(GtkCssProvider *cssProvider, GtkWidget *g_widgete);

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

    cbt_ports = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_ports"));
    cbt_baudrate = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_baud_rate"));
    button_connect = (GtkButton *)GTK_WIDGET(gtk_builder_get_object(builder, "button_connect"));
    tv_screen = (GtkTextView *)GTK_WIDGET(gtk_builder_get_object(builder, "tv_screen"));
    buffer_screen = gtk_text_view_get_buffer(tv_screen);
    im_connect = GTK_WIDGET(gtk_builder_get_object(builder, "im_connect"));
    im_disconnect = GTK_WIDGET(gtk_builder_get_object(builder, "im_disconnect"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_builder_connect_signals(builder, NULL);
    refresh_ports();
    css_set(provider, (GtkWidget *)tv_screen);
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