//
//  test_math.cpp
//
//  Created by Jonathan Tompson on 2/11/14.
//

#include <atomic>
#include <thread>
#include "jtil/math/math_types.h"
#include "jtil/math/math_base.h"
#include "jtil/threading/thread.h"
#include "jzmq/jzmq_publisher.h"
#include "jzmq/jzmq_subscriber.h"
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
namespace pub_sub_test {
  const int test_time_sec = 1;
  const int ping_sleep_ms = 1;
  const uint32_t num_subscribers = 4;
  const uint32_t buffer_len = 512;
  std::atomic<uint64_t> n_errors = 0;
  std::atomic<uint64_t> n_publisher_pings = 0;
  std::atomic<uint64_t> n_subscriber_pings[num_subscribers];
  std::thread subscribers[num_subscribers];

  // Publisher side method to send data.
  bool publishData(JZMQPublisher& publisher, char* buffer) {
    // Check if any data was recieved
    uint64_t timeout_ms = 1000 * test_time_sec;  // blocking with timeout

    snprintf(buffer, buffer_len-1, "Hello Subscriber");
    int bytes_sent = publisher.sendData(buffer, strlen(buffer), timeout_ms);
    if (bytes_sent == 0) {
      std::cout << "Could not publish data!" << std::endl;
      n_errors++;
      return false;
    } 
    return true;  // Data was sent
  }

  void PublisherThread() {
    Clk clk;
    char* buffer = new char[buffer_len];
    jtil::threading::SetThreadName("Publisher Thread");

    try {
      // Start the ZeroMQ Publisher
      JZMQPublisher publisher("tcp://*:5558");
      publisher.initConn();

      double t0 = clk.getTime();
      while ((clk.getTime() - t0) < test_time_sec) {
        // Broadcast data to all the subscribers
        bool message_recieved = publishData(publisher, buffer);
        if (message_recieved) {
          n_publisher_pings++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(ping_sleep_ms));
      }

      // Shut down the publisher
      publisher.killConn();
    } catch (std::wruntime_error& e) {
      std::cout << "Exception caught while running test! " << std::endl;
      std::cout << "  " << ToNarrowString(e.errorMsg()) << std::endl;
      n_errors++;
    }

    delete[] buffer;
  }

  // Subscriber side method to receive data.
  bool receiveData(JZMQSubscriber& subscriber, char* buffer) {
    uint64_t timeout_ms = 0;  // Non-blocking
    int bytes_recieved = subscriber.receiveData(buffer, buffer_len-1, 
      timeout_ms);
    if (bytes_recieved > 0) {
      buffer[std::min<int>(bytes_recieved, buffer_len-1)] = '\0';
      if (buffer == std::string("Hello Subscriber")) {
        return true;
      } else {
        std::cout << "Incorrect Message received!" << std::endl;
        n_errors++;
      }
    }
    // No data was received yet OR the incorrect message was received
    return false;
  }

  void SubscriberThread(const uint32_t i) {
    Clk clk;
    char* buffer = new char[buffer_len];
    snprintf(buffer, buffer_len-1, "Subscriber thread %d", i);
    jtil::threading::SetThreadName(buffer);

    try {
      // Start the ZeroMQ Subscriber
      JZMQSubscriber subscriber("tcp://localhost:5558");
      subscriber.initConn();

      double t0 = clk.getTime();
      while ((clk.getTime() - t0) < test_time_sec) {
        // Get a message from the publisher
        bool message_receieved = receiveData(subscriber, buffer);
        if (message_receieved) {
          n_subscriber_pings[i]++;
        }
        std::this_thread::yield();
      }

      subscriber.killConn();
    } catch (std::wruntime_error& e) {
      std::cout << "Exception caught while running test! " << std::endl;
      std::cout << "  " << ToNarrowString(e.errorMsg()) << std::endl;
      n_errors++;
    }

    delete[] buffer;
  }

};  // namespace pub_sub_test

TEST(JZMQTests, SimplePublisherSubscriber) {
  // Spawn a bunch of clients
  for (uint64_t i = 0; i < pub_sub_test::num_subscribers; i++) {
    pub_sub_test::n_subscriber_pings[i] = 0;  // Reset the ping counter
    pub_sub_test::subscribers[i] = std::thread(pub_sub_test::SubscriberThread, 
      i);
  }

  // Spawn a Publisher thread
  std::thread publisher(pub_sub_test::PublisherThread);

  // Wait for the publisher and clients to finish
  for (uint64_t i = 0; i < pub_sub_test::num_subscribers; i++) {
    pub_sub_test::subscribers[i].join();
  }
  publisher.join();

  // We ran the simulation long enough for at least a few pings from the
  // server to the clients
  EXPECT_TRUE(pub_sub_test::n_errors == 0);
  EXPECT_TRUE(pub_sub_test::n_publisher_pings > 0);
  for (uint64_t i = 0; i < pub_sub_test::num_subscribers; i++) {
    EXPECT_TRUE(pub_sub_test::n_subscriber_pings[i] > 0);
  }
}
