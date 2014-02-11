//
//  publisher.h
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

  // The Publisher class (to be paired with Subscriber)
  class Publisher : public Connection {
  public:
    Publisher(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~Publisher();

  private:
    // Non-copyable, non-assignable.
    Publisher(Publisher&);
    Publisher& operator=(const Publisher&);
  };

};  // namespace jzmq
