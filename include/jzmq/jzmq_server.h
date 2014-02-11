//
//  jzmq_server.h
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

  // The server class (to be paired with JZMQClient)
  // Note: all communication with this server must be an alternating sequence
  // of 1. recieveData() --> 2. sendData().  If you try and send data in any
  // other order an exception will be thrown.  ZeroMQ does have a mechanism
  // for arbitrary message patterns, however sticking to this one is very
  // robust.
  class JZMQServer : public JZMQConnection {
  public:
    JZMQServer(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQServer();
  };

};  // namespace jzmq
