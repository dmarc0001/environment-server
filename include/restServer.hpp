#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <esp_vfs.h>
#include <esp_http_server.h>
#include "AppPreferences.hpp"

namespace rest_webserver
{
  typedef struct rest_server_context
  {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[Prefs::WEB_SCRATCH_BUFSIZE];
  } rest_server_context_t;

  class RestServer
  {
  private:
    static const char *tag; //! Kennzeichnung fürs debug
    static bool is_spiffs_ready;

  public:
    static void init();
    static void compute();

  private:
    static esp_err_t spiffs_init();
    static esp_err_t index_html_get_handler(httpd_req_t *);
    static const char *get_path_from_uri(char *, const char *, const char *, size_t);
    static esp_err_t startRestServer(const char *);
    static esp_err_t registerSystemInfoHandler(httpd_handle_t, rest_server_context_t *);
    static esp_err_t systemInfoGetHandler(httpd_req_t *);
    static esp_err_t registerRootHandler(httpd_handle_t, rest_server_context_t *);
    static esp_err_t rootGetHandler(httpd_req_t *);
  };

}
