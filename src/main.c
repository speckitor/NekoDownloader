#include <adwaita.h>
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <string.h>
#include <webp/decode.h>

#include "api.h"
#include "parser.h"

typedef struct {
    GtkWindow *win;
    AdwToastOverlay *overlay;
    GtkSpinner *spin;
    GtkPicture *pic;
    GtkSwitch *nekos_switch;
    GtkCheckButton *rating_list[4];
    GBytes *image_bytes;
} ApplicationContext;

ApplicationContext *ctx;

static const char *api_url = "https://api.nekosapi.com/v4/images/random?limit=1";

static gboolean reload_finish(gpointer data)
{
    gtk_spinner_stop(ctx->spin);

    int width, height;
    gsize image_size;

    const uint8_t *image_data = g_bytes_get_data(ctx->image_bytes, &image_size);
    uint8_t *rgba = WebPDecodeRGBA(image_data, image_size, &width, &height);

    if (!rgba) {
        g_printerr("Failed to decode webp\n");
        g_bytes_unref(ctx->image_bytes);
        return FALSE;
    }

    GBytes *pixel_bytes = g_bytes_new_take(rgba, width * height * 4);

    GdkTexture *texture =
        gdk_memory_texture_new(width, height, GDK_MEMORY_R8G8B8A8, pixel_bytes, width * 4);
    gtk_picture_set_paintable(ctx->pic, GDK_PAINTABLE(texture));

    g_bytes_unref(pixel_bytes);
    g_object_unref(texture);
    g_free(ctx->image_bytes);

    return FALSE;
}

static GString *get_rating(GtkCheckButton *ratings_list[4])
{
    GString *rating = g_string_new("&rating=");
    size_t len = rating->len;

    for (int i = 0; i < 4; ++i) {
        if (gtk_check_button_get_active(ratings_list[i])) {
            g_string_append(rating, gtk_check_button_get_label(ratings_list[i]));
            g_string_append(rating, ",");
        }
    }

    if (len == rating->len) {
        g_string_free(rating, TRUE);
        return NULL;
    }

    return rating;
}

static gpointer reload_async(gpointer data)
{
    g_idle_add((GSourceFunc)gtk_spinner_start, ctx->spin);

    GString *url = g_string_new(api_url);

    if (gtk_switch_get_active(ctx->nekos_switch)) {
        g_string_append(url, "&tags=catgirl");
    }

    GString *rating = get_rating(ctx->rating_list);
    if (rating) {
        g_string_append(url, rating->str);
        g_string_free(rating, TRUE);
    }

    GString *image_json = curl_perform_request(url->str);
    GString *image_url = get_image_url(image_json);
    GString *image_str = curl_perform_request(image_url->str);

    ctx->image_bytes = g_string_free_to_bytes(image_str);

    g_string_free(image_json, TRUE);
    g_string_free(image_url, TRUE);

    g_idle_add_full(G_PRIORITY_DEFAULT, reload_finish, NULL, NULL);

    return NULL;
}

static void reload()
{
    if (gtk_spinner_get_spinning(ctx->spin))
        return;

    g_thread_new("reload_thread", (GThreadFunc)reload_async, NULL);
}

static void copy_image(gpointer data)
{
    GdkPaintable *paintb = gtk_picture_get_paintable(ctx->pic);

    GdkTexture *texture = GDK_TEXTURE(paintb);
    GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(ctx->pic));
    GdkClipboard *clip = gdk_display_get_clipboard(display);
    gdk_clipboard_set_texture(clip, texture);

    adw_toast_overlay_add_toast(ctx->overlay, adw_toast_new(("Image Copied")));
}
static void dialog_finish(GObject *source_object, GAsyncResult *res, gpointer data)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source_object);
    GFile *file = gtk_file_dialog_save_finish(dialog, res, NULL);

    if (file) {
        const char *path = g_file_get_path(file);

        GdkPaintable *paintb = gtk_picture_get_paintable(ctx->pic);

        GdkTexture *texture = GDK_TEXTURE(paintb);

        gdk_texture_save_to_png(texture, path);

        g_print("Saved file: (path: %s)\n", path);

        adw_toast_overlay_add_toast(ctx->overlay, adw_toast_new(("Image Saved")));

        g_object_unref(file);
    } else {
        g_warning("Save dialog was cancelled or failed\n");
    }

    g_object_unref(dialog);
}

static void save_image(GtkButton *self, gpointer data)
{
    GtkFileDialog *dialog = gtk_file_dialog_new();

    gtk_file_dialog_set_title(dialog, "Save Image");
    gtk_file_dialog_set_initial_name(dialog, "Neko.png");

    gtk_file_dialog_save(dialog, ctx->win, NULL, dialog_finish, NULL);
}

static void activate(GApplication *app)
{
    GtkBuilder *build;
    GtkWindow *win;
    AdwToastOverlay *overlay;
    GtkButton *refresh;
    GtkSpinner *spin;
    GtkButton *copy;
    GtkButton *save;
    GtkButton *pref;
    GtkPicture *pic;

    GtkBuilder *pref_build;
    GtkWindow *pref_win;
    GtkCheckButton *safe_btn;
    GtkCheckButton *suggestive_btn;
    GtkCheckButton *borderline_btn;
    GtkCheckButton *explicit_btn;
    GtkSwitch *nekos_switch;

    build = gtk_builder_new_from_resource("/org/speckitor/nekodownloader/window.ui");
    win = GTK_WINDOW(gtk_builder_get_object(build, "win"));
    overlay = ADW_TOAST_OVERLAY(gtk_builder_get_object(build, "toast_overlay"));
    refresh = GTK_BUTTON(gtk_builder_get_object(build, "refresh_button"));
    spin = GTK_SPINNER(gtk_builder_get_object(build, "spinner"));
    copy = GTK_BUTTON(gtk_builder_get_object(build, "copy"));
    save = GTK_BUTTON(gtk_builder_get_object(build, "save"));
    pref = GTK_BUTTON(gtk_builder_get_object(build, "pref"));
    pic = GTK_PICTURE(gtk_builder_get_object(build, "pic"));

    pref_build = gtk_builder_new_from_resource("/org/speckitor/nekodownloader/preferences.ui");
    pref_win = GTK_WINDOW(gtk_builder_get_object(pref_build, "pref_win"));
    safe_btn = GTK_CHECK_BUTTON(gtk_builder_get_object(pref_build, "safe_btn"));
    suggestive_btn = GTK_CHECK_BUTTON(gtk_builder_get_object(pref_build, "suggestive_btn"));
    borderline_btn = GTK_CHECK_BUTTON(gtk_builder_get_object(pref_build, "borderline_btn"));
    explicit_btn = GTK_CHECK_BUTTON(gtk_builder_get_object(pref_build, "explicit_btn"));
    nekos_switch = GTK_SWITCH(gtk_builder_get_object(pref_build, "nekos_switch"));

    ctx->win = win;
    ctx->overlay = overlay;
    ctx->spin = spin;
    ctx->pic = pic;
    ctx->rating_list[0] = safe_btn;
    ctx->rating_list[1] = suggestive_btn;
    ctx->rating_list[2] = borderline_btn;
    ctx->rating_list[3] = explicit_btn;
    ctx->nekos_switch = nekos_switch;

    gtk_window_set_application(win, GTK_APPLICATION(app));

    g_signal_connect_swapped(refresh, "clicked", G_CALLBACK(reload), NULL);
    g_signal_connect_swapped(copy, "clicked", G_CALLBACK(copy_image), NULL);
    g_signal_connect_swapped(save, "clicked", G_CALLBACK(save_image), NULL);

    gtk_window_set_hide_on_close(pref_win, TRUE);
    g_signal_connect_swapped(pref, "clicked", G_CALLBACK(gtk_widget_show), GTK_WIDGET(pref_win));

    gtk_window_set_transient_for(pref_win, win);

    gtk_window_present(win);

    reload();
}

int main(int argc, char **argv)
{
    ctx = g_new(ApplicationContext, 1);
    AdwApplication *app;
    int status;

    curl_global_init(CURL_GLOBAL_ALL);
    gtk_init();

    app = adw_application_new("org.speckitor.NekoDownloader", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_free(ctx);
    g_object_unref(app);
    return status;
}
