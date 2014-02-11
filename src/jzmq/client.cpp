#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/client.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  Client::Client(const std::string& conn_str) : 
    Connection(conn_str, ClientType){
  }

  Client::~Client() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Client::~Client() - Warning: Socket was not "
        "closed!" << std::endl;
    }
  }
  
  void Client::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("Client::initConn() - ERROR: connection "
        "already initialized.");
    }
    void* context = Connection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_REQ);
    if (socket_ == NULL) {
      throwErrorMessage("Client::initConn() - ERROR: "
        "Could not create ZMQ_REQ socket");
    }

    int rc = zmq_connect(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Client::initConn() - ERROR: "
        "Could not connect ZMQ_REQ socket");
    }
    num_open_connections_++;
  }

  void Client::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("Client::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
