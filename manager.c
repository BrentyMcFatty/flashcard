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

static GtkWidget *create_overview_box(ManageState* ms) {
    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);

    GtkWidget *title =
        gtk_label_new("Overview");

    gtk_widget_add_css_class(title, "title-2");

    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_NEVER,      // No horizontal scrollbar
        GTK_POLICY_AUTOMATIC   // Vertical scrollbar appears when needed
    );

    gtk_box_append(GTK_BOX(box), scrolled_window);


    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);

    GtkWidget *id = gtk_label_new("<b>ID</b>");
    GtkWidget *question = gtk_label_new("<b>Question</b>");
    GtkWidget *answer = gtk_label_new("<b>Answer</b>");

    gtk_label_set_use_markup(GTK_LABEL(id), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(question), TRUE);
    gtk_label_set_use_markup(GTK_LABEL(answer), TRUE);

    gtk_grid_attach(GTK_GRID(grid), id, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), question, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), answer, 2, 0, 1, 1);


    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled_window),
        grid);


    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT id, question, answer "
        "FROM flashcards";

    if (sqlite3_prepare_v2(ms->s->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        show_message(GTK_WINDOW(ms->window), "Could not read the flashcard database.");
        return box;
    }

    int i = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        i++;

        int current_id = sqlite3_column_int(stmt, 0);
        gchar *id_str = g_strdup_printf("%d", current_id);
        GtkWidget *label1 = gtk_label_new(id_str);

        const char *question =
            (const char *)sqlite3_column_text(stmt, 1);
        GtkWidget *label2 = gtk_label_new(question);

        const char *answer =
            (const char *)sqlite3_column_text(stmt, 2);
        GtkWidget *label3 = gtk_label_new(answer);

        gtk_grid_attach(GTK_GRID(grid), label1, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), label2, 1, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), label3, 2, i, 1, 1);

        g_free(id_str);
    }

    return box;
}

static void on_adding_clicked(GtkButton *button, gpointer data) {
    (void)button;
    ManageState* ms = data;

    gtk_stack_set_visible_child(
        GTK_STACK(ms->stack),
        ms->content_box_adding);
}

static void on_overview_clicked(GtkButton *button, gpointer data) {
    (void)button;
    ManageState* ms = data;

    gtk_stack_set_visible_child(
        GTK_STACK(ms->stack),
        ms->content_box_overview);
}

static GtkWidget *create_button_grid(ManageState* ms) {
    GtkWidget *grid =
        gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);
    gtk_widget_set_valign(grid, GTK_ALIGN_END);

    GtkWidget *adding =
        gtk_button_new_with_label("Adding flashcards");
    gtk_widget_set_hexpand(adding, TRUE);

    GtkWidget *overview =
        gtk_button_new_with_label("Overview");
    gtk_widget_set_hexpand(overview, TRUE);

    GtkWidget *close =
        gtk_button_new_with_label("Close");
    gtk_widget_set_hexpand(close, TRUE);

    g_signal_connect(
        adding,
        "clicked",
        G_CALLBACK(on_adding_clicked),
        ms);

    g_signal_connect(
        overview,
        "clicked",
        G_CALLBACK(on_overview_clicked),
        ms);

    g_signal_connect(
        close,
        "clicked",
        G_CALLBACK(close_manager),
        ms);

    gtk_grid_attach(GTK_GRID(grid), adding, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), overview, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), close, 2, 0, 1, 1);

    return grid;
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

    GtkWidget *stack = gtk_stack_new();

    ms->stack = stack;

    GtkWidget *add_box = create_add_box(ms);
    GtkWidget *overview_box = create_overview_box(ms);

    ms->content_box_adding = add_box;
    ms->content_box_overview = overview_box;

    // set stack pages + select add page
    gtk_stack_add_child(
        GTK_STACK(ms->stack),
        add_box);

    gtk_stack_add_child(
        GTK_STACK(ms->stack),
        overview_box);

    gtk_stack_set_visible_child(
        GTK_STACK(ms->stack),
        add_box);

    gtk_box_append(GTK_BOX(box), stack);

    GtkWidget *buttons = create_button_grid(ms);

    gtk_box_append(GTK_BOX(box), buttons);

    gtk_window_set_child(
        GTK_WINDOW(ms->window),
        box);

    g_object_set_data(
        G_OBJECT(ms->window),
        "app-state",
        s);

    gtk_window_present(GTK_WINDOW(ms->window));
}
