#include <iostream>
#include <istream>
#include <regex>
#include <string>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>

using boost::asio::ip::tcp;
namespace asio = boost::asio;

/** Server TCP port. */
static constexpr unsigned PORT = 20001;

/** Handy exception that accepts a boost::format object. */
class format_exception : public std::runtime_error {
public:
  format_exception(const std::string& message)
    : std::runtime_error(message)
  {}

  format_exception(const boost::format& message)
    : format_exception(boost::str(message))
  {}
};

/** Class responsible of handling a session.
 *
 * In a session a sensor will first send identification information,
 * and then it will proceed to send the samples.
 */
class session {
public:
  /** Constructor.
   *
   * @param socket The TCP socket.
   */
  session(tcp::socket socket)
    : socket_{std::move(socket)}
  {}

  /** Handles the session. */
  void run() {
    auto sensor_id = get_sensor_id();
    std::cout << boost::format("sensor: %1%\n") % sensor_id;
  }

private:
  /** The TCP socket to communicate with the sensor. */
  tcp::socket socket_;

  /** Buffer to receive data from the sensor. */
  asio::streambuf buffer_;
 
  /** Reads a line from the socket.
   *
   * @return The line read.
   */ 
  std::string read_line() {
    asio::read_until(socket_, buffer_, '\n');
    std::istream input(&buffer_);
    std::string line;
    std::getline(input, line);
    boost::trim(line);
    return line;
  }

  /** Reads the sensor ID.
   *
   * @return The sensor ID.
   */
  std::string get_sensor_id() {
    auto line = read_line();
    std::smatch match;
    if (!std::regex_match(line, match, std::regex("sensor-id: (\\w+)"))
        || match.size() != 2)
      throw format_exception(
          boost::format("Error reading sensor id: %1%") % line);
    return match[1].str();
  }
};

/** Starts a session to communicate with a sensor.
 *
 * @param socket TCP socket associated with the sensor.
 */
static void start_session(tcp::socket socket) {
  try {
    std::cout << "Session started\n";
    session s(std::move(socket));
    s.run();
    std::cout << "Session ended\n";
  } catch (const std::exception& e) {
    std::cerr << boost::format("Error: %1%\n") % e.what();
  } catch (...) {
    std::cerr << "Unknown error\n";
  }
}

/** Waits for connections from sensors.
 *
 * @param io_service Asio IO service.
 */
static void server(asio::io_service& io_service) {
  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), PORT));
  while (true) {
    tcp::socket socket(io_service);
    acceptor.accept(socket);
    std::thread(start_session, std::move(socket)).detach();
  }
}

int main() {
  try {
    asio::io_service io_service;
    server(io_service);
  } catch (const std::exception& e) {
    std::cerr << boost::format("Error: %1%\n") % e.what();
  } catch (...) {
    std::cerr << "Unknown error\n";
  }
}

