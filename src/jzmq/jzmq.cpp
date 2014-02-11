#include <mutex>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <zmq.h>
#include "jzmq/jzmq.h"
#include "jtil/exceptions/wruntime_error.h"

#define SAFE_DELETE(x) if (x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARR(x) if (x != NULL) { delete[] x; x = NULL; }

namespace jzmq {

  void* JZMQConnection::context_ = NULL;
  std::mutex JZMQConnection::context_lck_;
  std::atomic<int64_t> JZMQConnection::num_open_connections_ = 0;

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
    ss << err_msg << " (zmqerr=" << zmq_strerror(rc) << ")";
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
    context_ = NULL;
  }

  int JZMQConnection::receiveData(char* buff, const uint64_t buff_size, 
    const int timout_ms) {
    if (type_ == Publisher) {
      throw std::wruntime_error("JZMQConnection::receive() - ERROR: "
        "A Publisher is trying to receive data (they can only send data).");
    }

    // Poll the socket for a reply, with timeout.  If ZMQ_POLLIN is in the
    // revent, then we're gaurenteed at least one message may be receive
    // without blocking.
    zmq_pollitem_t items [] = {{socket_, 0, ZMQ_POLLIN, 0}};
    int rc = zmq_poll(items, 1, timout_ms);
    if (rc == -1) {
      // Interrupt received
      return 0;
    }

    if (items[0].revents & ZMQ_POLLIN) {
      int rc = zmq_recv(socket_, buff, buff_size, 0);
      if (rc >= 0) {
        return rc;
      } else {
        rc = zmq_errno();
      }
      std::stringstream ss;
      ss << "Error receiving data on Socket.  " << zmq_strerror(rc);
      throw std::wruntime_error(ss.str());
    }
    return 0;
  }

  int JZMQConnection::sendData(char* buff, const uint64_t buff_size, 
    const int timout_ms) {
    if (type_ == Subscriber) {
      throw std::wruntime_error("JZMQConnection::sendData() - ERROR: "
        "A Subscriber is trying to send data (they can only receive data).");
    }
    // Poll the socket for an empty queue, with timeout.  If ZMQ_POLLOUT is in 
    // the revent, then we're gaurenteed at least one message may be sent
    // without blocking.
    zmq_pollitem_t items [] = {{socket_, 0, ZMQ_POLLOUT, 0}};
    int rc = zmq_poll(items, 1, timout_ms);
    if (rc == -1) {
      // Interrupt received
      return 0;
    }

    if (items[0].revents & ZMQ_POLLOUT) {
      int rc = zmq_send(socket_, buff, buff_size, 0);
      if (rc >= 0) {
        return rc;
      } else {
        rc = zmq_errno();
      }
      std::stringstream ss;
      ss << "Error sending data on Socket.  " << zmq_strerror(rc);
      throw std::wruntime_error(ss.str());
    }
    return 0;
  }
}  // namespace jzmq
