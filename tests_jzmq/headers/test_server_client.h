//
//  test_math.cpp
//
//  Created by Jonathan Tompson on 3/14/12.
//
//  C++ code to implement test_math.m --> Use matlab to check C results
//  Typical code will not use templates directly, but I do so here so
//  that I can switch out float for doubles and test both cases
//
//  LOTS of tests here.  There's a lot of code borrowed from Prof 
//  Alberto Lerner's code repository (I took his distributed computing class, 
//  which was amazing), particularly the test unit stuff.

#include <atomic>
#include <thread>
#include "jtil/math/math_types.h"
#include "jtil/math/math_base.h"
#include "jtil/threading/thread.h"
#include "jzmq/jzmq_server.h"
#include "jzmq/jzmq_client.h"
#include "jtil/clk/clk.h"
#include "jtil/exceptions/wruntime_error.h"
#include "jtil/string_util/string_util.h"

#if defined(_WIN32) || defined(WIN32)
  #ifndef snprintf
    #define snprintf _snprintf
  #endif
#endif

using namespace jtil::math;
using namespace jtil::clk;
using namespace jtil::string_util;
using namespace jzmq;

// Invoke a new namespace to keep test data separate 
namespace ser_cnt_test {
  const int test_time_sec = 1;
  const int test_timeout_sec = 10;
  const int ping_sleep_ms = 1;
  const uint32_t num_clients = 4;
  const uint32_t buffer_len = 512;

  std::atomic<uint64_t> n_clients_running = 0;
  std::atomic<uint64_t> n_errors = 0;
  std::atomic<uint64_t> n_server_pings = 0;
  std::atomic<uint64_t> n_client_pings[num_clients];
  std::thread clients[num_clients];

  // Server side method to recieve and acknowledge requests.
  bool serviceRequest(JZMQServer& server, char* buffer, const char* response) {
    // Check if any data was recieved
    uint64_t timeout_ms = 0;  // Non-blocking
    int bytes_recieved = server.receiveData(buffer, buffer_len-1, timeout_ms);
    if (bytes_recieved > 0) {
      // Null terminate the string because ZeroMQ does not do this for us
      buffer[std::max<int>(bytes_recieved,buffer_len-1)] = '\0';
      // Check the message that was recieved
      if (buffer != std::string("Hello server")) {
        std::cout << "Incorrect (or missing) message!" << std::endl;
        n_errors++;
      }
      // Send back a reply (This Server must ALWAYS send a reply).
      snprintf(buffer, buffer_len-1, response);
      server.sendData(buffer, strnlen(buffer, buffer_len-1), timeout_ms);
      return true;
    } 
    return false;  // No message was recieved
  }

  void ServerThread() {
    Clk clk;
    char* buffer = new char[buffer_len];
    jtil::threading::SetThreadName("Server Thread");

    try {
      // Start the ZeroMQ server
      JZMQServer server("tcp://*:5558");
      server.initConn();

      double t0 = clk.getTime();
      while ((clk.getTime() - t0) < test_time_sec) {
        // Check if any data was recieved (non-blocking) and send a dummy reply
        bool message_recieved = serviceRequest(server, buffer, "Hello client");
        if (message_recieved) {
          n_server_pings++;
        }
        std::this_thread::yield();
      }

      // Reply "KILL" to all client messages, causing them to quit (note that
      // we could have also used non-blocking read-requests on the client side,
      // killed the server and then just let the clients timeout, but this
      // tests the 2 way communication).
      while (n_clients_running > 0 && (clk.getTime()-t0) < test_timeout_sec) {
        bool message_recieved = serviceRequest(server, buffer, "KILL");
        n_server_pings++;
        std::this_thread::yield();
      }

      if (n_clients_running > 0) {
        std::cout << "Some clients never exited!" << std::endl;
        n_errors++;
      }

      // Shut down the server
      server.killConn();
    } catch (std::wruntime_error& e) {
      std::cout << "Exception caught while running test! " << std::endl;
      std::cout << "  " << ToNarrowString(e.errorMsg()) << std::endl;
      n_errors++;
    }

    delete[] buffer;
  }

  // Client side method to send and recieve requests.
  int sendRequest(JZMQClient& client, char* buffer) {
    snprintf(buffer, buffer_len-1, "Hello server");
    int bytes_sent = client.sendData(buffer, buffer_len-1);
    buffer[0] = '\0';
    if (bytes_sent == 0) {
      return 0;  // No message was sent
    } else {
      // The message was sent to the server so we should get a response
      int bytes_recieved = client.receiveData(buffer, buffer_len-1);
      if (bytes_recieved == 0) {
         std::cout << "Could not get server response!" << std::endl;
         n_errors++;
      } else {
        // Null terminate the string because ZeroMQ does not do this for us
        buffer[std::min<int>(bytes_recieved,buffer_len-1)] = '\0';
      }
    }

    // Check the message that was recieved
    if (buffer == std::string("Hello client")) {
      return 1;
    } else if (buffer == std::string("KILL")) { 
      return 2;
    } else {
      std::cout << "Incorrect (or missing) message!" << std::endl;
      n_errors++;
      return 0;
    }
  }

  void ClientThread(const uint32_t i) {
    char* buffer = new char[buffer_len];
    snprintf(buffer, buffer_len-1, "Client thread %d", i);
    jtil::threading::SetThreadName(buffer);

    try {
      // Start the ZeroMQ client
      JZMQClient client("tcp://localhost:5558");
      client.initConn();

      int server_message;
      do {
        // Send a message to the server
        server_message = sendRequest(client, buffer);
        if (server_message != 0) {
          n_client_pings[i]++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ping_sleep_ms));
      } while (server_message != 2);

      client.killConn();
    } catch (std::wruntime_error& e) {
      std::cout << "Exception caught while running test! " << std::endl;
      std::cout << "  " << ToNarrowString(e.errorMsg()) << std::endl;
      n_errors++;
    }

    delete[] buffer;
    n_clients_running--;
  }
};  // namespace ser_cnt_test

TEST(JZMQTests, SimpleServerClient) {
  ser_cnt_test::n_clients_running = ser_cnt_test::num_clients;

  // Spawn a server thread
  std::thread server(ser_cnt_test::ServerThread);

  // Spawn a bunch of clients
  for (uint64_t i = 0; i < ser_cnt_test::num_clients; i++) {
    ser_cnt_test::n_client_pings[i] = 0;  // Reset the ping counter
    ser_cnt_test::clients[i] = std::thread(ser_cnt_test::ClientThread, i);
  }

  // Wait for the server and clients to finish
  for (uint64_t i = 0; i < ser_cnt_test::num_clients; i++) {
    ser_cnt_test::clients[i].join();
  }
  server.join();

  // We ran the simulation long enough for at least a few pings from the
  // server to the clients
  EXPECT_TRUE(ser_cnt_test::n_clients_running == 0);
  EXPECT_TRUE(ser_cnt_test::n_errors == 0);
  EXPECT_TRUE(ser_cnt_test::n_server_pings > 0);
  for (uint64_t i = 0; i < ser_cnt_test::num_clients; i++) {
    EXPECT_TRUE(ser_cnt_test::n_client_pings[i] > 0);
  }
}
