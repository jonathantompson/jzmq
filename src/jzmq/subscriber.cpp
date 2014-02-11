#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/subscriber.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  Subscriber::Subscriber(const std::string& conn_str) : 
    Connection(conn_str, SubscriberType){
  }

  Subscriber::~Subscriber() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void Subscriber::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("Subscriber::initConn() - ERROR: "
        "connection already initialized.");
    }
    void* context = Connection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_SUB);
    if (socket_ == NULL) {
      throwErrorMessage("Subscriber::initConn() - ERROR: "
        "Could not create ZMQ_SUB socket");
    }

    int rc = zmq_connect(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Subscriber::initConn() - ERROR: "
        "Could not connect ZMQ_SUB socket");
    }

    const char* filter = "";  // Don't filter anything
    rc = zmq_setsockopt(socket_, ZMQ_SUBSCRIBE, filter, strlen(filter));
    if (rc != 0) {
      throwErrorMessage("Subscriber::initConn() - ERROR: "
        "Could not subscribe ZMQ_SUB socket");
    }

    num_open_connections_++;
  }

  void Subscriber::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("Subscriber::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
