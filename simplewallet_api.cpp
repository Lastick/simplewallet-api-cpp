
// Copyright (c) 2019 The Karbowanec developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl\curl.h>
#include <jansson.h>
#include "simplewallet_api.h"


char *strcopy(const char *source){
  char *res;
  size_t str_size = strlen(source);
  res = (char*) malloc((str_size + 1) * sizeof(char));
  strncpy(res, source, (str_size + 1) * sizeof(char));
  return res;
}

char *strinit(const unsigned int str_size){
  char *res;
  res = (char*) malloc((size_t) (str_size + 1) * sizeof(char));
  memset(res, 0x00, (size_t) (str_size + 1) * sizeof(char));
  return res;
}

char *numtostr(const int num){
  const size_t num_size = 15;
  char *res;
  res = (char*) malloc((size_t) (num_size + 1) * sizeof(char));
  memset(res, 0x00, (size_t) (num_size + 1) * sizeof(char));
  sprintf(res, "%d", num);
  return res;
}

void free_mem(void *ptr){
  free(ptr);
}

const char *SimplewalletAPI::default_api_host = "127.0.0.1";
const char *SimplewalletAPI::default_api_path = "/";
const unsigned int SimplewalletAPI::default_api_port = 15000;
const bool SimplewalletAPI::default_api_ssl = false;
const char *SimplewalletAPI::user_agent = "SimplewalletAPI/1.0";
const char *SimplewalletAPI::id_conn = "EWF8aIFX0y9w";
const char *SimplewalletAPI::rpc_v = "2.0";

SimplewalletAPI::SimplewalletAPI(){
  this->api_host = strcopy(SimplewalletAPI::default_api_host);
  this->api_path = strcopy(SimplewalletAPI::default_api_path);
  this->api_port = SimplewalletAPI::default_api_port;
  this->api_ssl = SimplewalletAPI::default_api_ssl;
  this->res_status = false;
}

SimplewalletAPI::SimplewalletAPI(const char *host, const char *path, const unsigned port, const bool ssl){
  this->api_host = strcopy(host);
  this->api_path = strcopy(path);
  this->api_port = port;
  this->api_ssl = ssl;
  this->res_status = false;
}

SimplewalletAPI::SimplewalletAPI(const SimplewalletAPI &obj){
  this->api_host = strcopy(obj.api_host);
  this->api_path = strcopy(obj.api_path);
  this->api_port = obj.api_port;
  this->api_ssl = obj.api_ssl;
  this->res_status = false;
}

void SimplewalletAPI::operator= (const SimplewalletAPI &obj){
  free_mem((void *) this->api_host);
  free_mem((void *) this->api_path);
  this->api_host = strcopy(obj.api_host);
  this->api_path = strcopy(obj.api_path);
  this->api_port = obj.api_port;
  this->api_ssl = obj.api_ssl;
}

SimplewalletAPI::~SimplewalletAPI(){
  free_mem((void *) this->api_host);
  free_mem((void *) this->api_path);
}

size_t SimplewalletAPI::WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb;
  SimplewalletAPI::MemoryStruct * mem = (struct MemoryStruct *) userp;
  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL){
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}

char *SimplewalletAPI::client(const char *data){
  const char *http_prefix = "http://";
  const char *https_prefix = "https://";
  const char *sep_port = ":";
  const char *uri_target = "/json_rpc";
  char *api_uri = strinit(8 + strlen(this->api_host) + 1 + 15 + 9);
  char *str_api_port = numtostr(this->api_port);
  if (this->api_ssl){
    strcat(api_uri, https_prefix);
  } else {
    strcat(api_uri, http_prefix);
  }
  strcat(api_uri, this->api_host);
  strcat(api_uri, sep_port);
  strcat(api_uri, str_api_port);
  strcat(api_uri, uri_target);
  CURL *curl;
  CURLcode res;
  curl_slist *list = NULL;
  SimplewalletAPI::MemoryStruct chunk;
  chunk.memory = (char*) malloc(1);
  chunk.size = 0;
  list = curl_slist_append(list, "Content-Type: application/json");
  this->api_status = false;
  curl = curl_easy_init();
  if(curl){
    curl_easy_setopt(curl, CURLOPT_URL, api_uri);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(data));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, SimplewalletAPI::user_agent);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, SimplewalletAPI::WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
    res = curl_easy_perform(curl);
    curl_slist_free_all(list);
    if(res == CURLE_OK){
      if (chunk.size > 0) this->api_status = true;
	  } else {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
    curl_easy_cleanup(curl);
  }
  free_mem((void *) api_uri);
  free_mem((void *) str_api_port);
  return chunk.memory;
}

void SimplewalletAPI::getHeight(unsigned int &height){
  const char *rpc_method = "get_height";
  height = 0;
  json_t *json_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_t *height_obj;
  json_error_t error;
  json_obj = json_object();
  json_object_set(json_obj, "jsonrpc", json_string(SimplewalletAPI::rpc_v));
  json_object_set(json_obj, "id", json_string(SimplewalletAPI::id_conn));
  json_object_set(json_obj, "method", json_string(rpc_method));
  this->res_status = false;
  char *json_res = this->client(json_dumps(json_obj, 0));
  json_object_clear(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if(json_obj){
      if (json_is_object(json_obj)){
        id_obj = json_object_get(json_obj, "id");
        if (json_is_string(id_obj)){
          if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
            result_obj = json_object_get(json_obj, "result");
            if (json_is_object(result_obj)){
              height_obj = json_object_get(result_obj, "height");
              if (json_is_integer(height_obj)){
                height = (unsigned int) json_number_value(height_obj);
                json_object_clear(height_obj);
              }
              json_object_clear(result_obj);
            }
          }
          json_object_clear(id_obj);
        }
      }
      json_object_clear(json_obj);
    }
  }
  free_mem((void *) json_res);
}

bool SimplewalletAPI::getStatus(){
  return this->res_status;
}