/*
 * udp_forward.h
 *
 *  Created on: Feb 7, 2013
 *      Author: wcgbg
 */

#ifndef UDPFORWARD_H_
#define UDPFORWARD_H_

#include <iostream>
#include <list>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class udp_forward {
private:
	struct connection {
		connection(const boost::asio::ip::udp::endpoint& sender_endpoint,
				boost::asio::ip::udp::socket&& client_socket,
				const boost::posix_time::ptime& last_receive_time);
		boost::asio::ip::udp::endpoint sender_endpoint;
		boost::asio::ip::udp::socket client_socket;
		boost::posix_time::ptime last_receive_time;
	};
public:
	udp_forward(boost::asio::io_service& io_service,
			const boost::asio::ip::udp::endpoint& local_endpoint,
			const boost::asio::ip::udp::endpoint& remote_endpoint,const std::string& key);
private:
	void start_clean_timer();
	void start_server_receive();
	void start_client_receive(std::shared_ptr<connection> pconn);
	void handle_server_receive(const boost::system::error_code& ec,
			std::size_t bytes_transferred);
	void handle_client_receive(std::shared_ptr<connection> pconn,
			const boost::system::error_code& ec, std::size_t bytes_transferred);
	void handle_clean_timer(const boost::system::error_code& ec);
	void obfuscate(size_t buffer_size);
private:
	boost::asio::io_service& io_service;
	boost::asio::ip::udp::endpoint local_endpoint;
	boost::asio::ip::udp::endpoint remote_endpoint;
	boost::asio::ip::udp::socket server_socket;
	boost::asio::ip::udp::endpoint sender_endpoint;
	static const size_t buffer_capacity = 65536;
	std::vector<uint8_t> buffer;
	/* udp socket will be closed if no traffic in expiration time. */
	const boost::posix_time::time_duration expiration;
	std::list<std::shared_ptr<connection> > connections;
	boost::asio::deadline_timer clean_timer;
	std::string key;
};

#endif /* UDPFORWARD_H_ */
