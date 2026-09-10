//
// Created by brent on 28/08/2026.
//

#include <gtk/gtk.h>
#include "manager.h"
#include "../states.h"

#include "add.h"
#include "overview.h"

static void on_manager_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    ManageState *ms = data;
    g_free(ms);
}

static void close_manager(GtkButton *button, gpointer data) {
    (void)button;

    ManageState* ms = data;

    ms->s->show_flashcard_view(ms->s);

    gtk_window_destroy(GTK_WINDOW(ms->window));
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

    // Connect the "destroy" signal to free ms
    g_signal_connect(ms->window, "destroy", G_CALLBACK(on_manager_destroy), ms);

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
