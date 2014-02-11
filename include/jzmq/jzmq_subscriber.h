//
//  jzmq_subscriber.h
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

  // The subscriber class (to be paired with JZMQPublisher)
  class JZMQSubscriber : public JZMQConnection {
  public:
    JZMQSubscriber(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQSubscriber();
  };

};  // namespace jzmq
