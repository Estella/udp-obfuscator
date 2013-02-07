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
				std::move(client_socket)), last_receive_time(
				last_receive_time) {
		}

udp_forward::udp_forward(asio::io_service& io_service,
		const asio::ip::udp::endpoint& local_endpoint,
		const asio::ip::udp::endpoint& remote_endpoint, const string& key) :
		io_service(io_service), local_endpoint(local_endpoint), remote_endpoint(
				remote_endpoint), server_socket(io_service, local_endpoint), buffer(
				buffer_capacity), expiration(posix_time::minutes(5)), clean_timer(
				io_service), key(key) {
	start_server_receive();
	start_clean_timer();
}

void udp_forward::start_clean_timer() {
	clean_timer.expires_from_now(posix_time::minutes(1));
	clean_timer.async_wait(
			bind(&udp_forward::handle_clean_timer, this,
					boost::asio::placeholders::error));
}

void udp_forward::start_server_receive() {
	server_socket.async_receive_from(asio::buffer(buffer), sender_endpoint,
			bind(&udp_forward::handle_server_receive, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void udp_forward::start_client_receive(std::shared_ptr<connection> pconn) {
	pconn->client_socket.async_receive(asio::buffer(buffer),
			bind(&udp_forward::handle_client_receive, this, pconn,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void udp_forward::handle_server_receive(const boost::system::error_code& ec,
		std::size_t bytes_transferred) {
	if (ec) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
		boost::system::error_code ec_close;
		server_socket.close(ec_close);
		connections.clear();
		return;
	}
	std::shared_ptr<connection> pconn;
	for (std::shared_ptr<connection>& pconn2 : connections) {
		if (sender_endpoint == pconn2->sender_endpoint) {
			pconn2->last_receive_time =
					posix_time::microsec_clock::universal_time();
			pconn = pconn2;
			break;
		}
	}
	if (not pconn) {
		pconn = std::shared_ptr<connection>(
				new connection(sender_endpoint,
						asio::ip::udp::socket(io_service),
						posix_time::microsec_clock::universal_time()));
		boost::system::error_code ec_connect;
		pconn->client_socket.connect(remote_endpoint, ec_connect);
		if (ec_connect) {
			cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message()
					<< endl;
			start_server_receive();
			return;
		}
		start_client_receive(pconn);
		connections.push_back(pconn);
	}
	obfuscate(bytes_transferred);
	boost::system::error_code ec_send;
	size_t bytes_sent = pconn->client_socket.send(
			asio::buffer(buffer.data(), bytes_transferred), 0, ec_send);
	if (ec_send) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
		boost::system::error_code ec_close;
		pconn->client_socket.close(ec_close);
		connections.remove(pconn);
	} else if (bytes_sent != bytes_transferred) {
		cerr << "sent size error." << endl;
		boost::system::error_code ec_close;
		pconn->client_socket.close(ec_close);
		connections.remove(pconn);
	}
	start_server_receive();
}

void udp_forward::handle_client_receive(std::shared_ptr<connection> pconn,
		const boost::system::error_code& ec, std::size_t bytes_transferred) {
	if (ec) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
		boost::system::error_code ec_close;
		pconn->client_socket.close(ec_close);
		connections.remove(pconn);
		return;
	}
	pconn->last_receive_time = posix_time::microsec_clock::universal_time();
	obfuscate(bytes_transferred);
	boost::system::error_code ec_send;
	server_socket.send_to(asio::buffer(buffer.data(), bytes_transferred),
			pconn->sender_endpoint, 0, ec_send);
	if (ec) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
		boost::system::error_code ec_close;
		pconn->client_socket.close(ec_close);
		connections.remove(pconn);
		return;
	}
	start_client_receive(pconn);
}

void udp_forward::handle_clean_timer(const boost::system::error_code& ec) {
	if (ec) {
		cerr << __FUNCTION__ << ":" << __LINE__ << ": " << ec.message() << endl;
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
	for (auto& ipconn : to_be_removed) {
		connections.erase(ipconn);
	}
	start_clean_timer();
}

void udp_forward::obfuscate(size_t buffer_size) {
	if (key.empty()) {
		return;
	}
	for (size_t i = 0, k = 0; i < buffer_size; i++, k = (k + 1) % key.size()) {
		buffer[i] ^= key[k];
	}
}
