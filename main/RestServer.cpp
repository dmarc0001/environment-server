#include "RestServer.hpp"
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

namespace rest_webserver
{
  const char *RestServer::tag{"RestServer"}; //! tag fürs debug logging

  /**
   * initialize the server process
   */
  void RestServer::init()
  {
    ESP_LOGI(RestServer::tag, "RestServer init...");
    // ESP_LOGD(RestServer::tag, "esp enent loop init...");
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    // ESP_LOGI(RestServer::tag, "RestServer init...OK");
  }

  /**
   * the main thread for server
   */
  void RestServer::compute()
  {
    ESP_LOGI(RestServer::tag, "compute start...");
    ESP_ERROR_CHECK(RestServer::startRestServer(Prefs::WEB_PATH));
  }

  /**
   * startet den Webserver
   */
  esp_err_t RestServer::startRestServer(const char *base_path)
  {
    // check path
    // REST_CHECK( base_path, "wrong base path", err );
    //
    // allocate RAM
    //
    rest_server_context_t *rest_context = static_cast<rest_server_context_t *>(calloc(1, sizeof(rest_server_context_t)));
    // TODO: REST_CHECK( rest_context, "No memory for rest context", err );
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    //
    // Server prepare
    //
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard; // standart defined by espressif

    ESP_LOGI(RestServer::tag, "Starting HTTP Server");
    // TODO: richtig testen
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri;
    system_info_get_uri.uri = "/api/v1/system/info";
    system_info_get_uri.method = HTTP_GET;
    system_info_get_uri.handler = RestServer::systemInfoGetHandler;
    system_info_get_uri.user_ctx = rest_context;
    httpd_register_uri_handler(server, &system_info_get_uri);

    return ESP_OK;
  }

  /* Simple handler for getting system handler */
  esp_err_t RestServer::systemInfoGetHandler(httpd_req_t *req)
  {
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
  }

}
