/*
  Copyright (c) 2019 Akshay Vernekar

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#include "web_thing_adapter.h"
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <mdns.h>

#include <esp_http_server.h>

static Thing* gThing=NULL;
static const char* MDNS_INSTANCE_NAME = "webthing";
static const char* REST_TAG ="web_thing_adapter";

static void initialise_mdns(char* instance_name)
{
	if(gThing == NULL)
		return;

	mdns_init();
	mdns_hostname_set(MDNS_INSTANCE_NAME);
	mdns_instance_name_set(instance_name);

	mdns_txt_item_t serviceTxtData[] = {
		{"path", "/"}
	};

	ESP_ERROR_CHECK(mdns_service_add("webthing", "_webthing", "_tcp", CONFIG_WEB_THING_PORT, serviceTxtData,
									 sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

esp_err_t handleGetThing(httpd_req_t *req)
{
	Thing* device = NULL;
	cJSON* responseJson = NULL;
	if(req->user_ctx != NULL)
	{
		device = (Thing*)req->user_ctx;
	}
	else
	{
		ESP_LOGI(REST_TAG,"user_ctx is null");
	}

	if(device)
	{
		responseJson = serializeDevice(device);
		const char* strRes = cJSON_Print(responseJson);		
		httpd_resp_set_type(req, "application/json");
		httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
		httpd_resp_send(req, strRes, strlen(strRes));

		//cleanup
		free(strRes);
		cJSON_Delete(responseJson);
	}
    return ESP_OK;
}

esp_err_t handleThingGetItem(httpd_req_t *req)
{
	ThingProperty* property = NULL;
	cJSON* responseJson = NULL;
	if(req->user_ctx != NULL)
	{
		property = (ThingProperty*)req->user_ctx;
	}
	if(property)
	{
		responseJson = serialise_property_item(property);
		const char* strRes = cJSON_Print(responseJson);
		httpd_resp_set_type(req, "application/json");
		httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
		httpd_resp_send(req, strRes, strlen(strRes));

		//cleanup
		free(strRes);
		cJSON_Delete(responseJson);
	}
    return ESP_OK;
}

esp_err_t handleThingPutItem(httpd_req_t *req)
{
	ThingProperty* property = NULL;
	cJSON* responseJson = NULL;
	esp_err_t resCode = ESP_OK;
	if(req->user_ctx != NULL)
	{
		property = (ThingProperty*)req->user_ctx;
	}

	if(property)
	{
		char* content = malloc(sizeof(char)*(req->content_len+1));
		cJSON *newvalue = NULL;
    	int ret = httpd_req_recv(req, content, req->content_len);
    	content[req->content_len] = '\0';
	    if (ret <= 0) 
	    {  /* 0 return value indicates connection closed */
	        /* Check if timeout occurred */
	        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
	            /* In case of timeout one can choose to retry calling
	             * httpd_req_recv(), but to keep it simple, here we
	             * respond with an HTTP 408 (Request Timeout) error */
	            httpd_resp_send_408(req);
	        }
	        /* In case of error, returning ESP_FAIL will
	         * ensure that the underlying socket is closed */
	        resCode = ESP_FAIL;
	        goto cleanup;
	    }
	    newvalue = cJSON_Parse((char *)content);
        if(newvalue == NULL)
        {
            ESP_LOGI(REST_TAG,"handle_directives::Parsing failed");
            resCode = ESP_FAIL;
	        goto cleanup;
        }
		if(update_thing_property(property,newvalue))
		{
			httpd_resp_set_type(req, "application/json");
			httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
			httpd_resp_send(req, content, strlen(content));
		}
cleanup:
		if(newvalue)
			cJSON_Delete(newvalue);

		if(content)
			free(content);
	}
    return resCode;
}

esp_err_t handleThingGetAllProperties(httpd_req_t *req)
{
	ESP_LOGI(REST_TAG,"handleThingGetAllProperties hit");
	Thing* thing = NULL;
	cJSON* responseJson = NULL;
	if(req->user_ctx != NULL)
	{
		thing = (Thing*)req->user_ctx;
	}
	else
	{
		return ESP_FAIL;
	}


    ThingProperty* property = thing->property;
    responseJson = = cJSON_CreateObject();
	while (property != NULL) 
	{
		cJSON_AddItemToObject(responseJson,get_property_keyname(property),serialise_property_item(property));
		property = (ThingProperty*)property->next;
	}
	const char* strRes = cJSON_Print(responseJson);
	httpd_resp_set_type(req, "application/json");
	httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
	httpd_resp_send(req, strRes, strlen(strRes));

	//cleanup
	free(strRes);
	cJSON_Delete(responseJson);
	
    return ESP_OK;
}

void startRestAPIServer(Thing* thing)
{
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	/*
	 	3 handles for "/","/things" and "/things/properties" 
		2 handlers for each property one GET and one PUT 
		We are considering max 3 properties , so MAX_URI_HANDLERS = 3+ 2*<MAX_NUMBER_OF_PROPERTIES> 
		you can increase this depending upon your usecase 
	*/
	config.max_uri_handlers = CONFIG_MAX_PROPERTY*2 + 3;
	config.server_port = CONFIG_WEB_THING_PORT;

	ESP_LOGI(REST_TAG, "Starting webthing Server");
	
	if(httpd_start(&server, &config) != ESP_OK)
	{
		ESP_LOGI(REST_TAG,"server start failed!");
		return;
	}

	/* URI handler for fetching system info */
	httpd_uri_t system_info_get_uri = {
		.uri = "/",
		.method = HTTP_GET,
		.handler = handleGetThing,
		.user_ctx = thing
	};
	httpd_register_uri_handler(server, &system_info_get_uri);

	sprintf(&request_handle.uri,"/things/%s",gThing->id);
	system_info_get_uri.method = HTTP_GET;
	system_info_get_uri.handler = handleGetThing;
	system_info_get_uri.user_ctx = thing;
	httpd_register_uri_handler(server, &system_info_get_uri);

    ThingProperty* property = thing->property;
    httpd_uri_t request_handle;
	while (property != NULL) 
	{
		char* propUri = getPropertyEndpointUrl(thing,property);
		ESP_LOGI(REST_TAG,"url:%s",propUri);
		request_handle.uri = propUri;
		request_handle.method = HTTP_GET;
		request_handle.handler = handleThingGetItem;
		request_handle.user_ctx = property;
		httpd_register_uri_handler(server, &request_handle);

		if(!property->info.readOnly)
		{
			request_handle.uri = propUri;
			request_handle.method = HTTP_PUT;
			request_handle.handler = handleThingPutItem;
			request_handle.user_ctx = property;
			httpd_register_uri_handler(server, &request_handle);	
		}
		
		if(propUri)
		{
			free(propUri);
			propUri = NULL;
		}
		property = (ThingProperty*)property->next;
	}

	// request_handle.uri ="/things/properties";
	sprintf(&request_handle.uri,"/things/%s/properties",gThing->id);
	request_handle.method = HTTP_GET;
	request_handle.handler = handleThingGetAllProperties;
	request_handle.user_ctx = thing;
	httpd_register_uri_handler(server, &request_handle);

}

void initAdapter(Thing* thing)
{
	gThing = thing;
	initialise_mdns(gThing->title);
}

void startAdapter()
{
	startRestAPIServer(gThing);
}
