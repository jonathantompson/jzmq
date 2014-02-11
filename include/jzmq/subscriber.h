//
//  subscriber.h
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

  // The Subscriber class (to be paired with Publisher)
  class Subscriber : public Connection {
  public:
    Subscriber(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~Subscriber();

  private:
    // Non-copyable, non-assignable.
    Subscriber(Subscriber&);
    Subscriber& operator=(const Subscriber&);
  };

};  // namespace jzmq
