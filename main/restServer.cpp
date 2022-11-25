#include <sys/param.h>
#include <esp_vfs.h>
#include <esp_http_server.h>
#include <cJSON.h>
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_spiffs.h"
#include "restServer.hpp"

namespace rest_webserver
{
  const char *RestServer::tag{"RestServer"}; //! tag fürs debug logging
  bool RestServer::is_spiffs_ready{false};

  /**
   * initialize the server process
   */
  void RestServer::init()
  {
    ESP_LOGI(RestServer::tag, "RestServer init...");
    RestServer::spiffs_init();
  }

  esp_err_t RestServer::spiffs_init()
  {
    ESP_LOGI(RestServer::tag, "Initializing SPIFFS...");

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
        ESP_LOGE(RestServer::tag, "Failed to mount or format filesystem");
      }
      else if (ret == ESP_ERR_NOT_FOUND)
      {
        ESP_LOGE(RestServer::tag, "Failed to find SPIFFS partition");
      }
      else
      {
        ESP_LOGE(RestServer::tag, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
      }
    }
    else
    {
      RestServer::is_spiffs_ready = true;
      ESP_LOGI(RestServer::tag, "Initializing SPIFFS...OK");
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(nullptr, &total, &used);
    if (ret != ESP_OK)
    {
      ESP_LOGE(RestServer::tag, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
      ESP_LOGI(RestServer::tag, "Partition size: total: %d, used: %d", total, used);
    }
    return ret;
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
    web_server_context_t *rest_context = static_cast<web_server_context_t *>(calloc(1, sizeof(web_server_context_t)));
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

    RestServer::registerSystemInfoHandler(server, rest_context);
    RestServer::registerRestHandler(server, rest_context);
    RestServer::registerRootHandler(server, rest_context);

    return ESP_OK;
  }

  esp_err_t RestServer::registerRestHandler(httpd_handle_t server, web_server_context_t *rest_context)
  {
    // URI handler for REST api
    httpd_uri_t system_info_get_uri;
    system_info_get_uri.uri = "/rest/v1/*";
    system_info_get_uri.method = HTTP_GET;
    system_info_get_uri.handler = RestServer::restGetHandler;
    system_info_get_uri.user_ctx = rest_context;
    httpd_register_uri_handler(server, &system_info_get_uri);
    return ESP_OK;
  }

  esp_err_t RestServer::restGetHandler(httpd_req_t *req)
  {
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

  esp_err_t RestServer::registerRootHandler(httpd_handle_t server, web_server_context_t *rest_context)
  {
    /* URI handler for fetching system info */
    httpd_uri_t root_index_uri;
    root_index_uri.uri = "/*";
    root_index_uri.method = HTTP_GET;
    root_index_uri.handler = RestServer::rootGetHandler;
    root_index_uri.user_ctx = rest_context;
    httpd_register_uri_handler(server, &root_index_uri);
    return ESP_OK;
  }

  esp_err_t RestServer::rootGetHandler(httpd_req_t *req)
  {
    /*https://github.com/espressif/esp-idf/blob/96b0cb2828fb550ab0fe5cc7a5b3ac42fb87b1d2/examples/protocols/http_server/file_serving/main/file_server.c
    212
    */
    char filepath[FILE_PATH_MAX];
    FILE *fd = nullptr;
    struct stat file_stat;

    if (!is_spiffs_ready)
    {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "SPIFFS not ready!");
      return ESP_FAIL;
    }

    // max pathlen check
    const char *filename = getPathFromUri(filepath, ((struct webServerContext *)req->user_ctx)->base_path,
                                          req->uri, sizeof(filepath));
    if (!filename)
    {
      ESP_LOGE(RestServer::tag, "Filename is too long");
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
      return ESP_FAIL;
    }

    //
    // check if filename exits
    // or its hardcodet path
    //
    if (stat(filepath, &file_stat) == -1)
    {
      /* If file not present on SPIFFS check if URI
         * corresponds to one of the hardcoded paths */
      if (strcmp(filename, "/") == 0)
      {
        return indexHtmlGetHandler(req);
      }
      ESP_LOGE(RestServer::tag, "Failed to stat file : %s", filepath);
      /* Respond with 404 Not Found */
      httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
      return ESP_FAIL;
    }
    //
    // try to open file, readonly of course
    //
    fd = fopen(filepath, "r");
    if (!fd)
    {
      ESP_LOGE(RestServer::tag, "Failed to read existing file : %s", filepath);
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
      return ESP_FAIL;
    }
    //
    // content-type from file find and set
    //
    ESP_LOGI(RestServer::tag, "Sending file : <%s> (%ld bytes)...", filename, file_stat.st_size);
    //httpd_resp_set_type(req, "text/html");
    setContentTypeFromFile(req, filename);
    //
    // deliver the file chunk-whise
    //

    // Retrieve the pointer to scratch buffer for temporary storage
    char *chunk = ((struct webServerContext *)req->user_ctx)->scratch;
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
          // was is wrong?
          fclose(fd);
          ESP_LOGE(RestServer::tag, "File sending failed!");
          // Abort sending file
          httpd_resp_sendstr_chunk(req, nullptr);
          // Respond with 500 Internal Server Error
          httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
          return ESP_FAIL;
        }
      }
      // Keep looping till the whole file is sent
    } while (chunksize != 0);
    httpd_resp_sendstr_chunk(req, nullptr);
    //
    // Close file after sending complete
    //
    fclose(fd);
    ESP_LOGI(RestServer::tag, "File sending complete");
    return ESP_OK;
  }

  esp_err_t RestServer::registerSystemInfoHandler(httpd_handle_t server, web_server_context_t *rest_context)
  {
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

  //
  // redirect from "/" to "/index.html"
  //
  esp_err_t RestServer::indexHtmlGetHandler(httpd_req_t *req)
  {
    httpd_resp_set_status(req, "307 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/index.html");
    httpd_resp_send(req, NULL, 0); // Response body can be empty
    return ESP_OK;
  }

  /* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
  const char *RestServer::getPathFromUri(char *dest, const char *base_path, const char *uri, size_t destsize)
  {
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest)
    {
      pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash)
    {
      pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize)
    {
      /* Full path string won't fit into destination buffer */
      return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
  }

  /* Set HTTP response content type according to file extension */
  esp_err_t RestServer::setContentTypeFromFile(httpd_req_t *req, const char *filename)
  {
    if (RestServer::isFileTypeExist(filename, ".pdf"))
    {
      return httpd_resp_set_type(req, "application/pdf");
    }
    else if (RestServer::isFileTypeExist(filename, ".html"))
    {
      return httpd_resp_set_type(req, "text/html");
    }
    else if (RestServer::isFileTypeExist(filename, ".jpeg"))
    {
      return httpd_resp_set_type(req, "image/jpeg");
    }
    else if (RestServer::isFileTypeExist(filename, ".ico"))
    {
      return httpd_resp_set_type(req, "image/x-icon");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
  }

}
