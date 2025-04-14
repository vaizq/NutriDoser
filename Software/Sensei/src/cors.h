//
// Created by vaige on 17.7.2024.
//

#ifndef SENSEI_CORS_H
#define SENSEI_CORS_H

#include "esp_http_server.h"


// Function to set CORS headers
esp_err_t set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    return ESP_OK;
}

esp_err_t options_handler(httpd_req_t *req) {
    set_cors_headers(req);
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "3600");
    httpd_resp_send(req, nullptr, 0);  // Send an empty response body
    return ESP_OK;
}

#endif //SENSEI_CORS_H