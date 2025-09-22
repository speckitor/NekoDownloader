#include <glib.h>
#include <json-glib/json-glib.h>

#include "parser.h"

GString *get_image_url(GString *image_json)
{
    if (!image_json) {
        g_printerr("Failed to get image url, no image data provided\n");
        return NULL;
    }

    JsonParser *parser = json_parser_new();

    GError *error = NULL;
    if (!json_parser_load_from_data(parser, image_json->str, -1, &error)) {
        g_printerr("Failed to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return NULL;
    }

    JsonReader *reader = json_reader_new(json_parser_get_root(parser));

    if (!json_reader_read_element(reader, 0) || !json_reader_read_member(reader, "url")) {
        g_printerr("Failed to read JSON\n");
        g_object_unref(reader);
        g_object_unref(parser);
        return NULL;
    }

    GString *image_url = g_string_new(json_reader_get_string_value(reader));

    g_object_unref(reader);
    g_object_unref(parser);

    g_print("Image found (url: %s)\n", image_url->str);
    return image_url;
}
