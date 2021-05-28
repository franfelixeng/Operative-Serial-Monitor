#ifndef HELPER_H
#define HELPER_H

#include <gtk/gtk.h>


struct CssFont
{
    char* size;
    char* style;
    char* famyly;
    char* weight;
};

int refresh_ports(GtkComboBoxText *cbt, GtkEntry *entry);
int get_data_font(char* text_font, struct CssFont* cssFont);
void css_set(GtkCssProvider *cssProvider, GtkWidget *g_widgete);
void error(char *msg);

#endif