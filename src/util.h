//
// Created by brent on 28/08/2026.
//

#ifndef GTKFLASHCARDS_UTIL_H
#define GTKFLASHCARDS_UTIL_H

#include <gtk/gtk.h>

static void show_message(GtkWindow *parent, const char *message) {
    GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
    gtk_alert_dialog_show(dialog, parent);
    g_object_unref(dialog);
}

#endif //GTKFLASHCARDS_UTIL_H
