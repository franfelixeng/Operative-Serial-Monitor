#include "helper.h"
#include "arduino-serial-lib.h"
#include <string.h>
#include "stdint.h"
#include <stdio.h>

int refresh_ports(GtkComboBoxText *cbt, GtkEntry *entry)
{
    int n;
    char *ports[10];
    const char *text;
    uint8_t isMenber = 0;

    gtk_combo_box_text_remove_all(cbt);

    n = serialport_devices_ports(ports, 10);

    if (n > 0)
    {
        if (entry != NULL)
        {
            text = gtk_entry_get_text(entry);
        }

        for (int i = 0; i < n; i++)
        {
            if (entry != NULL)
            {
                if (strcmp(text, ports[i]) == 0)
                {
                    isMenber = 1;
                }
            }

            gtk_combo_box_text_append_text(cbt, ports[i]);
            free(ports[i]);
        }
    }
    if (!isMenber && entry != NULL)
    {
        gtk_entry_set_text(entry, "");
    }
    return n;
}

uint8_t isStyle(char *text)
{
    if (strcmp(text, "Italic") == 0)
    {
        return 1;
    }

    if (strcmp(text, "Oblique") == 0)
    {
        return 1;
    }

    return 0;
}

uint8_t isWeight(char *text)
{
    if (strcmp(text, "Bold") == 0)
    {
        return 1;
    }

    if (strcmp(text, "Semi-Bold") == 0)
    {
        strcpy(text, "600");
        return 1;
    }

    if (strcmp(text, "Lighter") == 0)
    {
        return 1;
    }

    if (strcmp(text, "Light") == 0)
    {
        strcpy(text, "300");
        return 1;
    }

    return 0;
}

void cssAdjustFontFamyli(char *text)
{
    int j = strlen(text) - 1;
    if (text[j] == ',')
    {
        text[j] = '\0';
    }
}
int get_data_font(char *text_font, struct CssFont *cssFont)
{
    char *position;

    cssFont->style = NULL;
    cssFont->weight = NULL;

    position = strrchr(text_font, ' ');
    cssFont->famyly = text_font;

    if (position == NULL)
    {   
        return 0;
    }
    *position = '\0';

    cssFont->size = position + 1;
    position = strrchr(text_font, ' ');

    if (position == NULL)
    {
        cssAdjustFontFamyli(text_font);
        return 2;
    }

    if (isStyle(position + 1))
    {
        cssFont->style = position + 1;
        *position = '\0';

        position = strrchr(text_font, ' ');
        if (position != NULL)
        {
            if (isWeight(position + 1))
            {
                cssFont->weight = position + 1;
                *position = '\0';

                cssAdjustFontFamyli(text_font);
                return 4;
            }
        }
    }
    else if (isWeight(position + 1))
    {
        cssFont->weight = position + 1;
        *position = '\0';
        cssAdjustFontFamyli(text_font);
        return 3;
    }

    cssAdjustFontFamyli(text_font);

    return 2;
}


void css_set(GtkCssProvider *cssProvider, GtkWidget *g_widgete)
{
    GtkStyleContext *context = gtk_widget_get_style_context(g_widgete);

    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(cssProvider),
                                   GTK_STYLE_PROVIDER_PRIORITY_USER);
}


void error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}