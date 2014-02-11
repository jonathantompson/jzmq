//
//  jzmq_client.h
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
#include "jzmq/jzmq_connection.h"

namespace jzmq {

  // The client class (to be paired with JZMQServer)
  class JZMQClient : public JZMQConnection {
  public:
    JZMQClient(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQClient();
  };

};  // namespace jzmq
