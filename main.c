#include <gtk/gtk.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define APP_ID "com.example.GTKFlashcards"
#define DB_FILE "flashcards.db"
#define DEFAULT_DELAY_SECONDS 30

typedef struct {
    GtkApplication *app;
    sqlite3 *db;

    GtkWidget *window;
    GtkWidget *question_label;
    GtkWidget *answer_label;
    GtkWidget *reveal_button;

    gboolean answer_visible;
    int current_id;

    guint startup_timer;
} AppState;

static void free_signal_data(gpointer data, GClosure *closure)
{
    (void)closure;
    g_free(data);
}

static void show_message(GtkWindow *parent, const char *message) {
    GtkAlertDialog *dialog = gtk_alert_dialog_new("%s", message);
    gtk_alert_dialog_show(dialog, parent);
    g_object_unref(dialog);
}

static gboolean db_exec(AppState *s, const char *sql) {
    char *error = NULL;
    int rc = sqlite3_exec(s->db, sql, NULL, NULL, &error);

    if (rc != SQLITE_OK) {
        g_warning("SQLite error: %s", error ? error : "unknown error");
        sqlite3_free(error);
        return FALSE;
    }

    return TRUE;
}

static gboolean init_database(AppState *s) {
    const char *sql =
        "CREATE TABLE IF NOT EXISTS flashcards ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "question TEXT NOT NULL,"
        "answer TEXT NOT NULL"
        ");";

    return db_exec(s, sql);
}

static gboolean have_cards(AppState *s) {
    sqlite3_stmt *stmt = NULL;
    int count = 0;

    if (sqlite3_prepare_v2(
            s->db,
            "SELECT COUNT(*) FROM flashcards",
            -1,
            &stmt,
            NULL) != SQLITE_OK)
        return FALSE;

    if (sqlite3_step(stmt) == SQLITE_ROW)
        count = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);
    return count > 0;
}

static void set_card_text(AppState *s, const char *question, const char *answer) {
    gtk_label_set_text(GTK_LABEL(s->question_label), question);
    gtk_label_set_text(GTK_LABEL(s->answer_label), answer);

    gtk_widget_set_visible(s->question_label, TRUE);
    gtk_widget_set_visible(s->answer_label, TRUE);
}

static void load_random_card(AppState *s) {
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT id, question, answer "
        "FROM flashcards "
        "ORDER BY RANDOM() "
        "LIMIT 1";

    if (sqlite3_prepare_v2(s->db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        show_message(GTK_WINDOW(s->window), "Could not read the flashcard database.");
        return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        s->current_id = sqlite3_column_int(stmt, 0);

        const char *question =
            (const char *)sqlite3_column_text(stmt, 1);

        const char *answer =
            (const char *)sqlite3_column_text(stmt, 2);

        gtk_label_set_text(GTK_LABEL(s->question_label), question);
        gtk_label_set_text(GTK_LABEL(s->answer_label), answer);

        gtk_widget_set_visible(s->question_label, TRUE);
        gtk_widget_set_visible(s->reveal_button, TRUE);

        s->answer_visible = FALSE;
        gtk_widget_set_visible(s->answer_label, FALSE);
        gtk_button_set_label(
            GTK_BUTTON(s->reveal_button),
            "Reveal answer");

        gtk_window_present(GTK_WINDOW(s->window));
    }

    sqlite3_finalize(stmt);
}

static void on_reveal_clicked(GtkButton *button, gpointer data) {
    AppState *s = data;

    s->answer_visible = !s->answer_visible;

    gtk_widget_set_visible(
        s->answer_label,
        s->answer_visible);

    gtk_button_set_label(
        button,
        s->answer_visible ? "Hide answer" : "Reveal answer");
}

static void on_next_clicked(GtkButton *button, gpointer data) {
    (void)button;
    AppState *s = data;

    load_random_card(s);
}

static void on_close_clicked(GtkButton *button, gpointer data) {
    (void)button;
    AppState *s = data;

    gtk_window_minimize(GTK_WINDOW(s->window));
}

static void on_add_card_clicked(GtkButton *button, gpointer data) {
    (void)button;

    GtkWidget **widgets = data;

    GtkWidget *window = widgets[0];
    GtkWidget *question_entry = widgets[1];
    GtkWidget *answer_entry = widgets[2];

    AppState *s = g_object_get_data(
        G_OBJECT(window),
        "app-state");

    const char *question =
        gtk_editable_get_text(GTK_EDITABLE(question_entry));

    const char *answer =
        gtk_editable_get_text(GTK_EDITABLE(answer_entry));

    if (!*question || !*answer) {
        show_message(
            GTK_WINDOW(window),
            "Please enter both a question and an answer.");
        return;
    }

    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO flashcards(question, answer) "
        "VALUES (?, ?)";

    if (sqlite3_prepare_v2(
            s->db,
            sql,
            -1,
            &stmt,
            NULL) != SQLITE_OK) {
        show_message(
            GTK_WINDOW(window),
            "Could not prepare database query.");
        return;
    }

    sqlite3_bind_text(
        stmt, 1, question, -1, SQLITE_TRANSIENT);

    sqlite3_bind_text(
        stmt, 2, answer, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        show_message(
            GTK_WINDOW(window),
            "Could not save the flashcard.");
    }

    sqlite3_finalize(stmt);

    gtk_editable_set_text(
        GTK_EDITABLE(question_entry), "");

    gtk_editable_set_text(
        GTK_EDITABLE(answer_entry), "");
}

static void on_manage_clicked(GtkButton *button, gpointer data) {
    (void)button;

    AppState *s = data;

    GtkWidget *window = gtk_window_new();

    gtk_window_set_title(
        GTK_WINDOW(window),
        "Manage Flashcards");

    gtk_window_set_default_size(
        GTK_WINDOW(window),
        500,
        300);

    gtk_window_set_transient_for(
        GTK_WINDOW(window),
        GTK_WINDOW(s->window));

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

    GtkWidget *question =
        gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(question),
        "Question");

    gtk_box_append(GTK_BOX(box), question);

    GtkWidget *answer =
        gtk_entry_new();

    gtk_entry_set_placeholder_text(
        GTK_ENTRY(answer),
        "Answer");

    gtk_box_append(GTK_BOX(box), answer);

    GtkWidget *add =
        gtk_button_new_with_label("Add flashcard");

    gtk_box_append(GTK_BOX(box), add);

    GtkWidget *close =
        gtk_button_new_with_label("Close");

    gtk_box_append(GTK_BOX(box), close);

    gtk_window_set_child(
        GTK_WINDOW(window),
        box);

    g_object_set_data(
        G_OBJECT(window),
        "app-state",
        s);

    GtkWidget **data_array =
        g_new(GtkWidget *, 3);

    data_array[0] = window;
    data_array[1] = question;
    data_array[2] = answer;

    g_signal_connect_data(
        add,
        "clicked",
        G_CALLBACK(on_add_card_clicked),
        data_array,
        free_signal_data,
        0);

    g_signal_connect_swapped(
        close,
        "clicked",
        G_CALLBACK(gtk_window_destroy),
        window);

    gtk_window_present(GTK_WINDOW(window));
}

static GtkWidget *make_button_bar(AppState *s) {
    GtkWidget *bar =
        gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    GtkWidget *reveal =
        gtk_button_new_with_label("Reveal answer");

    GtkWidget *next =
        gtk_button_new_with_label("Next");

    GtkWidget *later =
        gtk_button_new_with_label("Later");

    GtkWidget *manage =
        gtk_button_new_with_label("Manage cards");

    g_signal_connect(
        reveal,
        "clicked",
        G_CALLBACK(on_reveal_clicked),
        s);

    g_signal_connect(
        next,
        "clicked",
        G_CALLBACK(on_next_clicked),
        s);

    g_signal_connect(
        later,
        "clicked",
        G_CALLBACK(on_close_clicked),
        s);

    g_signal_connect(
        manage,
        "clicked",
        G_CALLBACK(on_manage_clicked),
        s);

    gtk_box_append(GTK_BOX(bar), reveal);
    gtk_box_append(GTK_BOX(bar), next);
    gtk_box_append(GTK_BOX(bar), later);
    gtk_box_append(GTK_BOX(bar), manage);

    s->reveal_button = reveal;

    return bar;
}

static void create_main_window(AppState *s) {
    s->window = gtk_application_window_new(s->app);

    gtk_window_set_title(
        GTK_WINDOW(s->window),
        "Flashcard");

    gtk_window_set_default_size(
        GTK_WINDOW(s->window),
        550,
        350);

    gtk_window_set_resizable(
        GTK_WINDOW(s->window),
        FALSE);

    GtkWidget *box =
        gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_margin_top(box, 30);
    gtk_widget_set_margin_bottom(box, 30);
    gtk_widget_set_margin_start(box, 30);
    gtk_widget_set_margin_end(box, 30);

    GtkWidget *heading =
        gtk_label_new("Today's flashcard");

    gtk_widget_add_css_class(
        heading,
        "title-1");

    gtk_box_append(
        GTK_BOX(box),
        heading);

    s->question_label =
        gtk_label_new("");

    gtk_label_set_wrap(
        GTK_LABEL(s->question_label),
        TRUE);

    gtk_label_set_wrap_mode(
        GTK_LABEL(s->question_label),
        PANGO_WRAP_WORD_CHAR);

    gtk_widget_add_css_class(
        s->question_label,
        "title-2");

    gtk_box_append(
        GTK_BOX(box),
        s->question_label);

    s->answer_label =
        gtk_label_new("");

    gtk_label_set_wrap(
        GTK_LABEL(s->answer_label),
        TRUE);

    gtk_label_set_wrap_mode(
        GTK_LABEL(s->answer_label),
        PANGO_WRAP_WORD_CHAR);

    gtk_box_append(
        GTK_BOX(box),
        s->answer_label);

    GtkWidget *buttons =
        make_button_bar(s);

    gtk_box_append(
        GTK_BOX(box),
        buttons);

    gtk_window_set_child(
        GTK_WINDOW(s->window),
        box);

    /* Do not force the flashcard above other windows. */
    gtk_window_set_focus_visible(
        GTK_WINDOW(s->window),
        FALSE);
}

static gboolean delayed_start(gpointer data) {
    AppState *s = data;

    s->startup_timer = 0;

    if (have_cards(s))
        load_random_card(s);

    return G_SOURCE_REMOVE;
}

static void app_activate(
    GtkApplication *app,
    gpointer user_data) {

    AppState *s = user_data;

    s->app = app;

    if (sqlite3_open(DB_FILE, &s->db) != SQLITE_OK) {
        g_printerr(
            "Could not open %s\n",
            DB_FILE);
        return;
    }

    if (!init_database(s)) {
        sqlite3_close(s->db);
        s->db = NULL;
        return;
    }

    create_main_window(s);

    /*
     * Wait 30 seconds after desktop login before
     * showing the first card.
     */
    s->startup_timer =
        g_timeout_add_seconds(
            DEFAULT_DELAY_SECONDS,
            delayed_start,
            s);
}

static void app_shutdown(
    GApplication *app,
    gpointer user_data) {

    (void)app;

    AppState *s = user_data;

    if (s->startup_timer) {
        g_source_remove(s->startup_timer);
        s->startup_timer = 0;
    }

    if (s->db) {
        sqlite3_close(s->db);
        s->db = NULL;
    }
}

int main(int argc, char **argv) {
    AppState state = {0};

    GtkApplication *app =
        gtk_application_new(
            APP_ID,
            G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(
        app,
        "activate",
        G_CALLBACK(app_activate),
        &state);

    g_signal_connect(
        app,
        "shutdown",
        G_CALLBACK(app_shutdown),
        &state);

    int status =
        g_application_run(
            G_APPLICATION(app),
            argc,
            argv);

    g_object_unref(app);

    return status;
}
