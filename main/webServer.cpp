#include <string>
#include <memory>
#include <sys/param.h>
#include <esp_vfs.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <cJSON.h>
#include <esp_log.h>
#include <lwip/err.h>
#include <lwip/sys.h>
#include <esp_spiffs.h>
#include <http_app.h>
#include <esp_http_server.h>
#include <http_app.h>
#include <wifi_manager.h>
#include "statusObject.hpp"
#include "webServer.hpp"

//extern httpd_handle_t httpd_handle;

namespace webserver
{

  const char *WebServer::tag{"webserver"}; //! tag fürs debug logging
  bool WebServer::is_spiffs_ready{false};
  bool WebServer::is_snmp_init{false};

  /**
   * initialize the server process
  */
  void WebServer::init()
  {
    ESP_LOGI(WebServer::tag, "WebServer init...");

    WebServer::spiffs_init();
    wifi_manager_start();

    /* register a callback as an example to how you can integrate your code with the wifi manager */
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &WebServer::callBackConnectionOk);
    wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &WebServer::callBackDisconnect);
    http_app_set_handler_hook(HTTP_GET, &WebServer::callbackGetHttpHandler);
  }

  void WebServer::compute()
  {
    // zyclic call from main

    //
    //TODO: Callbacks if WiFi or not
    //
  }

  esp_err_t WebServer::spiffs_init()
  {
    ESP_LOGI(WebServer::tag, "initializing spiffs...");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = Prefs::WEB_PATH,
        .partition_label = nullptr,
        .max_files = 5,
        .format_if_mount_failed = false};

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
      if (ret == ESP_FAIL)
      {
        ESP_LOGE(WebServer::tag, "failed to mount or format filesystem");
      }
      else if (ret == ESP_ERR_NOT_FOUND)
      {
        ESP_LOGE(WebServer::tag, "failed to find spiffs partition");
      }
      else
      {
        ESP_LOGE(WebServer::tag, "failed to initialize spiffs (%s)", esp_err_to_name(ret));
      }
    }
    else
    {
      WebServer::is_spiffs_ready = true;
      ESP_LOGI(WebServer::tag, "initializing spiffs...OK");
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(nullptr, &total, &used);
    if (ret != ESP_OK)
    {
      ESP_LOGE(WebServer::tag, "failed to get spiffs partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
      ESP_LOGI(WebServer::tag, "partition size: total: %d, used: %d", total, used);
    }
    return ret;
  }

  void WebServer::callBackConnectionOk(void *pvParameter)
  {
    StatusObject::setWlanState(WlanState::CONNECTED);
    http_app_set_handler_hook(HTTP_GET, &WebServer::callbackGetHttpHandler);
    if (!WebServer::is_snmp_init)
    {
      sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
      sntp_setoperatingmode(SNTP_OPMODE_POLL);
      sntp_setservername(1, "pool.ntp.org");
      sntp_set_time_sync_notification_cb(WebServer::time_sync_notification_cb);
      sntp_init();
      WebServer::is_snmp_init = true;
    }
    else
    {
      sntp_restart();
    }

    ip_event_got_ip_t *param = (ip_event_got_ip_t *)pvParameter;
    /* transform IP to human readable string */
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

    ESP_LOGI(WebServer::tag, "connection has set ip: %s!", str_ip);
  }

  void WebServer::callBackDisconnect(void *)
  {
    StatusObject::setWlanState(WlanState::DISCONNECTED);
    http_app_set_handler_hook(HTTP_GET, nullptr);
  }

  esp_err_t WebServer::callbackGetHttpHandler(httpd_req_t *req)
  {
    std::string uri(req->uri);
    //
    // Aktion zum anzeigen der LED
    //
    StatusObject::setHttpActive(true);

    if ((uri.substr(0, 19)).compare("/api/v1/system/info") == 0)
    {
      WebServer::systemInfoGetHandler(req);
    }
    else if ((uri.substr(0, 9)).compare("/rest/v1/") == 0)
    {
      WebServer::restGetHandler(req);
    }
    else
    {
      WebServer::rootGetHandler(req);
    }
    // {
    //   /* send a 404 otherwise */
    //   httpd_resp_send_404(req);
    // }
    return ESP_OK;
  }

  esp_err_t WebServer::restGetHandler(httpd_req_t *req)
  {
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "current", IDF_VER);
    cJSON_AddNumberToObject(root, "temp", 99.9);
    const char *tmp_info = cJSON_Print(root);
    httpd_resp_sendstr(req, tmp_info);
    free((void *)tmp_info);
    cJSON_Delete(root);
    return ESP_OK;
  }

  esp_err_t WebServer::rootGetHandler(httpd_req_t *req)
  {
    /*https://github.com/espressif/esp-idf/blob/96b0cb2828fb550ab0fe5cc7a5b3ac42fb87b1d2/examples/protocols/http_server/file_serving/main/file_server.c
    212
    */
    static const std::string basePath{Prefs::WEB_PATH};
    std::string filePath;
    std::string uri(req->uri);

    FILE *fd = nullptr;
    struct stat file_stat;

    if (!is_spiffs_ready)
    {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SPIFFS not ready!");
      return ESP_FAIL;
    }

    // max pathlen check
    esp_err_t ret = getPathFromUri(filePath, basePath, uri);
    if (ret != ESP_OK)
    {
      ESP_LOGE(WebServer::tag, "Filename is too long");
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
      return ESP_FAIL;
    }
    //
    // check if filename not exits
    // its hardcodet path ?
    // do this after file check, so i can overwrite this
    // behavior if an file is exist
    //
    if (stat(filePath.c_str(), &file_stat) == -1)
    {
      //
      // If file not present on SPIFFS check if URI
      // corresponds to one of the hardcoded paths
      //
      if (uri.compare("/") == 0)
      {
        //
        // redirect to index.html
        //
        return WebServer::indexHtmlGetHandler(req);
      }
      //
      // so i not found file or hardcodet uri
      // make a failure message
      //
      ESP_LOGE(WebServer::tag, "failed to stat file : %s", filePath.c_str());
      httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "file does not exist");
      return ESP_FAIL;
    }
    //
    // try to open file, readonly of course
    //
    fd = fopen(filePath.c_str(), "r");
    if (!fd)
    {
      ESP_LOGE(WebServer::tag, "failed to read existing file : %s", filePath.c_str());
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return ESP_FAIL;
    }
    //
    // content-type from file find and set
    //
    ESP_LOGI(WebServer::tag, "sending file : <%s> (%ld bytes)...", filePath.c_str(), file_stat.st_size);
    // set content type of file
    std::string content{"text"};
    setContentTypeFromFile(content, req, filePath);
    //
    // deliver the file chunk-whise if too big for buffsize
    // create smart buffer
    //
    if (file_stat.st_size + 6 < Prefs::WEB_SCRATCH_BUFSIZE)
    {
      // one piece
      if (content.compare("json") == 0)
      {
        //
        // "[" and "]" for correct json file
        //
        std::unique_ptr<char> deliverBuf(new char[file_stat.st_size + 6]);
        char *buffptr = deliverBuf.get();
        *(buffptr++) = '[';
        *(buffptr++) = '\n';
        buffptr += fread(buffptr, 1, file_stat.st_size, fd);
        *(buffptr++) = ']';
        *(buffptr++) = '\n';
        *(buffptr++) = 0;
        httpd_resp_sendstr(req, deliverBuf.get());
      }
      else
      {
        std::unique_ptr<char> deliverBuf(new char[file_stat.st_size + 6]);
        char *buffptr = deliverBuf.get();
        // read in buffer
        buffptr += fread(deliverBuf.get(), 1, file_stat.st_size, fd);
        *(buffptr++) = '\0';
        httpd_resp_sendstr(req, deliverBuf.get());
      }
      ESP_LOGI(WebServer::tag, "file sending complete");
    }
    else
    {
      // send chunked
      std::unique_ptr<char> deliverBuf(new char[Prefs::WEB_SCRATCH_BUFSIZE]);
      // Retrieve the pointer to scratch buffer for temporary storage
      if (content.compare("json") == 0)
      {
        const char *startPtr{"[\n"};
        httpd_resp_send_chunk(req, startPtr, 2);
      }
      char *chunk = deliverBuf.get();
      size_t chunksize;
      do
      {
        //
        // Read file in chunks into the scratch buffer
        //
        chunksize = fread(chunk, 1, Prefs::WEB_SCRATCH_BUFSIZE, fd);
        //
        // if read any content
        //
        if (chunksize > 0)
        {
          //
          // Send the buffer contents as HTTP response chunk
          //
          if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK)
          {
            // is something wrong?
            fclose(fd);
            ESP_LOGE(WebServer::tag, "file sending failed!");
            // Abort sending file
            httpd_resp_sendstr_chunk(req, nullptr);
            // Respond with 500 Internal Server Error
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to send file");
            return ESP_FAIL;
          }
        }
        // Keep looping till the whole file is sent
      } while (chunksize != 0);
      // send signal "sending done!"
      if (content.compare("json") == 0)
      {
        const char *endPtr{"[\n]\n"};
        httpd_resp_send_chunk(req, endPtr, 3);
      }
      httpd_resp_sendstr_chunk(req, nullptr);
    }
    //
    // Close file after sending complete
    //
    fclose(fd);
    ESP_LOGI(WebServer::tag, "chunk file sending complete");
    return ESP_OK;
  }

  /* Simple handler for getting system handler */
  esp_err_t WebServer::systemInfoGetHandler(httpd_req_t *req)
  {
    httpd_resp_set_status(req, "200 OK");
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

  //
  // redirect from "/" to "/index.html"
  //
  esp_err_t WebServer::indexHtmlGetHandler(httpd_req_t *req)
  {
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    httpd_resp_send(req, NULL, 0); // Response body can be empty
    return ESP_OK;
  }

  /**
   * copy basepath and uri to the filepath in the filesystem
   * return in dest the filepath in the spiffs
   * max size is Prefs::FILEPATH_MAX_SIZE
  */

  esp_err_t WebServer::getPathFromUri(std::string &dest, const std::string &base_path, std::string &uri)
  {
    size_t pos{std::string::npos};

    pos = uri.find_first_of("?#", 0);
    if (pos != std::string::npos)
    {
      uri = uri.substr(0, pos);
    }

    if ((base_path.length() + uri.length()) > Prefs::WEB_FILEPATH_MAX_SIZE)
    {
      /* Full path string won't fit into destination buffer */
      dest.clear();
      return ESP_FAIL;
    }

    //
    // dest if reference so ist this the filepath after return
    //
    dest = base_path;
    dest += uri;
    return ESP_OK;
  }

  /* Set HTTP response content type according to file extension */
  esp_err_t WebServer::setContentTypeFromFile(std::string &type, httpd_req_t *req, const std::string &filename)
  {
    if (filename.rfind(".pdf") != std::string::npos)
    {
      type = "pdf";
      return httpd_resp_set_type(req, "application/pdf");
    }
    if (filename.rfind(".html") != std::string::npos)
    {
      type = "html";
      return httpd_resp_set_type(req, "text/html");
    }
    if (filename.rfind(".jpeg") != std::string::npos)
    {
      type = "jpeg";
      return httpd_resp_set_type(req, "image/jpeg");
    }
    if (filename.rfind(".ico") != std::string::npos)
    {
      type = "ico";
      return httpd_resp_set_type(req, "image/x-icon");
    }
    if (filename.rfind(".json") != std::string::npos)
    {
      type = "json";
      return httpd_resp_set_type(req, "application/json");
    }
    if (filename.rfind(".js") != std::string::npos)
    {
      type = "js";
      return httpd_resp_set_type(req, "text/javascript");
    }
    if (filename.rfind(".css") != std::string::npos)
    {
      type = "css";
      return httpd_resp_set_type(req, "text/css");
    }

    //
    // This is a limited set only
    // For any other type always set as plain text
    //
    type = "text";
    return httpd_resp_set_type(req, "text/plain");
  }

  void WebServer::time_sync_notification_cb(struct timeval *tv)
  {
    using namespace Prefs;
    sntp_sync_status_t state = sntp_get_sync_status();
    switch (state)
    {
    case SNTP_SYNC_STATUS_COMPLETED:
      ESP_LOGI(WebServer::tag, "notification: time status sync completed!");
      if (StatusObject::getWlanState() == WlanState::CONNECTED)
      {
        StatusObject::setWlanState(WlanState::TIMESYNCED);
      }
      break;
    default:
      ESP_LOGI(WebServer::tag, "notification: time status not synced!");
      if (StatusObject::getWlanState() == WlanState::TIMESYNCED)
      {
        StatusObject::setWlanState(WlanState::CONNECTED);
      }
    }
  }
}
