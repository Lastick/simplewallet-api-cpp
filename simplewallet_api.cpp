
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
#include <math.h>
#include <time.h>
#include <string.h>
#include <curl/curl.h>
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

char *genURI(const char *host, const unsigned &port, const char *path, const bool &ssl){
  const char *http_prefix = "http://";
  const char *https_prefix = "https://";
  const char *sep_port = ":";
  const char *uri_target = "/json_rpc";
  char *res = strinit(8 + strlen(host) + 1 + 15 + 9);
  char *str_api_port = numtostr(port);
  if (ssl){
    strcat(res, https_prefix);
  } else {
    strcat(res, http_prefix);
  }
  strcat(res, host);
  strcat(res, sep_port);
  strcat(res, str_api_port);
  strcat(res, uri_target);
  free_mem((void *) str_api_port);
  return res;
}

void BalanceRebase(double &balance_frl,
                   long long &balance_ntv,
                   const unsigned int &decimal_point,
                   const unsigned int &prec_point,
                   bool mode){

  if (mode){
    balance_ntv = 0;
    if (balance_frl > 0){
      balance_ntv = (long long) floor(balance_frl * pow((double) 10, (double) decimal_point) + 0.5);
    }
  } else {
    balance_frl = 0.0;
    if (balance_ntv > 0){
      balance_frl = (double) floor(balance_ntv / pow((double) 10, (double) decimal_point - prec_point) + 0.5) /
                                                 pow((double) 10, (double) prec_point);
    }
  }

}

char *genPID(){
  const unsigned int pid_len = 32;
  char *pid = strinit(pid_len * 2);
  unsigned char byte_pid = 0x00;
  srand((unsigned int) time(NULL));
  for (unsigned int n = 0; n < pid_len; n++){
    byte_pid = rand() % 255;
    sprintf(pid + n * 2, "%02X", byte_pid);
  }
  return pid;
};

const char *SimplewalletAPI::default_api_host = "127.0.0.1";
const char *SimplewalletAPI::default_api_path = "/";
const unsigned int SimplewalletAPI::default_api_port = 15000;
const bool SimplewalletAPI::default_api_ssl = false;
const char *SimplewalletAPI::user_agent = "SimplewalletAPI/1.0";
const char *SimplewalletAPI::id_conn = "EWF8aIFX0y9w";
const char *SimplewalletAPI::rpc_v = "2.0";
const unsigned int SimplewalletAPI::decimal_point = 12;
const unsigned int SimplewalletAPI::prec_point = 4;

SimplewalletAPI::SimplewalletAPI(){
  this->api_host = strcopy(SimplewalletAPI::default_api_host);
  this->api_path = strcopy(SimplewalletAPI::default_api_path);
  this->api_port = SimplewalletAPI::default_api_port;
  this->api_ssl = SimplewalletAPI::default_api_ssl;
  this->res_status = false;
}

SimplewalletAPI::SimplewalletAPI(const char *host, const unsigned port, const bool ssl){
  this->api_host = strcopy(host);
  this->api_path = strcopy(SimplewalletAPI::default_api_path);
  this->api_port = port;
  this->api_ssl = ssl;
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
  CURL *curl;
  CURLcode res;
  curl_slist *list = NULL;
  SimplewalletAPI::MemoryStruct chunk;
  chunk.memory = (char*) malloc(1);
  chunk.size = 0;
  list = curl_slist_append(list, "Content-Type: application/json");
  char *api_uri = genURI(this->api_host, this->api_port, this->api_path, this->api_ssl);
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
  return chunk.memory;
}

void SimplewalletAPI::getHeight(unsigned int &height){
  const char *rpc_method = "get_height";
  height = 0;
  json_t *json_obj;
  json_t *error_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_t *height_obj;
  json_error_t error;
  json_obj = json_pack("{ssssss}",
                       "jsonrpc", SimplewalletAPI::rpc_v,
                       "id", SimplewalletAPI::id_conn,
                       "method", rpc_method);
  char *json_req;
  json_req = json_dumps(json_obj, JSON_INDENT(2));
  this->res_status = false;
  char *json_res = this->client(json_req);
  free_mem((void *) json_req);
  json_object_clear(json_obj);
  json_decref(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if (json_obj){
      if (json_is_object(json_obj)){
        error_obj = json_object_get(json_obj, "error");
        if (json_is_object(error_obj)){
          json_object_clear(error_obj);
          this->res_status = false;
        } else {
          id_obj = json_object_get(json_obj, "id");
          if (json_is_string(id_obj)){
            if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
              result_obj = json_object_get(json_obj, "result");
              if (json_is_object(result_obj)){
                height_obj = json_object_get(result_obj, "height");
                if (json_is_integer(height_obj)){
                  height = (unsigned int) json_number_value(height_obj);
                  this->res_status = true;
                  json_object_clear(height_obj);
                }
                json_object_clear(result_obj);
              }
            }
            json_object_clear(id_obj);
          }
        }
      }
      json_object_clear(json_obj);
    }
  }
  json_decref(json_obj);
  free_mem((void *) json_res);
}

void SimplewalletAPI::getBalance(double &available_balance, double &locked_amount){
  const char *rpc_method = "getbalance";
  available_balance = 0;
  locked_amount = 0;
  long long amount_buff = 0;
  unsigned int status_n = 0;
  json_t *json_obj;
  json_t *error_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_t *available_balance_obj;
  json_t *locked_amount_obj;
  json_error_t error;
  json_obj = json_pack("{ssssss}",
                       "jsonrpc", SimplewalletAPI::rpc_v,
                       "id", SimplewalletAPI::id_conn,
                       "method", rpc_method);
  char *json_req;
  json_req = json_dumps(json_obj, JSON_INDENT(2));
  this->res_status = false;
  char *json_res = this->client(json_req);
  free_mem((void *) json_req);
  json_object_clear(json_obj);
  json_decref(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if (json_obj){
      if (json_is_object(json_obj)){
        error_obj = json_object_get(json_obj, "error");
        if (json_is_object(error_obj)){
          json_object_clear(error_obj);
          this->res_status = false;
        } else {
          id_obj = json_object_get(json_obj, "id");
          if (json_is_string(id_obj)){
            if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
              result_obj = json_object_get(json_obj, "result");
              if (json_is_object(result_obj)){
                available_balance_obj = json_object_get(result_obj, "available_balance");
                if (json_is_integer(available_balance_obj)){
                  amount_buff = 0;
                  amount_buff = (long long) json_integer_value(available_balance_obj);
                  BalanceRebase(available_balance,
                                amount_buff,
                                SimplewalletAPI::decimal_point,
                                SimplewalletAPI::prec_point,
                                false);
                  status_n++;
                  json_object_clear(available_balance_obj);
                }
                locked_amount_obj = json_object_get(result_obj, "locked_amount");
                if (json_is_integer(locked_amount_obj)){
                  amount_buff = 0;
                  amount_buff = (long long) json_integer_value(locked_amount_obj);
                  BalanceRebase(locked_amount,
                                amount_buff,
                                SimplewalletAPI::decimal_point,
                                SimplewalletAPI::prec_point,
                                false);
                  status_n++;
                  json_object_clear(locked_amount_obj);
                }
                json_object_clear(result_obj);
                if (status_n == 2) this->res_status = true;
              }
            }
            json_object_clear(id_obj);
          }
        }
      }
      json_object_clear(json_obj);
    }
  }
  json_decref(json_obj);
  free_mem((void *) json_res);
}

void SimplewalletAPI::doReset(){
  const char *rpc_method = "reset";
  json_t *json_obj;
  json_t *error_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_error_t error;
  json_obj = json_pack("{ssssss}",
                       "jsonrpc", SimplewalletAPI::rpc_v,
                       "id", SimplewalletAPI::id_conn,
                       "method", rpc_method);
  char *json_req;
  json_req = json_dumps(json_obj, JSON_INDENT(2));
  this->res_status = false;
  char *json_res = this->client(json_req);
  free_mem((void *) json_req);
  json_object_clear(json_obj);
  json_decref(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if (json_obj){
      if (json_is_object(json_obj)){
        error_obj = json_object_get(json_obj, "error");
        if (json_is_object(error_obj)){
          json_object_clear(error_obj);
          this->res_status = false;
        } else {
          id_obj = json_object_get(json_obj, "id");
          if (json_is_string(id_obj)){
            if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
              result_obj = json_object_get(json_obj, "result");
              if (json_is_object(result_obj)){
                this->res_status = true;
                json_object_clear(result_obj);
              }
            }
            json_object_clear(id_obj);
          }
        }
      }
      json_object_clear(json_obj);
    }
  }
  json_decref(json_obj);
  free_mem((void *) json_res);
}

void SimplewalletAPI::getTransfers(std::vector<Transfer> &transfers){
  const char *rpc_method = "get_transfers";
  transfers.clear();
  json_t *json_obj;
  json_t *error_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_t *transfers_obj;
  json_t *transfer_obj;
  json_t *amount_obj;
  long long amount_buff = 0;
  json_t *transactionHash_obj;
  json_t *output_obj;
  size_t transfer_n = 0;
  json_error_t error;
  json_obj = json_pack("{ssssss}",
                       "jsonrpc", SimplewalletAPI::rpc_v,
                       "id", SimplewalletAPI::id_conn,
                       "method", rpc_method);
  char *json_req;
  json_req = json_dumps(json_obj, JSON_INDENT(2));
  this->res_status = false;
  char *json_res = this->client(json_req);
  free_mem((void *) json_req);
  json_object_clear(json_obj);
  json_decref(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if (json_obj){
      if (json_is_object(json_obj)){
        error_obj = json_object_get(json_obj, "error");
        if (json_is_object(error_obj)){
          json_object_clear(error_obj);
          this->res_status = false;
        } else {
          id_obj = json_object_get(json_obj, "id");
          if (json_is_string(id_obj)){
            if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
              result_obj = json_object_get(json_obj, "result");
              if (json_is_object(result_obj)){
                transfers_obj = json_object_get(result_obj, "transfers");
                if (json_is_array(transfers_obj)){
                  transfer_n = json_array_size(transfers_obj);
                  if (transfer_n > 0){
                    transfers.reserve(transfer_n);
                    transfers.resize(transfer_n);
                    for (size_t n = 0; n < transfer_n; n++){
                      transfers[n].amount = 0;
                      transfers[n].paymentId = "";
                      transfers[n].output = false;
                      transfer_obj = json_array_get(transfers_obj, n);
                      if (json_is_object(transfer_obj)){
                        amount_obj = json_object_get(transfer_obj, "amount");
                        if (json_is_integer(amount_obj)){
                          amount_buff = 0;
                          amount_buff = (long long) json_integer_value(amount_obj);
                          BalanceRebase(transfers[n].amount,
                                        amount_buff,
                                        SimplewalletAPI::decimal_point,
                                        SimplewalletAPI::prec_point,
                                        false);
                          json_object_clear(amount_obj);
                        }
                        transactionHash_obj = json_object_get(transfer_obj, "transactionHash");
                        if (json_is_string(transactionHash_obj)){
                          transfers[n].paymentId = std::string(json_string_value(transactionHash_obj));
                          json_object_clear(transactionHash_obj);
                        }
                        output_obj = json_object_get(transfer_obj, "output");
                        if (json_is_boolean(output_obj)){
                          if (json_is_true(output_obj)){
                            transfers[n].output = true;
                          } else {
                            transfers[n].output = false;
                          }
                        }
                        json_object_clear(transfer_obj);
                      }
                    }
                  }
                  this->res_status = true;
                  json_object_clear(transfers_obj);
                }
                json_object_clear(result_obj);
              }
            }
            json_object_clear(id_obj);
          }
        }
      }
      json_object_clear(json_obj);
    }
  }
  json_decref(json_obj);
  free_mem((void *) json_res);
}

void SimplewalletAPI::doTransfer(std::vector<Destination> &destinations, std::string &payment_id, std::string &tx_hash){
  const char *rpc_method = "transfer";
  tx_hash.clear();
  json_t *json_obj;
  json_t *params_obj;
  json_t *destination_obj;
  long long amount;
  json_t *destinations_obj;
  json_t *error_obj;
  json_t *id_obj;
  json_t *result_obj;
  json_t *tx_hash_obj;
  json_error_t error;
  destinations_obj = json_array();
  for (unsigned int n = 0; n < destinations.size(); n++){
    BalanceRebase(destinations[n].amount,
                  amount,
                  SimplewalletAPI::decimal_point,
                  SimplewalletAPI::prec_point,
                  true);
    destination_obj = json_pack("{s:s, s:I}", "address", destinations[n].address.c_str(), "amount", amount);
    json_array_append(destinations_obj, destination_obj);
    json_decref(destination_obj);
  }

  params_obj = json_pack("{s:i, s:i, s:s, s:I, s:o}",
                         "mixin", 0,
                         "unlock_time", 0,
                         "payment_id", payment_id.c_str(),
                         "fee", 100000000000L,
                         "destinations", destinations_obj);

  json_obj = json_pack("{s:s, s:s, s:s, s:o}",
                       "jsonrpc", SimplewalletAPI::rpc_v,
                       "id", SimplewalletAPI::id_conn,
                       "method", rpc_method,
                       "params", params_obj);

  char *json_req;
  json_req = json_dumps(json_obj, JSON_INDENT(2));
  this->res_status = false;
  char *json_res = this->client(json_req);
  free_mem((void *) json_req);
  json_object_clear(json_obj);
  json_decref(json_obj);
  if (this->api_status){
    json_obj = json_loads(json_res, 0, &error);
    if (json_obj){
      if (json_is_object(json_obj)){
        error_obj = json_object_get(json_obj, "error");
        if (json_is_object(error_obj)){
          json_object_clear(error_obj);
          this->res_status = false;
        } else {
          id_obj = json_object_get(json_obj, "id");
          if (json_is_string(id_obj)){
            if (strcmp(json_string_value(id_obj), SimplewalletAPI::id_conn) == 0){
              result_obj = json_object_get(json_obj, "result");
              if (json_is_object(result_obj)){
                tx_hash_obj = json_object_get(result_obj, "tx_hash");
                if (json_is_string(tx_hash_obj)){
                  tx_hash = std::string(json_string_value(tx_hash_obj));
                  json_object_clear(tx_hash_obj);
                  this->res_status = true;
                }
                json_object_clear(result_obj);
              }
            }
            json_object_clear(id_obj);
          }
        }
      }
      json_object_clear(json_obj);
    }
  }
  json_decref(json_obj);
  free_mem((void *) json_res);
}

void SimplewalletAPI::getPID(std::string &pid){
  pid.clear();
  char *pid_str = genPID();
  pid = std::string(pid_str);
  free_mem((void *) pid_str);
}

bool SimplewalletAPI::getStatus(){
  return this->res_status;
}
