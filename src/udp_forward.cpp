/*
 * udp_forward.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: wcgbg
 */

#include "udp_forward.h"

using namespace std;
using namespace boost;

udp_forward::connection::connection(
		const asio::ip::udp::endpoint& sender_endpoint,
		asio::ip::udp::socket&& client_socket,
		const posix_time::ptime& last_receive_time) :
		sender_endpoint(sender_endpoint), client_socket(
				std::move(client_socket)), last_receive_time(last_receive_time), receive_buffer(
				udp_forward::buffer_capacity) {
}

udp_forward::udp_forward(asio::io_service& io_service,
		const asio::ip::udp::endpoint& local_endpoint,
		const asio::ip::udp::endpoint& remote_endpoint, const string& key,
		bool debug) :
		io_service(io_service), local_endpoint(local_endpoint), remote_endpoint(
				remote_endpoint), server_socket(io_service, local_endpoint), server_receive_buffer(
				buffer_capacity), expiration(posix_time::minutes(5)), clean_timer(
				io_service), key(key), debug(debug) {
	start_server_receive();
	start_clean_timer();
}

void udp_forward::start_clean_timer() {
	clean_timer.expires_from_now(posix_time::minutes(1));
	clean_timer.async_wait(
			boost::bind(&udp_forward::handle_clean_timer, this,
					boost::asio::placeholders::error));
}

void udp_forward::start_server_receive() {
	server_socket.async_receive_from(asio::buffer(server_receive_buffer),
			server_receive_sender_endpoint,
			boost::bind(&udp_forward::handle_server_receive, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void udp_forward::start_client_receive(std::shared_ptr<connection> pconn) {
	pconn->client_socket.async_receive(asio::buffer(pconn->receive_buffer),
			boost::bind(&udp_forward::handle_client_receive, this, pconn,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void udp_forward::handle_server_receive(
		const boost::system::error_code& ec_receive,
		std::size_t bytes_transferred) {
	try {
		boost::asio::detail::throw_error(ec_receive, "receive");
		if (debug) {
			cout << "server receives ";
			print_packet(server_receive_buffer, bytes_transferred);
		}
		std::shared_ptr<connection> pconn;
		for (auto ipconn2 = connections.begin(); ipconn2 != connections.end();
				++ipconn2) {
			if (server_receive_sender_endpoint == (*ipconn2)->sender_endpoint) {
				(*ipconn2)->last_receive_time =
						posix_time::microsec_clock::universal_time();
				pconn = *ipconn2;
				break;
			}
		}
		boost::system::error_code ec;
		if (!pconn) {
			pconn = std::shared_ptr<connection>(
					new connection(server_receive_sender_endpoint,
							asio::ip::udp::socket(io_service),
							posix_time::microsec_clock::universal_time()));
			pconn->client_socket.connect(remote_endpoint);
			start_client_receive(pconn);
			connections.push_back(pconn);
		}
		obfuscate(server_receive_buffer, bytes_transferred);
		size_t bytes_sent = pconn->client_socket.send(
				asio::buffer(server_receive_buffer.data(), bytes_transferred), 0, ec);
		if (ec) {
			cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message()
					<< endl;
			pconn->client_socket.close(ec);
			connections.remove(pconn);
		} else if (bytes_sent != bytes_transferred) {
			cerr << __FUNCTION__ << ":" << __LINE__ << ": "
					<< "Sent size error." << endl;
			pconn->client_socket.close(ec);
			connections.remove(pconn);
		}
	} catch (boost::system::system_error &error) {
		cerr << __FUNCTION__ << ":" << error.what() << endl;
	}
	start_server_receive();
}

void udp_forward::handle_client_receive(std::shared_ptr<connection> pconn,
		const boost::system::error_code& ec_receive,
		std::size_t bytes_transferred) {
	if (ec_receive) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec_receive.message()
				<< endl;
		boost::system::error_code ec;
		pconn->client_socket.close(ec);
		connections.remove(pconn);
		return;
	}
	if (debug) {
		cout << "client receives ";
		print_packet(pconn->receive_buffer, bytes_transferred);
	}
	pconn->last_receive_time = posix_time::microsec_clock::universal_time();
	obfuscate(pconn->receive_buffer, bytes_transferred);
	boost::system::error_code ec;
	server_socket.send_to(asio::buffer(pconn->receive_buffer.data(), bytes_transferred),
			pconn->sender_endpoint, 0, ec);
	if (ec) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
		boost::system::error_code ec;
		pconn->client_socket.close(ec);
		connections.remove(pconn);
		return;
	}
	start_client_receive(pconn);
}

void udp_forward::handle_clean_timer(
		const boost::system::error_code& ec_timer) {
	if (ec_timer) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec_timer.message()
				<< endl;
		return;
	}
	posix_time::ptime now(posix_time::microsec_clock::universal_time());
	vector<list<std::shared_ptr<connection> >::iterator> to_be_removed;
	for (auto ipconn = connections.begin(); ipconn != connections.end();
			++ipconn) {
		assert((*ipconn)->client_socket.is_open());
		if (now > (*ipconn)->last_receive_time + expiration) {
			to_be_removed.push_back(ipconn);
		}
	}
	for (auto iipconn = to_be_removed.begin(); iipconn != to_be_removed.end();
			++iipconn) {
		connections.erase(*iipconn);
	}
	start_clean_timer();
}

void udp_forward::obfuscate(std::vector<uint8_t>& buffer,
		size_t buffer_size) const {
	if (key.empty()) {
		return;
	}
	for (size_t i = 0, k = 0; i < buffer_size; i++, k = (k + 1) % key.size()) {
		buffer[i] ^= key[k];
	}
}

void udp_forward::print_packet(const std::vector<uint8_t>& buffer,
		size_t buffer_size) {
	const char* const t = "0123456789abcdef";
	cout << buffer_size << '\t';
	for (size_t i = 0; i < min(buffer_size, size_t(16)); i++) {
		cout << t[buffer[i] / 16] << t[buffer[i] % 16] << ' ';
	}
	cout << endl;
}
