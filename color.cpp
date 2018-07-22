#include "color.hpp"
using std::string;
using std::vector;

#include <oil/subprocess.hpp>

namespace color {
	string red{"\x1b\x5b\x39\x31\x6d"},
		green{"\x1b\x5b\x39\x32\x6d"},
		blue{"\x1b\x5b\x39\x34\x6d"},
		brightwhite{"\x1b\x5b\x39\x37\x6d"},
		reset{"\x1b\x5b\x6d\x0f"};
}

string color::tput(vector<string> args) {
	Subprocess sp{"/usr/bin/tput", args};
	auto code = sp.run();
	auto &br = sp.br();
	string res{};
	while(!br.eof())
		res += br.read();
	return res;
}

void color::setupColorCodes() {
	red = tput({"setaf", "9"});
	green = tput({"setaf", "10"});
	blue = tput({"setaf", "12"});
	brightwhite = tput({"setaf", "15"});
	reset = tput({"sgr0"});
}

