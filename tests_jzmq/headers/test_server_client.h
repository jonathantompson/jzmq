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
#include "jzmq/jzmq.h"
#include "jtil/clk/clk.h"
#include "jtil/exceptions/wruntime_error.h"

using namespace jtil::math;
using namespace jtil::clk;
using namespace jzmq;

#define PING_TIME_SEC 2
#define NUM_CLIENTS 4

bool server_running = false;
std::atomic<uint64_t> n_clients_running = 0;
uint64_t n_server_pings = 0;
uint64_t n_client_pings[NUM_CLIENTS];
std::thread clients[NUM_CLIENTS];

void ServerThread() {
  Clk clk;
  
  // Start the ZeroMQ server
  JZMQServer server("tcp://localhost:5558");
  server.initConn();

  server_running = true;
  double t0 = clk.getTime();
  while (clk.getTime() - t0 < PING_TIME_SEC) {
    if (n_clients_running < NUM_CLIENTS) {
      t0 = clk.getTime();  // Keep updating the clock until clients are started
    }
    // TODO: Wait for reponses
  }
  server_running = false;
  // BUSY wait until the clients are finished
  while (n_clients_running > 0) { }
  server.killConn();
}

void ClientThread(const uint32_t i) {
  n_clients_running++;
  while (server_running) {
    // TODO: Ping server for response
  }
  n_clients_running--;
}

TEST(JZMQTests, SimpleServerClient) {
  // Spawn a server thread
  std::thread server(ServerThread);
  // Busy wait until the server has started
  while (!server_running) { }
  // Spawn a client 
  
  for (uint64_t i = 0; i < NUM_CLIENTS; i++) {
    n_client_pings[i] = 0;  // Reset the ping counter
    clients[i] = std::thread(ClientThread, 0);
  }

  // Wait for the server to finish.  Note: Server only finishes once all
  // the clients are finished.
  server.join();

  // We ran the simulation long enough for at least a few pings from the
  // server to the clients
  EXPECT_TRUE(n_server_pings > 0);
  for (uint64_t i = 0; i < NUM_CLIENTS; i++) {
    EXPECT_TRUE(n_client_pings[i] > 0);
  }
}
