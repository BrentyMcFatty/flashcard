//
// Created by brent on 28/08/2026.
//

#include "manager.h"
#include "states.h"
#include "util.h"
#include <gtk/gtk.h>

static void on_add_card_clicked(GtkButton *button, gpointer data) {
    (void)button;

    ManageState *ms = data;

    const char *question =
        gtk_editable_get_text(GTK_EDITABLE(ms->question_entry));

    const char *answer =
        gtk_editable_get_text(GTK_EDITABLE(ms->answer_entry));

    if (!*question || !*answer) {
        show_message(
            GTK_WINDOW(ms->window),
            "Please enter both a question and an answer.");
        return;
    }

    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO flashcards(question, answer) "
        "VALUES (?, ?)";

    if (sqlite3_prepare_v2(
            ms->s->db,
            sql,
            -1,
            &stmt,
            NULL) != SQLITE_OK) {
        show_message(
            GTK_WINDOW(ms->window),
            "Could not prepare database query.");
        return;
    }

    sqlite3_bind_text(
        stmt, 1, question, -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt, 2, answer, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        show_message(
            GTK_WINDOW(ms->window),
            "Could not save the flashcard.");
    }

    sqlite3_finalize(stmt);

    gtk_editable_set_text(
        GTK_EDITABLE(ms->question_entry), "");

    gtk_editable_set_text(
        GTK_EDITABLE(ms->answer_entry), "");
}

static void close_manager(GtkButton *button, gpointer data) {
    (void)button;

    ManageState* ms = data;

    ms->s->show_flashcard_view(ms->s);

    gtk_window_destroy(GTK_WINDOW(ms->window));
}

static GtkWidget *create_add_box(ManageState* ms) {
    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);

    GtkWidget *title =
        gtk_label_new("Add a flashcard");

    gtk_widget_add_css_class(title, "title-2");

    gtk_box_append(GTK_BOX(box), title);

    ms->question_entry =
        gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(ms->question_entry),
        "Question");

    gtk_box_append(GTK_BOX(box), ms->question_entry);

    ms->answer_entry =
        gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(ms->answer_entry),
        "Answer");

    gtk_box_append(GTK_BOX(box), ms->answer_entry);

    GtkWidget *add =
        gtk_button_new_with_label("Add flashcard");

    gtk_box_append(GTK_BOX(box), add);

    g_signal_connect(
        add,
        "clicked",
        G_CALLBACK(on_add_card_clicked),
        ms);

    return box;
}

void show_manager(AppState *s) {
    ManageState *ms = g_new(ManageState, 1);
    ms->s = s;

    ms->window = gtk_window_new();

    gtk_window_set_title(
        GTK_WINDOW(ms->window),
        "Manage Flashcards");

    gtk_window_set_default_size(
        GTK_WINDOW(ms->window),
        500,
        300);

    gtk_window_set_transient_for(
        GTK_WINDOW(ms->window),
        GTK_WINDOW(s->window));

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);

    GtkWidget *add_box = create_add_box(ms);

    gtk_box_append(GTK_BOX(box), add_box);

    GtkWidget *close =
        gtk_button_new_with_label("Close");

    gtk_box_append(GTK_BOX(box), close);

    gtk_window_set_child(
        GTK_WINDOW(ms->window),
        box);

    g_object_set_data(
        G_OBJECT(ms->window),
        "app-state",
        s);

    g_signal_connect(
        close,
        "clicked",
        G_CALLBACK(close_manager),
        ms);

    gtk_window_present(GTK_WINDOW(ms->window));
}
