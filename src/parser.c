#include <glib.h>
#include <json-glib/json-glib.h>

#include "parser.h"

static GString *get_image_id(GString *image_json)
{
    if (!image_json) {
        g_warning("Failed to get image id, image json is NULL\n");
        return NULL;
    }

    JsonParser *parser = json_parser_new();

    GError *error = NULL;
    if (!json_parser_load_from_data(parser, image_json->str, -1, &error)) {
        g_warning("Failed to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return NULL;
    }

    JsonReader *reader = json_reader_new(json_parser_get_root(parser));

    if (!json_reader_read_member(reader, "images") || !json_reader_read_element(reader, 0) ||
        !json_reader_read_member(reader, "id")) {
        g_warning("Failed to read JSON\n");
        g_object_unref(reader);
        g_object_unref(parser);
        return NULL;
    }

    GString *image_id = g_string_new(json_reader_get_string_value(reader));

    g_object_unref(reader);
    g_object_unref(parser);

    return image_id;
}

static GString *construct_image_url(GString *image_id)
{
    GString *image_url = g_string_new("https://nekos.moe/image/");
    g_string_append_len(image_url, image_id->str, image_id->len);
    return image_url;
}

GString *get_image_url(GString *image_json)
{
    GString *image_id = get_image_id(image_json);
    GString *image_url = construct_image_url(image_id);
    g_print("Image found (id: %s; url: %s)\n", image_id->str, image_url->str);

    g_string_free(image_id, TRUE);
    return image_url;
}
