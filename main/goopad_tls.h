#pragma once

#include "esp_https_server.h"

esp_err_t goopad_tls_configure(httpd_ssl_config_t *config, const char *ip);
