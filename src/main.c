#include <string.h>
#include <gtk/gtk.h>
#include <adwaita.h>
#include <curl/curl.h>

#include "api.h"
#include "parser.h"

typedef struct {
        GtkWindow *win;
        AdwToastOverlay *overlay;
        GtkSpinner *spin;
        GtkPicture *pic;
        GBytes *image_bytes;
        GtkDropDown *nsfw_menu;
} ApplicationContext;

ApplicationContext *ctx;

static char *urls[] = {
        "https://nekos.moe/api/v1/random/image?nsfw=false",
        "https://nekos.moe/api/v1/random/image",
        "https://nekos.moe/api/v1/random/image?nsfw=true"
};

static gboolean reload_finish(gpointer data)
{
        gtk_spinner_stop(ctx->spin);

        GdkTexture *texture = gdk_texture_new_from_bytes(ctx->image_bytes, NULL);
        gtk_picture_set_paintable(ctx->pic, GDK_PAINTABLE(texture));

        g_bytes_unref(ctx->image_bytes);

        return FALSE;
}

static const char *get_link()
{
        GtkStringObject *nsfw_obj = gtk_drop_down_get_selected_item(ctx->nsfw_menu);
        const char *nsfw_str = gtk_string_object_get_string(nsfw_obj);

        if (strcmp(nsfw_str, "No NSFW") == 0)
                return urls[0];
        else if (strcmp(nsfw_str, "Allow NSFW") == 0)
                return urls[1];
        else
                return urls[2];
}

static gpointer reload_async(gpointer data)
{
        g_idle_add((GSourceFunc)gtk_spinner_start, ctx->spin);

        GString *image_json = curl_perform_request(get_link());
        GString *image_url = get_image_url(image_json);
        GString *image_str = curl_perform_request(image_url->str);
        GBytes *image_bytes = g_string_free_to_bytes(image_str);

        g_string_free(image_json, TRUE);
        g_string_free(image_url, TRUE);

        ctx->image_bytes = image_bytes;

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
        GtkDropDown *nsfw_menu;

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
        nsfw_menu = GTK_DROP_DOWN(gtk_builder_get_object(pref_build, "nsfw_menu"));

        ctx->win = win;
        ctx->overlay = overlay;
        ctx->spin = spin;
        ctx->pic = pic;
        ctx->nsfw_menu = nsfw_menu;

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
