#if 0
gcc -DNTESTING -Wall -g -I../core libedit.c ../core/?*.c -o libedit `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`
exit
#endif


#include "library.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>



typedef struct
{
    GtkWidget *image_label;
    GtkWidget *image;
    GtkWidget *read_as;
    GtkWidget *radius;
    GtkWidget *window;
    Library library;
    int shelf;
    int record;
    char *path;
} Editor;


static void on_prev(GtkButton *button, Editor *e);
static void on_next(GtkButton *button, Editor *e);



static Shelf *current_shelf(Editor *e)
{
    return library_get_shelf(e->library, e->shelf);
}

static LibraryRecord *current_record(Editor *e)
{
    return &current_shelf(e)->records[e->record];
}


static void set_image(Editor *e)
{
    Shelf *s = current_shelf(e);
    LibraryRecord *rec = &s->records[e->record];
    int x, y;
    int w = s->width, h = s->height;
    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                                       /*alpha:*/ FALSE,
                                       /* bits_per_sample: */ 8,
                                       w, h);
    unsigned char **pixels = s->pixels;
    for (y = 0; y < h; y++)
    {
        guchar *p = gdk_pixbuf_get_pixels(pixbuf) + y * gdk_pixbuf_get_rowstride(pixbuf);
        
        for (x = 0; x < w; x++)
        {
            int black = pixels[y][x];
            int in_x = x >= rec->left && x < rec->left + rec->width;
            int in_y = y >= rec->top && y < rec->top + rec->height;
            int inside = in_x && in_y;
            guchar color = 128 + (1 - black * 2) * (inside ? 127 : 50);
            p[3 * x] = p[3 * x + 1] = p[3 * x + 2] = color;            
        }
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(e->image), pixbuf);
    g_object_unref(G_OBJECT(pixbuf));    
}


static void set_image_label(Editor *e)
{
    char buf[100];
    sprintf(buf, "Word: %d/%d   Fragment: %d/%d", e->shelf + 1,
                                                  library_shelves_count(e->library),
                                                  e->record + 1, library_get_shelf(e->library, e->shelf)->count);
    gtk_label_set_text(GTK_LABEL(e->image_label), buf);
}


static void update_record(Editor *e)
{
    const char *t = gtk_entry_get_text(GTK_ENTRY(e->read_as));
    int r = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(e->radius));
    LibraryRecord *rec = current_record(e);
    strncpy(rec->text, t, MAX_TEXT_SIZE);
    rec->radius = r;
}

/*
static void on_delete(GtkButton *button, Editor *e)
{
    update_record(e);
    printf("delete\n");
}
*/

static gboolean on_quit(GtkWidget *widget, GdkEvent *event, Editor *e)
{
    update_record(e);
    library_save(e->library, e->path, 0);
    library_free(e->library);
    gtk_main_quit();
    return FALSE;
}

static GtkWidget *create_buttons(Editor *e)
{
    GtkWidget *hbox = gtk_hbox_new(TRUE, 0);
    GtkWidget *button;

    button = gtk_button_new_with_mnemonic("Previous");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_prev), e);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 2);

    button = gtk_button_new_with_mnemonic("Next");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_next), e);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 2);

    /*
    button = gtk_button_new_with_mnemonic("Delete");
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_delete), e);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 2);
    */

    return hbox;
}


static GtkWidget *wrap_with_label_ex(GtkWidget *w, const char *s, gboolean expand, GtkWidget **plabel, int d)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    GtkWidget *label;
    
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(s);
    if (plabel)
        *plabel = label;
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), w, expand, TRUE, d);

    return vbox;
}

static GtkWidget *wrap_with_label(GtkWidget *w, const char *s)
{
    return wrap_with_label_ex(w, s, FALSE, NULL, 0);
}


static GtkWidget *create_sample_view(Editor *e)
{
    GtkWidget *image = gtk_image_new();
    e->image = image;
    return wrap_with_label_ex(image, "", TRUE, &e->image_label, 5);
}

static GtkWidget *create_text_input(Editor *e)
{
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_TEXT_SIZE - 1);
    e->read_as = entry;
    return wrap_with_label(entry, "Read as:");
}

static GtkWidget *create_radius_input(Editor *e)
{
    GtkObject *radius_adj = gtk_adjustment_new (50, 0, 5000, 1, 10, 10);
    GtkWidget *spin = gtk_spin_button_new(GTK_ADJUSTMENT(radius_adj), 1, 0);
    e->radius = spin;
    return wrap_with_label(spin, "Radius:");
}

static void create_library_editor(Editor *e)
{
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    GtkWidget *library_editor = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_gravity(GTK_WINDOW(library_editor), GDK_GRAVITY_CENTER);

    gtk_container_add(GTK_CONTAINER(library_editor), vbox);
    
    gtk_box_pack_start(GTK_BOX(vbox), create_sample_view(e),  TRUE,  TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_text_input(e),   FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), create_radius_input(e), FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), create_buttons(e),      FALSE, TRUE, 5);    

    e->window = library_editor;
}

static void update_view(Editor *e)
{
    LibraryRecord *rec = current_record(e);
    gtk_entry_set_text(GTK_ENTRY(e->read_as), rec->text);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(e->radius), rec->radius);
    set_image(e);
    set_image_label(e);
    gtk_widget_grab_focus(e->read_as);
}



/* ________________________________   prev/next   ____________________________________ */

static void on_prev(GtkButton *button, Editor *e)
{
    update_record(e);
    if (e->record > 0)
    {
        e->record--;
        update_view(e);
        return;
    }

    e->shelf--;
    if (e->shelf == -1)
        e->shelf = library_shelves_count(e->library) - 1;
    e->record = current_shelf(e)->count - 1;
    update_view(e);
}

static void on_next(GtkButton *button, Editor *e)
{
    Shelf *s = current_shelf(e);
    update_record(e);
    if (e->record < s->count - 1)
    {
        e->record++;
        update_view(e);
        return;
    }

    e->record = 0;
    e->shelf++;
    if (e->shelf >= library_shelves_count(e->library))
        e->shelf = 0;
    update_view(e);
}

/* _________________________________   on_key   _______________________________ */

static gboolean on_key(GtkWidget *widget, GdkEventKey *event, Editor *e)
{
    switch(event->keyval)
    {
        case GDK_Up:
            on_prev(NULL, e);
            return TRUE;
        case GDK_Down: case GDK_Return:
            on_next(NULL, e);
            return TRUE;            
    }
    return FALSE;
}


/* _________________________________   main   _______________________________ */

int main(int argc, char **argv)
{
    Editor e;
    GtkWidget *win;
    gtk_init(&argc, &argv);

    if (argc <= 1)
    {
        fprintf(stderr, "usage: %s [-r] <library>\n", argv[0]);
        exit(1);
    }

    if (!strcmp(argv[1], "-r"))
    {
        e.path = argv[2];
        e.library = library_open_recreating(e.path);
    }
    else
    {
        e.path = argv[1];
        e.library = library_open(e.path);
        library_read_prototypes(e.library);
    }
    e.shelf = 0;
    e.record = 0;
    if (!library_shelves_count(e.library))
    {
        fprintf(stderr, "the library %s is empty!\n", e.path);
        exit(1);
    }
    
    create_library_editor(&e);
    update_view(&e);
    win = e.window;
    gtk_window_set_title(GTK_WINDOW(win), e.path);
    g_signal_connect(G_OBJECT(win), "key_press_event", G_CALLBACK(on_key), &e);
    g_signal_connect(G_OBJECT(win), "delete_event", G_CALLBACK(on_quit), &e);
    gtk_widget_show_all(win);
    gtk_main();

    return 0;
}
