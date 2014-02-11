//
//  jzmq_publisher.h
//
//  Created by Jonathan Tompson on 2/10/14.
//
//  Please see jzmq.h for API documentation.
//

#pragma once

#include <atomic>
#include <string>
#include <mutex>
#include "jtil/math/math_types.h"
#include "jzmq/jzmq.h"

namespace jzmq {

  // The publisher class (to be paired with JZMQSubscriber)
  class JZMQPublisher : public JZMQConnection {
  public:
    JZMQPublisher(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQPublisher();
  };

};  // namespace jzmq
