#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <wifi_manager.h>
#include <esp_sntp.h>
#include "appPreferences.hpp"

namespace webserver
{

  constexpr int FILE_PATH_MAX = (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN);

  class WebServer
  {
  private:
    static const char *tag; //! Kennzeichnung fürs debug
    static bool is_spiffs_ready;
    static bool is_snmp_init;

  public:
    static void init();
    static void compute();

  private:
    static esp_err_t spiffs_init();
    static void callBackConnectionOk(void *);
    static void callBackDisconnect(void *);
    static esp_err_t callbackGetHttpHandler(httpd_req_t *);
    static void time_sync_notification_cb(struct timeval *);

    static esp_err_t getPathFromUri(std::string &, const std::string &, std::string &);
    static esp_err_t indexHtmlGetHandler(httpd_req_t *);
    static esp_err_t apiSystemInfoGetHandler(httpd_req_t *);
    static esp_err_t rootGetHandler(httpd_req_t *);
    static esp_err_t apiRestHandlerCurrent(httpd_req_t *);
    static esp_err_t apiRestHandlerInterval(httpd_req_t *);
    static esp_err_t setContentTypeFromFile(std::string &, httpd_req_t *, const std::string &);
  };

}
