//
// Created by brent on 10/09/2026.
//

#include <gtk/gtk.h>
#include "overview.h"
#include "../states.h"
#include "../util.h"

#define ROW_TYPE_DATA (row_data_get_type())
G_DECLARE_FINAL_TYPE(RowData, row_data, ROW, DATA, GObject)

struct _RowData {
    GObject parent_instance;

    gint id;
    gchar *question;
    gchar *answer;
};

G_DEFINE_TYPE(RowData, row_data, G_TYPE_OBJECT)

static void row_data_finalize(GObject *object) {
    RowData *row = ROW_DATA(object);

    g_free(row->question);
    g_free(row->answer);

    G_OBJECT_CLASS(row_data_parent_class)->finalize(object);
}

static void row_data_class_init(RowDataClass *class) {
    GObjectClass *object_class = G_OBJECT_CLASS(class);
    object_class->finalize = row_data_finalize;
}

static void row_data_init(RowData *row) {
    (void)row;
}

static RowData *row_data_new(
    int id,
    const char *question,
    const char *answer)
{
    RowData *row = g_object_new(ROW_TYPE_DATA, NULL);

    row->id = id;
    row->question = g_strdup(question);
    row->answer = g_strdup(answer);

    return row;
}

static char *get_sort_column(SortColumn sort_type) {
    switch (sort_type) {
        case ID: return "id";
        case QUESTION: return "question";
        case ANSWER: return "answer";
        default:
            g_assert_not_reached();
    }
}

static void set_total_rows(ManageState *ms) {
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT COUNT(*) FROM flashcards";

    if (sqlite3_prepare_v2(ms->s->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        show_message(GTK_WINDOW(ms->window), "Could not read the flashcard database.");
        sqlite3_finalize(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        ms->total_rows = count;
    }
}

static char *get_sort_direction(SortDirection direction) {
    return direction == SORT_ASC ? "ASC" : "DESC";
}

static GListModel *create_model(ManageState *ms) {
    // --- Create the list store and populate it ---
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);

    // Add data from SQLite
    sqlite3_stmt *stmt = NULL;
    const char *sql = g_strdup_printf(
                      "SELECT "
                      "id, question, answer "
                      "FROM flashcards "
                      "ORDER BY %s %s "
                      "LIMIT ? OFFSET ?",
                      get_sort_column(ms->sort_column),
                      get_sort_direction(ms->sort_direction)
                      );

    if (sqlite3_prepare_v2(ms->s->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        show_message(GTK_WINDOW(ms->window), "Could not read the flashcard database.");
        sqlite3_finalize(stmt);
        return G_LIST_MODEL(store);
    }

    const gint offset = ms->page * ms->page_size;
    sqlite3_bind_int(stmt, 1, ms->page_size);
    sqlite3_bind_int(stmt, 2, offset);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *question = (const char *)sqlite3_column_text(stmt, 1);
        const char *answer = (const char *)sqlite3_column_text(stmt, 2);
        RowData *data = row_data_new(id, question, answer);

        g_list_store_append(store, data);

        g_object_unref(data);
    }
    sqlite3_finalize(stmt);

    g_print("Store items: %u\n",
        g_list_model_get_n_items(G_LIST_MODEL(store)));

    return G_LIST_MODEL(store);
}

static void
update_pagination_controls(ManageState *ms)
{
    char *page_number = g_strdup_printf("%d", ms->page + 1);
    gtk_label_set_text(GTK_LABEL(ms->page_label), page_number);
}

static void
reload_page(ManageState *ms)
{
    GListModel *model = create_model(
        ms
    );

    GtkSingleSelection *selection_model = gtk_single_selection_new(model);
    gtk_list_view_set_model(ms->list_view, GTK_SELECTION_MODEL(selection_model));
    update_pagination_controls(ms);
}

static void
sort_clicked(GtkButton *button, gpointer user_data)
{
    ManageState *ms = user_data;

    SortColumn column = GPOINTER_TO_INT(
        g_object_get_data(G_OBJECT(button), "sort-column")
    );

    if (ms->sort_column == column) {
        ms->sort_direction =
            ms->sort_direction == SORT_ASC
                ? SORT_DESC
                : SORT_ASC;
    } else {
        ms->sort_column = column;
        ms->sort_direction = SORT_ASC;
    }

    /* Sorting starts from the first page. */
    ms->page = 0;

    reload_page(ms);
}

static void
previous_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;

    ManageState *ms = user_data;

    if (ms->page - 1 >= 0) {
        ms->page--;
        reload_page(ms);
    }
}

static void
next_clicked(GtkButton *button, gpointer user_data)
{
    (void)button;

    ManageState *ms = user_data;

    gint total_pages =
        (ms->total_rows + ms->page_size - 1) /
        ms->page_size;

    if (ms->page + 1 < total_pages) {
        ms->page++;
        reload_page(ms);
    }
}

// Setup function for the factory
static void
setup_factory(GtkSignalListItemFactory *factory,
              GtkListItem *list_item)
{
    (void)factory;

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    GtkWidget *id_label = gtk_label_new("");
    GtkWidget *question_label = gtk_label_new("");
    GtkWidget *answer_label = gtk_label_new("");

    gtk_widget_set_size_request(id_label, 8, 8);
    gtk_widget_set_hexpand(id_label, FALSE);
    gtk_widget_set_hexpand(question_label, TRUE);
    gtk_widget_set_hexpand(answer_label, TRUE);

    gtk_label_set_xalign(GTK_LABEL(id_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(question_label), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(answer_label), 0.0f);

    gtk_grid_attach(GTK_GRID(grid), question_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), answer_label, 1, 0, 1, 1);

    g_object_set_data(G_OBJECT(hbox), "id", id_label);
    g_object_set_data(G_OBJECT(hbox), "question", question_label);
    g_object_set_data(G_OBJECT(hbox), "answer", answer_label);

    gtk_box_append(GTK_BOX(hbox), id_label);
    gtk_box_append(GTK_BOX(hbox), grid);

    gtk_list_item_set_child(list_item, hbox);
}

// Bind function for the factory
static void bind_factory(GtkSignalListItemFactory *factory, GtkListItem *list_item) {
    (void)factory;

    GtkWidget *hbox =
        gtk_list_item_get_child(list_item);

    RowData *row =
        ROW_DATA(gtk_list_item_get_item(list_item));

    GtkWidget *id_label =
        g_object_get_data(G_OBJECT(hbox), "id");

    GtkWidget *question_label =
        g_object_get_data(G_OBJECT(hbox), "question");

    GtkWidget *answer_label =
        g_object_get_data(G_OBJECT(hbox), "answer");

    char *id = g_strdup_printf("%d", row->id);

    gtk_label_set_text(GTK_LABEL(id_label), id);
    gtk_label_set_text(GTK_LABEL(question_label), row->question);
    gtk_label_set_text(GTK_LABEL(answer_label), row->answer);

    g_free(id);
}

static GtkWidget *create_question_view(ManageState *ms) {
    // Create a vertical box to hold the header and list view
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // --- Create the header row ---
    GtkWidget *header_grid = gtk_grid_new();
    gtk_widget_set_hexpand(header_grid, TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(header_grid), FALSE);

    GtkWidget *id_header = gtk_label_new("ID");
    GtkWidget *question_header = gtk_label_new("Question");
    GtkWidget *answer_header = gtk_label_new("Answer");

    gtk_widget_add_css_class(id_header, "bold");
    gtk_widget_add_css_class(question_header, "bold");
    gtk_widget_add_css_class(answer_header, "bold");

    gtk_widget_set_size_request(id_header, 8, 8);
    gtk_widget_set_hexpand(id_header, FALSE);
    gtk_widget_set_hexpand(question_header, TRUE);
    gtk_widget_set_hexpand(answer_header, TRUE);

    gtk_grid_attach(GTK_GRID(header_grid), id_header, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), question_header, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(header_grid), answer_header, 2, 0, 1, 1);

    // Add the header box to the vertical box
    gtk_box_append(GTK_BOX(vbox), header_grid);

    GListModel *model = create_model(ms);

    // --- Create the list view ---
    // Create the factory
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    g_signal_connect(factory, "setup", G_CALLBACK (setup_factory), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK (bind_factory), NULL);

    // Create a selection model
    GtkSingleSelection *selection_model = gtk_single_selection_new(model);
    gtk_single_selection_set_autoselect(selection_model, FALSE);

    // Create a column view
    GtkWidget *list_view = gtk_list_view_new(GTK_SELECTION_MODEL(selection_model), factory);
    ms->list_view = GTK_LIST_VIEW(list_view);

    // Add the list view to the vertical box
    gtk_box_append(GTK_BOX(vbox), list_view);

    // Set the list view to expand
    gtk_widget_set_hexpand(list_view, TRUE);
    gtk_widget_set_vexpand(list_view, TRUE);

    return vbox;
}

GtkWidget *create_overview_box(ManageState *ms) {
    ms->page = 0;
    ms->page_size = 5;
    ms->sort_column = ID;
    ms->sort_direction = SORT_ASC;
    set_total_rows(ms);

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_hexpand(box, TRUE);
    gtk_widget_set_vexpand(box, TRUE);

    GtkWidget *title =
        gtk_label_new("Overview");

    gtk_widget_add_css_class(title, "title-2");

    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *question_view = create_question_view(ms);
    gtk_widget_set_hexpand(question_view, TRUE);
    gtk_widget_set_vexpand(question_view, TRUE);

    gtk_box_append(GTK_BOX(box), question_view);

    GtkWidget *paging_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_hexpand(paging_box, TRUE);

    GtkWidget *previous_button = gtk_button_new_with_label("Previous");
    GtkWidget *next_button = gtk_button_new_with_label("Next");
    GtkWidget *page_label = gtk_label_new("1");
    gtk_widget_set_hexpand(previous_button, TRUE);
    gtk_widget_set_hexpand(next_button, TRUE);
    gtk_widget_set_hexpand(page_label, TRUE);

    ms->page_label = GTK_LABEL(page_label);

    g_signal_connect(
        previous_button,
        "clicked",
        G_CALLBACK(previous_clicked),
        ms
    );

    g_signal_connect(
        next_button,
        "clicked",
        G_CALLBACK(next_clicked),
        ms
    );

    gtk_box_append(GTK_BOX(paging_box), previous_button);
    gtk_box_append(GTK_BOX(paging_box), page_label);
    gtk_box_append(GTK_BOX(paging_box), next_button);

    gtk_box_append(GTK_BOX(box), paging_box);

    return box;
}
