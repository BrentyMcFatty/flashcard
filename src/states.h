//
// Created by brent on 28/08/2026.
//

#ifndef GTKFLASHCARDS_STATES_H
#define GTKFLASHCARDS_STATES_H

#include <gtk/gtk.h>
#include <sqlite3.h>

typedef struct AppState {
    GtkApplication *app;
    sqlite3 *db;

    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *content_box_flashcard;
    GtkWidget *content_box_empty;
    GtkWidget *question_label;
    GtkWidget *answer_label;
    GtkWidget *reveal_button;

    gboolean answer_visible;
    int current_id;

    guint startup_timer;

    void (*show_flashcard_view)(struct AppState *self);
} AppState;

typedef enum {
    ID,
    QUESTION,
    ANSWER
} SortColumn;

typedef enum {
    SORT_ASC,
    SORT_DESC
} SortDirection;

typedef struct {
    AppState *s;

    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *content_box_adding;
    GtkWidget *content_box_overview;

    GtkWidget *question_entry;
    GtkWidget *answer_entry;

    // overview specific
    gint page;
    gint page_size;

    SortColumn sort_column;
    SortDirection sort_direction;

    gint total_rows;

    GtkListView *list_view;
    GtkLabel *page_label;

    GtkSingleSelection *selection_model;
} ManageState;

#endif // GTKFLASHCARDS_STATES_H
