
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

#ifndef SIMPLEWALLET_API_H_INCLUDED
#define SIMPLEWALLET_API_H_INCLUDED

#include <string>
#include <vector>

struct Transfer {
  double amount;
  std::string paymentId;
  bool output;
};

class SimplewalletAPI {

  public:
    SimplewalletAPI();
    SimplewalletAPI(const char *host, const char *path, const unsigned port, const bool ssl);
    SimplewalletAPI(const SimplewalletAPI &obj);
    ~SimplewalletAPI();
    void operator= (const SimplewalletAPI &obj);
    void getHeight(unsigned int &height);
    void getBalance(double &available_balance, double &locked_amount);
    void doReset();
    void getTransfers(std::vector<Transfer> &transfers);
    bool getStatus();

  private:
    struct MemoryStruct {
      char *memory;
      size_t size;
    };
    static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
    char *api_host;
    char *api_path;
    unsigned int api_port;
    bool api_ssl;
    static const char *default_api_host;
    static const char *default_api_path;
    static const char *user_agent;
    static const char *id_conn;
    static const char *rpc_v;
    static const unsigned int default_api_port;
    static const unsigned int decimal_point;
    static const unsigned int prec_point;
    static const bool default_api_ssl;
    char *client(const char *data);
    bool api_status;
    bool res_status;

};

#endif // SIMPLEWALLET_API_H_INCLUDED
