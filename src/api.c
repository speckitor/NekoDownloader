#include <glib.h>
#include <curl/curl.h>

#include "api.h"

static size_t curl_write_function(void *contents, size_t size, size_t nmemb, GString *userp)
{
        size_t realsize = size * nmemb;
        g_string_append_len(userp, contents, realsize);
        return realsize;
}

GString *curl_perform_request(const char *url)
{
        CURL *handle = curl_easy_init();

        if (!handle) {
                g_warning("Failed to initialize CURL\n");
                return NULL; 
        }

        GString *buf = g_string_new(NULL);
        CURLcode result;

        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_write_function);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, buf);

        result = curl_easy_perform(handle);

        curl_easy_cleanup(handle);

        if (result == CURLE_OK) {
                return buf; 
        }

        g_warning("Request failed: %s (URL: %s)\n", curl_easy_strerror(result), url);
        g_string_free(buf, TRUE);
        return NULL;
}
