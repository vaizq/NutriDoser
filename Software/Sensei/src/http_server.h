//
// Created by vaige on 16.7.2024.
//
#ifndef UNTITLED3_HTTP_SERVER_H
#define UNTITLED3_HTTP_SERVER_H


#include "esp_http_server.h"
#include <cmath>
#include <vector>
#include "cors.h"


class HTTPServer 
{
public:
    using Handler = esp_err_t(*)(httpd_req_t*);

    ~HTTPServer()
    {
        if (server) {
            /* Stop the httpd server */
            httpd_stop(server);
        }
    }

    void get(const char* uri, Handler handler)
    {
        httpd_uri_t uri_get = {
                .uri      = uri,
                .method   = HTTP_GET,
                .handler  = handler,
                .user_ctx = nullptr
        };
        handlers.push_back(uri_get);
    }

    void post(const char* uri, Handler handler)
    {
        httpd_uri_t uri_post = {
                .uri      = uri,
                .method   = HTTP_POST,
                .handler  = handler,
                .user_ctx = nullptr
        };
        handlers.push_back(uri_post);
    }

    void listen(uint16_t port = 80)
    {
        /* Generate default configuration */
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = 16;
        config.server_port = port;

        /* Start the httpd server */
        if (httpd_start(&server, &config) == ESP_OK) {
            /* Register URI handlers */
            for (const auto& handler : handlers) {
                httpd_register_uri_handler(server, &handler);
                const httpd_uri_t options_uri = {
                        .uri = handler.uri,
                        .method = HTTP_OPTIONS,
                        .handler = options_handler,
                        .user_ctx = nullptr
                };
                httpd_register_uri_handler(server, &options_uri);
            }
        }
    }
private:
    std::vector<httpd_uri_t> handlers;
    httpd_handle_t server{nullptr};
};


#endif //UNTITLED3_HTTP_SERVER_H