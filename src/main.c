#include <gtk/gtk.h>
#include <adwaita.h>
#include <curl/curl.h>

#include "api.h"
#include "parser.h"

typedef struct {
        GtkPicture *pic;
        GtkSpinner *spin;
} ReloadWidgets;

typedef struct {
        ReloadWidgets *wid;
        GBytes *image_bytes;
        GString *image_json;
        GString *image_url;
} ReloadContext;

static gboolean reload_finish(gpointer user_data)
{
        ReloadContext *ctx = user_data;
        gtk_spinner_stop(ctx->wid->spin);

        GdkTexture *texture = gdk_texture_new_from_bytes(ctx->image_bytes, NULL);
        gtk_picture_set_paintable(ctx->wid->pic, GDK_PAINTABLE(texture));

        g_bytes_unref(ctx->image_bytes);
        g_string_free(ctx->image_json, TRUE);
        g_string_free(ctx->image_url, TRUE);
        g_free(ctx);

        return FALSE;
}

static gpointer reload_async(gpointer user_data)
{
        ReloadWidgets *wid = user_data;

        g_idle_add((GSourceFunc)gtk_spinner_start, wid->spin);

        GString *image_json = curl_perform_request("https://nekos.moe/api/v1/random/image");
        GString *image_url = get_image_url(image_json);
        GString *image_str = curl_perform_request(image_url->str);
        GBytes *image_bytes = g_string_free_to_bytes(image_str);

        ReloadContext *ctx = g_new(ReloadContext, 1);
        ctx->wid = wid;
        ctx->image_bytes = image_bytes;
        ctx->image_json = image_json;
        ctx->image_url = image_url;

        g_idle_add_full(G_PRIORITY_DEFAULT, reload_finish, ctx, NULL);

        return NULL;
}

static void reload(GtkButton *self, gpointer user_data)
{
        g_thread_new("reload_thread", (GThreadFunc)reload_async, user_data);
}

static void copy_image(GtkButton *self, gpointer user_data)
{
        GtkPicture *pic = user_data;
        GdkPaintable *paintb = gtk_picture_get_paintable(pic);

        if (GDK_IS_TEXTURE(paintb)) {
                GdkTexture *texture = GDK_TEXTURE(paintb);
                GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(pic));
                GdkClipboard *clip = gdk_display_get_clipboard(display);
                gdk_clipboard_set_texture(clip, texture);
        }
}

static void save_image(GtkButton *self, gpointer user_data)
{

}

static void activate(GApplication *app)
{
        GtkWindow *win;
        GtkButton *refresh;
        GtkSpinner *spin;
        GtkButton *copy;
        GtkPicture *pic;
        GtkBuilder *build;

        build = gtk_builder_new_from_file("./ui/window.ui");
        win = GTK_WINDOW(gtk_builder_get_object(build, "win"));
        refresh = GTK_BUTTON(gtk_builder_get_object(build, "refresh_button"));
        spin = GTK_SPINNER(gtk_builder_get_object(build, "spinner"));
        copy = GTK_BUTTON(gtk_builder_get_object(build, "copy"));
        pic = GTK_PICTURE(gtk_builder_get_object(build, "pic"));

        ReloadWidgets *wid = g_new(ReloadWidgets, 1);
        wid->pic = pic;
        wid->spin = spin;

        gtk_window_set_application(win, GTK_APPLICATION(app));

        g_signal_connect(refresh, "clicked", G_CALLBACK(reload), wid);
        g_signal_connect(copy, "clicked", G_CALLBACK(copy_image), pic);
        g_signal_connect(win, "destroy", G_CALLBACK(g_free), wid);

        gtk_window_present(win);

        reload(NULL, wid);
}

int main(int argc, char **argv)
{
        AdwApplication *app;
        int status;

        curl_global_init(CURL_GLOBAL_ALL);

        app = adw_application_new("org.speckitor.CatgirlDownloader", G_APPLICATION_DEFAULT_FLAGS);
        g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
        status = g_application_run(G_APPLICATION(app), argc, argv);

        g_object_unref(app);
        return status;
}
