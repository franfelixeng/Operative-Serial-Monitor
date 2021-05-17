#include <stdio.h> // Standard input/output definitions
#include <stdlib.h>
#include <string.h> // String function definitions
#include <unistd.h> // for usleep()
#include <signal.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>

#include "arduino-serial-lib.h"

GtkWidget *window;
GtkWidget *im_connect;
GtkWidget *im_disconnect;

GtkComboBoxText *cbt_ports;
GtkComboBoxText *cbt_baudrate;
GtkButton *button_connect;
GtkBuilder *builder;

int fd = -1;
char *port = NULL;
int baudrate = 0;

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

void connection_made()
{
    gtk_button_set_image(button_connect, im_disconnect);
    gtk_button_set_label(button_connect, "connected");

    if (fork() == 0)
    {
        char c;
        while (1)
        {
            c = (char)serialport_read(fd);
            printf("%c", c);
        }
    }
}


void* runRead_theread(void* arg)
{
    
}
void on_button_connect_clicked()
{

    char serialPort[21] = "/dev/";
    pid_t child_connect;
    if (port != NULL && baudrate != 0)
    {
        if ((child_connect = fork()) == 0)
        {
            fd = serialport_init(strncat(serialPort, port, 20), baudrate);
            if (fd == -1)
            {
                error("couldn't open port");
                return;
            }
            printf("opened port %s\n", serialPort);
            serialport_flush(fd);
            exit(0);
        }
        else if (child_connect < 0)
        {
            perror("fork");
            exit(1);
        }
        g_child_watch_add(child_connect, connection_made, NULL);
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

int main(int argc, char *argv[])
{

    //gtk_init(&argc, &argv);
    gtk_init(NULL, NULL);
    const int buf_max = 256;
    pid_t fistfork;

    char serialport[buf_max];

    char *port_selected;

    builder = gtk_builder_new_from_file("glade/SerialUI.glade");

    window = GTK_WIDGET(gtk_builder_get_object(builder, "w_fist"));

    cbt_ports = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_ports"));
    cbt_baudrate = (GtkComboBoxText *)GTK_WIDGET(gtk_builder_get_object(builder, "cbt_baud_rate"));
    button_connect = (GtkButton *)GTK_WIDGET(gtk_builder_get_object(builder, "button_connect"));

    im_connect = GTK_WIDGET(gtk_builder_get_object(builder, "im_connect"));
    im_disconnect = GTK_WIDGET(gtk_builder_get_object(builder, "im_disconnect"));

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_builder_connect_signals(builder, NULL);

    refresh_ports();

    gtk_widget_show(window);

    gtk_main();

    return EXIT_SUCCESS;

    return 0;
}