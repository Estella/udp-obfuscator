#include <iostream>
#include "udp_forward.h"
using namespace std;
using namespace boost;

bool parse_address(const char* s, string& addr, string& port) {
	if (!s) {
		return false;
	}
	string str(s);
	size_t i = str.rfind(':');
	if (i == string::npos) {
		return false;
	}
	addr = str.substr(0, i);
	/* ipv6 address */
	if (!addr.empty() && addr.front() == '[' && addr.back() == ']') {
		addr = str.substr(1, i - 2);
	}
	port = str.substr(i + 1);
	return true;
}

int main(int argc, const char* argv[]) {
	string bind_addr;
	string bind_port;
	string forward_addr;
	string forward_port;
	string key;
	bool debug = false;
	int server_send_replay = 1;
	int client_send_replay = 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i] == string("-b")) {
			i++;
			if (i >= argc || !parse_address(argv[i], bind_addr, bind_port)) {
				goto arg_error;
			}
		} else if (argv[i] == string("-f")) {
			i++;
			if (i >= argc
					|| !parse_address(argv[i], forward_addr, forward_port)) {
				goto arg_error;
			}
		} else if (argv[i] == string("-k")) {
			i++;
			if (i >= argc) {
				goto arg_error;
			}
			key = argv[i];
		} else if (argv[i] == string("-c")) {
			key = char(255);
		} else if (argv[i] == string("-d")) {
			debug = true;
		} else if (argv[i] == string("-B")) {
			i++;
			if (i >= argc) {
				goto arg_error;
			}
			server_send_replay = atoi(argv[i]);
			if (server_send_replay < 1) {
				goto arg_error;
			}
		} else if (argv[i] == string("-F")) {
			i++;
			if (i >= argc) {
				goto arg_error;
			}
			client_send_replay = atoi(argv[i]);
			if (client_send_replay < 1) {
				goto arg_error;
			}
		} else {
			goto arg_error;
		}
	}
	if (bind_port.empty() || forward_addr.empty() || forward_port.empty()) {
		goto arg_error;
	}
	goto arg_correct;
	arg_error: ;
	cerr
			<< "usage: udp-obfuscator -b [BIND_ADDRESS]:BIND_PORT -f FORWARD_ADDRESS:FORWARD_PORT [-k KEY | -c] [-d] [-B BIND_END_SEND_REPLAY] [-F FORWARD_END_SEND_REPLAY]"
			<< endl;
	return 1;
	arg_correct: ;
	try {
		using namespace asio;
		io_service io_service;
		ip::udp::resolver resolver(io_service);
		ip::udp::resolver::query bind_query(bind_addr, bind_port,
				ip::udp::resolver::query::passive);
		ip::udp::resolver::query forward_query(forward_addr, forward_port);
		udp_forward uf(io_service, *resolver.resolve(bind_query),
				*resolver.resolve(forward_query), key, debug,
				server_send_replay, client_send_replay);
		io_service.run();
	} catch (boost::system::system_error &error) {
		cerr << error.what() << endl;
		return 1;
	}
}
