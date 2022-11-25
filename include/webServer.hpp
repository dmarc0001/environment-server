#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <wifi_manager.h>
#include <esp_sntp.h>
#include "AppPreferences.hpp"

namespace webserver
{

  constexpr int FILE_PATH_MAX = (ESP_VFS_PATH_MAX + CONFIG_SPIFFS_OBJ_NAME_LEN);

  typedef struct webServerContext
  {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[Prefs::WEB_SCRATCH_BUFSIZE];
  } web_server_context_t;

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

    static const char *getPathFromUri(char *, const char *, const char *, size_t);
    static esp_err_t indexHtmlGetHandler(httpd_req_t *);
    static esp_err_t systemInfoGetHandler(httpd_req_t *);
    static esp_err_t registerRootHandler(httpd_handle_t, web_server_context_t *);
    static esp_err_t rootGetHandler(httpd_req_t *);
    static esp_err_t restGetHandler(httpd_req_t *);
    static esp_err_t setContentTypeFromFile(httpd_req_t *, const char *);
    inline static bool isFileTypeExist(const char *filename, const char *ext)
    {
      return (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0);
    }
  };

}
