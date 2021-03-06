#include <iostream>
#include <istream>
#include <regex>
#include <string>
#include <thread>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

namespace asio = boost::asio;
namespace ptime = boost::posix_time;
using boost::asio::ip::tcp;

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
    auto msg = read_message();
    std::cout << msg.to_str() << std::endl;
  }

private:
  struct message {
    std::string sensor_id;
    ptime::ptime time;
    std::vector<unsigned> samples;

    std::string to_str() const {
      std::vector<std::string> samples_str(samples.size());
      std::transform(begin(samples), end(samples), begin(samples_str),
          [](unsigned s) { return std::to_string(s); });

      return boost::str(boost::format("id:%1% time:%2% samples:[%3%]")
          % sensor_id
          % ptime::to_simple_string(time)
          % boost::algorithm::join(samples_str, ","));
    }
  };

  /** The TCP socket to communicate with the sensor. */
  tcp::socket socket_;

  /** Buffer to receive data from the sensor. */
  asio::streambuf buffer_;

  /** Reads a message.
   *
   * TODO we need a timeout in case an incomplete message is sent
   *
   * @return The message.
   */
  message read_message() {
    message msg;
    msg.sensor_id = get_sensor_id();
    msg.time = get_time();
    auto sample_count = get_sample_count();
    for (unsigned i = 0; i < sample_count; i++)
      msg.samples.push_back(get_sample());
    return msg;
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

  /** Reads the time of measurement.
   *
   * @return The time of measurement.
   */
  ptime::ptime get_time() {
    auto line = read_line();
    std::smatch match;
    if (!std::regex_match(line, match, std::regex("time: (\\d+)"))
        || match.size() != 2)
      throw format_exception(
          boost::format("Error reading time: %1%") % line);
    auto time = boost::lexical_cast<std::time_t>(match[1].str());
    return ptime::from_time_t(time);
  }

  std::size_t get_sample_count() {
    auto line = read_line();
    std::smatch match;
    if (!std::regex_match(line, match, std::regex("sample-count: (\\d+)"))
        || match.size() != 2)
      throw format_exception(
          boost::format("Error reading sample count: %1%") % line);
    return boost::lexical_cast<std::size_t>(match[1].str());
  }

  unsigned get_sample() {
    auto line = read_line();
    std::smatch match;
    if (!std::regex_match(line, match, std::regex("sample: (\\d+)"))
        || match.size() != 2)
      throw format_exception(
          boost::format("Error reading sample: %1%") % line);
    return boost::lexical_cast<unsigned>(match[1].str());
  }

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

