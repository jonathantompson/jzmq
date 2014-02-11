#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/jzmq_subscriber.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  JZMQSubscriber::JZMQSubscriber(const std::string& conn_str) : 
    JZMQConnection(conn_str, Subscriber){
  }

  JZMQSubscriber::~JZMQSubscriber() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void JZMQSubscriber::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("JZMQSubscriber::initConn() - ERROR: "
        "connection already initialized.");
    }
    void* context = JZMQConnection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_SUB);
    if (socket_ == NULL) {
      throwErrorMessage("JZMQSubscriber::initConn() - ERROR: "
        "Could not create ZMQ_SUB socket");
    }

    int rc = zmq_connect(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("JZMQSubscriber::initConn() - ERROR: "
        "Could not connect ZMQ_SUB socket");
    }

    const char* filter = "";  // Don't filter anything
    rc = zmq_setsockopt(socket_, ZMQ_SUBSCRIBE, filter, strlen(filter));
    if (rc != 0) {
      throwErrorMessage("JZMQSubscriber::initConn() - ERROR: "
        "Could not subscribe ZMQ_SUB socket");
    }

    num_open_connections_++;
  }

  void JZMQSubscriber::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("JZMQSubscriber::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
