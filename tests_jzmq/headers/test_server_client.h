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
#include "jzmq/jzmq.h"
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

#define TEST_TIME_SEC 1
#define TEST_TIMEOUT_SEC 10
#define PING_SLEEP_MSEC 1
#define NUM_CLIENTS 1

std::atomic<uint64_t> n_clients_running = 0;
std::atomic<uint64_t> n_errors = 0;
std::atomic<uint64_t> n_server_pings = 0;
std::atomic<uint64_t> n_client_pings[NUM_CLIENTS];
std::thread clients[NUM_CLIENTS];

// Server side method to recieve and acknowledge requests.
bool serviceRequest(JZMQServer& server, char* buffer, const char* response) {
  // Check if any data was recieved (blocking)
  bool blocking = false;
  int bytes_recieved = server.recieveData(buffer, 255, blocking);
  if (bytes_recieved > 0) {
    // Null terminate the string because ZeroMQ does not do this for us
    buffer[std::max<int>(bytes_recieved,255)] = '\0';
    // Check the message that was recieved
    if (buffer != std::string("Hello server")) {
      n_errors++;
    }
    // Send back a reply (This Server must ALWAYS send a reply).
    snprintf(buffer, 255, response);
    server.sendData(buffer, strnlen(buffer, 255), blocking);
    return true;
  } 
  return false;  // No message was recieved
}

void ServerThread() {
  Clk clk;
  char* buffer = new char[256];
  jtil::threading::SetThreadName("Server Thread");

  try {
    // Start the ZeroMQ server
    JZMQServer server("tcp://*:5558");
    server.initConn();

    double t0 = clk.getTime();
    while ((clk.getTime() - t0) < TEST_TIME_SEC) {
      if (n_clients_running < NUM_CLIENTS) {
        t0 = clk.getTime();  // Keep updating the clock until clients started
      }
      // Check if any data was recieved (non-blocking) and send a dummy reply
      bool message_recieved = serviceRequest(server, buffer, "Hello client");
      if (message_recieved) {
        n_server_pings++;
      }
      std::this_thread::yield();
    }

    // Reply "KILL" to all client messages, causing them to quit
    while (n_clients_running > 0 && (clk.getTime() - t0) < TEST_TIMEOUT_SEC) {
      bool message_recieved = serviceRequest(server, buffer, "KILL");
      n_server_pings++;
      std::this_thread::yield();
    }

    if (n_clients_running > 0) {
      std::cout << "Some clients never exited!" << std::endl;
      exit(-1);  // Force an exit (the library is potentially stuck).
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
  snprintf(buffer, 255, "Hello server");
  int bytes_sent = client.sendData(buffer, 255);
  buffer[0] = '\0';
  if (bytes_sent == 0) {
    return 0;  // No message was sent
  } else {
    // The message was sent to the server so we should get a response
    int bytes_recieved = client.recieveData(buffer, 255);
    if (bytes_recieved == 0) {
       n_errors++;
    } else {
      // Null terminate the string because ZeroMQ does not do this for us
      buffer[std::min<int>(bytes_recieved,255)] = '\0';
    }
  }

  // Check the message that was recieved
  if (buffer == std::string("Hello client")) {
    return 1;
  } else if (buffer == std::string("KILL")) { 
    return 2;
  } else {
    n_errors++;
    return 0;
  }
}

void ClientThread(const uint32_t i) {
  char* buffer = new char[256];
  snprintf(buffer, 255, "Client thread %d", i);
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
      std::this_thread::sleep_for(std::chrono::milliseconds(PING_SLEEP_MSEC));
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

TEST(JZMQTests, SimpleServerClient) {
  n_clients_running = NUM_CLIENTS;

  // Spawn a server thread
  std::thread server(ServerThread);
  // Spawn a client 
  for (uint64_t i = 0; i < NUM_CLIENTS; i++) {
    n_client_pings[i] = 0;  // Reset the ping counter
    clients[i] = std::thread(ClientThread, 0);
  }


  // Wait for the server and clients to finish
  for (uint64_t i = 0; i < NUM_CLIENTS; i++) {
    clients[i].join();
  }
  server.join();

  // We ran the simulation long enough for at least a few pings from the
  // server to the clients
  EXPECT_TRUE(n_errors == 0);
  EXPECT_TRUE(n_server_pings > 0);
  for (uint64_t i = 0; i < NUM_CLIENTS; i++) {
    EXPECT_TRUE(n_client_pings[i] > 0);
  }
}
