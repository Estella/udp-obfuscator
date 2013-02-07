#include <iostream>
#include "udp_forward.h"
using namespace std;
using namespace boost;

bool parse_address(const char* s, string& addr, string& port) {
	if (not s) {
		return false;
	}
	string str(s);
	size_t i = str.rfind(':');
	if (i == string::npos) {
		return false;
	}
	addr = str.substr(0, i);
	/* ipv6 address */
	if (not addr.empty() and addr.front() == '[' and addr.back() == ']') {
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
	if (argc != 5 and argc != 7) {
		goto arg_error;
	}
	for (int i = 1; i < argc; i += 2) {
		if (argv[i] == string("-b")) {
			if (not parse_address(argv[i + 1], bind_addr, bind_port)) {
				goto arg_error;
			}
		} else if (argv[i] == string("-f")) {
			if (not parse_address(argv[i + 1], forward_addr, forward_port)) {
				goto arg_error;
			}
		} else if (argv[i] == string("-k")) {
			key = argv[i + 1];
		}
	}
	goto arg_correct;
	arg_error: ;
	cerr
			<< "usage: udp-obfuscator -b [BIND_ADDRESS]:BIND_PORT -f FORWARD_ADDRESS:FORWARD_PORT [-k KEY]"
			<< endl;
	return 1;
	arg_correct: ;
	using namespace asio;
	io_service io_service;
	ip::udp::resolver resolver(io_service);
	ip::udp::resolver::query bind_query(bind_addr, bind_port,
			ip::udp::resolver::query::passive);
	ip::udp::resolver::query forward_query(forward_addr, forward_port);
	udp_forward uf(io_service, *resolver.resolve(bind_query),
			*resolver.resolve(forward_query), key);
	io_service.run();
}
