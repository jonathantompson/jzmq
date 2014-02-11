#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/jzmq_client.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  JZMQClient::JZMQClient(const std::string& conn_str) : 
    JZMQConnection(conn_str, Client){
  }

  JZMQClient::~JZMQClient() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void JZMQClient::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("JZMQClient::initConn() - ERROR: connection "
        "already initialized.");
    }
    void* context = JZMQConnection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_REQ);
    if (socket_ == NULL) {
      throwErrorMessage("JZMQClient::initConn() - ERROR: "
        "Could not create ZMQ_REQ socket");
    }

    int rc = zmq_connect(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("JZMQClient::initConn() - ERROR: "
        "Could not connect ZMQ_REQ socket");
    }
    num_open_connections_++;
  }

  void JZMQClient::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("JZMQClient::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
