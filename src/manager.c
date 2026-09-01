//
// Created by brent on 28/08/2026.
//

#include "manager.h"
#include "states.h"
#include "util.h"
#include <gtk/gtk.h>

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

static void on_manager_destroy(GtkWidget *widget, gpointer data) {
    (void)widget;
    ManageState *ms = data;
    g_free(ms);
}

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

static GListModel *create_model(ManageState *ms) {
    // --- Create the list store and populate it ---
    GListStore *store = g_list_store_new(G_TYPE_OBJECT);

    // Add data from SQLite
    sqlite3_stmt *stmt = NULL;
    const char *sql = "SELECT id, question, answer FROM flashcards";

    if (sqlite3_prepare_v2(ms->s->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        show_message(GTK_WINDOW(ms->window), "Could not read the flashcard database.");
        sqlite3_finalize(stmt);
        return G_LIST_MODEL(store);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const char *question = (const char *)sqlite3_column_text(stmt, 1);
        const char *answer = (const char *)sqlite3_column_text(stmt, 2);
        RowData *data = row_data_new(id, question, answer);

        g_list_store_append(store, data);

        g_object_unref(data);
    }
    sqlite3_finalize(stmt);

    return G_LIST_MODEL(store);
}

// Setup function for the factory
static void setup_factory(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory;
    (void)user_data;

    // Create a label for the cell
    GtkWidget *label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL (label), 0.0f); // Align left
    gtk_list_item_set_child(list_item, label);
}

// Bind function for the factory
static void bind_factory(GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
    (void)factory;

    const char *property_name = (const char *)user_data;
    GtkWidget *label = gtk_list_item_get_child (list_item);
    GObject *item = gtk_list_item_get_item (list_item);

    // Get the row data from the item
    RowData *row = ROW_DATA(item);

    // Set the label text based on the property name
    if (g_strcmp0 (property_name, "id") == 0) {
        char *text = g_strdup_printf ("%d", row->id);
        gtk_label_set_text (GTK_LABEL (label), text);
        g_free (text);
    } else if (g_strcmp0 (property_name, "question") == 0) {
        gtk_label_set_text (GTK_LABEL (label), row->question);
    } else if (g_strcmp0 (property_name, "answer") == 0) {
        gtk_label_set_text (GTK_LABEL (label), row->answer);
    }
}

// Function to create a column for the column view
static GtkColumnViewColumn *create_column(const char *title, const char *property_name) {
    // Create a factory for the cells in this column
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();

    GtkColumnViewColumn *column = gtk_column_view_column_new(title, factory);

    // Set up the factory to create and bind cells
    g_signal_connect(factory, "setup", G_CALLBACK (setup_factory), NULL);
    g_signal_connect(factory, "bind", G_CALLBACK (bind_factory), (gpointer)property_name);

    // Add the factory to the column
    gtk_column_view_column_set_factory(column, factory);

    return column;
}

static GtkWidget *create_question_view(ManageState *ms) {
    // Create a vertical box to hold the header and list view
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // --- Create the header row ---
    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_hexpand(header_box, TRUE);

    // Create header labels
    GtkWidget *id_header = gtk_label_new("ID");
    GtkWidget *question_header = gtk_label_new("Question");
    GtkWidget *answer_header = gtk_label_new("Answer");

    // Style the header labels (bold and left-aligned)
    gtk_label_set_xalign(GTK_LABEL(id_header), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(question_header), 0.0f);
    gtk_label_set_xalign(GTK_LABEL(answer_header), 0.0f);
    gtk_widget_add_css_class(id_header, "bold");
    gtk_widget_add_css_class(question_header, "bold");
    gtk_widget_add_css_class(answer_header, "bold");

    // Expand the header labels
    gtk_widget_set_hexpand(id_header, TRUE);
    gtk_widget_set_hexpand(question_header, TRUE);
    gtk_widget_set_hexpand(answer_header, TRUE);

    // Pack the header labels into the header box
    gtk_box_append(GTK_BOX(header_box), id_header);
    gtk_box_append(GTK_BOX(header_box), question_header);
    gtk_box_append(GTK_BOX(header_box), answer_header);

    // Add the header box to the vertical box
    gtk_box_append(GTK_BOX(vbox), header_box);

    GListModel *model = create_model(ms);

    // --- Create the list view ---
    // Create a selection model
    GtkSingleSelection *selection_model = gtk_single_selection_new(model);
    gtk_single_selection_set_autoselect(selection_model, FALSE);

    // Create a column view
    GtkWidget *column_view = gtk_column_view_new(GTK_SELECTION_MODEL(selection_model));

    // Add columns
    GtkColumnViewColumn *id_column = create_column("ID", "id");
    GtkColumnViewColumn *name_column = create_column("Question", "question");
    GtkColumnViewColumn *desc_column = create_column("Answer", "answer");

    gtk_column_view_column_set_expand(id_column, TRUE);
    gtk_column_view_column_set_expand(name_column, TRUE);
    gtk_column_view_column_set_expand(desc_column, TRUE);

    gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), id_column);
    gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), name_column);
    gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), desc_column);

    // Add the list view to the vertical box
    gtk_box_append(GTK_BOX(vbox), column_view);

    // Set the list view to expand
    gtk_widget_set_hexpand(column_view, TRUE);
    gtk_widget_set_vexpand(column_view, TRUE);

    return vbox;
}

static GtkWidget *create_overview_box(ManageState *ms) {
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

    GtkWidget *scrolled_window = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(
        GTK_SCROLLED_WINDOW(scrolled_window),
        GTK_POLICY_NEVER,      // No horizontal scrollbar
        GTK_POLICY_AUTOMATIC   // Vertical scrollbar appears when needed
    );

    GtkWidget *question_view = create_question_view(ms);
    gtk_widget_set_hexpand(question_view, TRUE);
    gtk_widget_set_vexpand(question_view, TRUE);

    gtk_scrolled_window_set_child(
        GTK_SCROLLED_WINDOW(scrolled_window),
        question_view);

    gtk_box_append(GTK_BOX(box), scrolled_window);

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
