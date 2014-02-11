//
//  client.h
//
//  Created by Jonathan Tompson on 2/10/14.
//
//  Please see jzmq_connection.h for API documentation.
//

#pragma once

#include <atomic>
#include <string>
#include <mutex>
#include "jtil/math/math_types.h"
#include "jzmq/connection.h"

namespace jzmq {

  // The Client class (to be paired with Server)
  class Client : public Connection {
  public:
    Client(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~Client();

  private:
    // Non-copyable, non-assignable.
    Client(Client&);
    Client& operator=(const Client&);
  };

};  // namespace jzmq
