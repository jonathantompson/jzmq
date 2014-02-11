#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/publisher.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  Publisher::Publisher(const std::string& conn_str) : 
    Connection(conn_str, PublisherType){
  }

  Publisher::~Publisher() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void Publisher::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("Publisher::initConn() - ERROR: "
        "connection already initialized.");
    }
    void* context = Connection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_PUB);
    if (socket_ == NULL) {
      throwErrorMessage("Publisher::initConn() - ERROR: "
        "Could not create ZMQ_PUB socket");
    }

    int rc = zmq_bind(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Publisher::initConn() - ERROR: "
        "Could not bind ZMQ_PUB socket");
    }
    num_open_connections_++;
  }

  void Publisher::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("Publisher::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
