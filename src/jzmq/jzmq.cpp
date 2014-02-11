#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/jzmq.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

#if defined(_WIN32) || defined(WIN32)
  #ifndef snprintf
    #define snprintf _snprintf
  #endif
#endif

namespace jzmq {

  void* JZMQConnection::context_ = NULL;
  std::mutex JZMQConnection::context_lck_;
  std::atomic<int64_t> JZMQConnection::num_open_connections_ = 0;

  // ******************************** JZMQConn ********************************
  JZMQConnection::JZMQConnection(const std::string& conn_str, 
    const SocketType type) {
    conn_str_ = conn_str;
    type_ = type;
    socket_ = NULL;
  }

  void* JZMQConnection::initContext() {
    if (context_ != NULL) {
      // Context has already been initialized
      return context_;
    }
    // Otherwise initilize the context 
    std::unique_lock<std::mutex> lck(context_lck_);
    // Someone may have grabbed the lock and initialized the context while we
    // were waiting for the lock.  Check again.
    if (context_ != NULL) {
      return context_;
    }
    // Context has not been initialized so create one.
    context_ = zmq_ctx_new();
    if (context_ == NULL) {
      throwErrorMessage("Could not initialize zmq context");
    }
    return context_;
  }

  void JZMQConnection::throwErrorMessage(const std::string& err_msg) {
    int rc = zmq_errno();
    std::stringstream ss;
    ss << err_msg << " (errno=" << rc << ")";
    throw std::wruntime_error(ss.str());
  }

  void JZMQConnection::killContext() {
    if (context_ == NULL || num_open_connections_ > 0) {
      // Context has already been destroyed OR there are still open connections
      return;
    }
    // Otherwise destroy the context 
    std::unique_lock<std::mutex> lck(context_lck_);
    // Someone may have grabbed the lock and destroyed the context while we
    // were waiting for the lock.  Check again.
    if (context_ == NULL) {
      return;
    }
    // Otherwise Kill the context
    zmq_ctx_destroy(context_);
    context_ == NULL;
  }

  int JZMQConnection::recieveData(char* buff, const uint64_t buff_size, 
    const bool blocking) {
    const int flags = blocking ? 0 : ZMQ_DONTWAIT;
    int rc = zmq_recv(socket_, buff, buff_size, flags);
    if (blocking && rc < 0) {
      rc = zmq_errno();
      if (rc == EAGAIN) {
        // Normal operation.  Non-blocking was requested and no data was ready.
        return 0;
      }
    } else if (rc >= 0) {
      // A message was recieved
      return rc;
    } else {
      rc = zmq_errno();
    }
    std::stringstream ss;
    ss << "Error recieving data on ZMQ_REQ Socket.  errno=" << rc;
    throw std::wruntime_error(ss.str());
  }

  int JZMQConnection::sendData(char* buff, const uint64_t buff_size, 
    const bool blocking) {
    const int flags = blocking ? 0 : ZMQ_DONTWAIT;
    int rc = zmq_send(socket_, buff, buff_size, flags);
    if (blocking && rc < 0) {
      rc = zmq_errno();
      if (rc == EAGAIN) {
        // Normal operation.  Non-blocking was requested and message cannot be
        // queued.
        return 0;
      }
    } else if (rc >= 0) {
      // A message was queued
      return rc;
    } else {
      rc = zmq_errno();
    }
    std::stringstream ss;
    ss << "Error queuing data on ZMQ_REP Socket.  Error code: " << rc;
    throw std::wruntime_error(ss.str());
  }

  // ******************************* JZMQServer *******************************
  JZMQServer::JZMQServer(const std::string& conn_str) : 
    JZMQConnection(conn_str, Server){
  }

  JZMQServer::~JZMQServer() {
    assert(socket_ == NULL);
  }
  
  void JZMQServer::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("JZMQServer::initConn() - ERROR: connection "
        "already initialized.");
    }
    void* context = JZMQConnection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_XREP);
    if (socket_ == NULL) {
      throwErrorMessage("Could not create ZMQ_XREP socket");
    }

    int rc = zmq_bind(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Could not bind ZMQ_XREQ socket");
    }
    num_open_connections_++;
  }

  void JZMQServer::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("JZMQServer::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

  // ******************************* JZMQClient *******************************
  JZMQClient::JZMQClient(const std::string& conn_str) : 
    JZMQConnection(conn_str, Client){
  }

  JZMQClient::~JZMQClient() {
    // You must kill all connections before calling destructor!
    assert(socket_ == NULL);
  }
  
  void JZMQClient::initConn() {
    if (socket_ != NULL) {
      throw std::wruntime_error("JZMQClient::initConn() - ERROR: connection "
        "already initialized.");
    }
    void* context = JZMQConnection::initContext();
    
    socket_ = zmq_socket(context, ZMQ_XREQ);
    if (socket_ == NULL) {
      throwErrorMessage("Could not create ZMQ_XREQ socket");
    }

    int rc = zmq_connect(socket_, conn_str_.c_str());
    if (rc != 0) {
      throwErrorMessage("Could not connect ZMQ_XREQ socket");
    }
    num_open_connections_++;
  }

  void JZMQClient::killConn() {
    if (socket_ == NULL) {
      throw std::wruntime_error("JZMQServer::killConn() - ERROR: "
        "Socket has not been initialized!");
    }
    zmq_close(socket_);
    socket_ = NULL;
    num_open_connections_--;
    killContext();
  }

}  // namespace jzmq
