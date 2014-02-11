#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/jzmq_publisher.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  JZMQPublisher::JZMQPublisher(const std::string& conn_str) : 
    JZMQConnection(conn_str, Publisher){
  }

  JZMQPublisher::~JZMQPublisher() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void JZMQPublisher::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("JZMQPublisher::initConn() - ERROR: "
        "connection already initialized.");
    }
    void* context = JZMQConnection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_REP);
    if (socket_ == NULL) {
      throwErrorMessage("JZMQPublisher::initConn() - ERROR: "
        "Could not create ZMQ_REP socket");
    }

    int rc = zmq_bind(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("JZMQPublisher::initConn() - ERROR: "
        "Could not bind ZMQ_REP socket");
    }
    num_open_connections_++;
  }

  void JZMQPublisher::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("JZMQPublisher::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
