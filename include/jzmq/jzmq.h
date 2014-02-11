//
//  jzmq.h
//
//  Created by Jonathan Tompson on 2/10/14.
//
//  I have created 2 frameworks.  A Server/Client and a Publisher/Subscriber
//  framework.  The Server client handles arbitrary 2 way communication between
//  a single server and a number of clients.  The Publisher/Subscriber
//  is used for 1 way communcation between a server and a number of clients.
//
//  For usage examples see the test code (test_jzmq in this repo).
//
//  NOTE: When you receive string data from ØMQ in C, you simply cannot trust 
//  that it's safely terminated. Every time you read a string, you should 
//  allocate a new buffer with space for an extra byte, copy the string, and 
//  terminate it properly with a null.

#pragma once

#include <atomic>
#include <string>
#include <mutex>
#include "jtil/math/math_types.h"

namespace jzmq {

  // Pure virtual base class for all our ZMQ classes.
  // Use the child classes to create instances of the JZMQ sockets and call
  // methods in this class to send and recieve data.
  class JZMQConnection {
  public:
    typedef enum {
      Server,
      Client,
      Publisher,
      Subscriber
    } SocketType;

    // Initialize a JZMQ instance.  conn_str usage:
    // JZMQ("tcp://192.168.0.1:5557", Client);  // TCP socket at IP:port
    // JZMQ("tcp://localhost:5558", Server);    // TCP socket at localhost:port
    JZMQConnection(const std::string& conn_str, const SocketType type);

    virtual void initConn() = 0;  // NOT thread safe
    virtual void killConn() = 0;  // NOT thread safe
    virtual ~JZMQConnection() { }

    // recieveData is by default blocking until a data is recieved.  Returns 
    // the length of data recieved to buff in bytes.  Note that the length can 
    // be greater than the buffer size (in which case the message is truncated 
    // into buff).
    int recieveData(char* buff, const uint64_t buff_size, 
      const bool blocking = true);  // NOT thread safe
    // sendData will queue the contents of buffer in a blocking fashion by 
    // default.  Note: If sucessful, this does not indicate that the data was
    // sent to the network; just that it was sucessfully queued to be sent.
    // Returns number of bytes sent.  If non-blocking and the message cannot
    // be queued, then sendData will return 0.
    int sendData(char* buff, const uint64_t buff_size, 
      const bool blocking = true);  // NOT thread safe

  protected:
    std::string conn_str_;
    SocketType type_;
    void* socket_;
    static std::atomic<int64_t> num_open_connections_;

    // All child classes should create a context through this interface.
    // Threads on the same process can share a context which makes message
    // passing faster.  This provides a seamless mechanism to share contexts.
    static void* initContext();  // Thread Safe
    static void killContext();  // Thread safe

    // This function will just query the zmq API for the error condition and
    // throw a std::wruntime_error.
    static void throwErrorMessage(const std::string& err_msg);

  private:
    static void* context_;
    static std::mutex context_lck_;
  };

  // The server class (to be paired with JZMQClient)
  class JZMQServer : public JZMQConnection {
  public:
    JZMQServer(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQServer();
  };

  // The client class (to be paired with JZMQServer)
  class JZMQClient : public JZMQConnection {
  public:
    JZMQClient(const std::string& conn_str);
    virtual void initConn();
    virtual void killConn();
    virtual ~JZMQClient();
  };

  // TODO: Write the publisher subscriber model

};  // namespace jzmq
