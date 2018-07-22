#ifndef COLOR_HPP
#define COLOR_HPP

#include <string>
#include <vector>

namespace color {
	// escape sequences to get colored output (hardcoded by default)
	extern std::string reset,
		red,
		green,
		blue,
		brightwhite;

	// small wrapper to grab tput output
	// ex: cout << tput({"setaf", "9"}) << "red text" << tput({"sgr0"}) << endl;
	std::string tput(std::vector<std::string> args);

	// run tput to dynamically get current escape sequences for colors
	void setupColorCodes();
}

#endif // COLOR_HPP
