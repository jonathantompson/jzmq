#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/server.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  Server::Server(const std::string& conn_str) : 
    Connection(conn_str, ServerType){
  }

  Server::~Server() {
    if (socket_ != NULL) {
      // A socket might not close correctly on a fatal error condition
      // Don't throw an exception or raise an assertion, but let the user know.
      std::cout << "Warning: Socket was not closed!" << std::endl;
    }
  }
  
  void Server::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("Server::initConn() - ERROR: connection "
        "already initialized.");
    }
    void* context = Connection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_REP);
    if (socket_ == NULL) {
      throwErrorMessage("Server::initConn() - ERROR: "
        "Could not create ZMQ_REP socket");
    }

    int rc = zmq_bind(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Server::initConn() - ERROR: "
        "Could not bind ZMQ_REP socket");
    }
    num_open_connections_++;
  }

  void Server::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("Server::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
