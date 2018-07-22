#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <oil/util.hpp>
#include <oil/subprocess.hpp>
#include <sys/statvfs.h>
#include <stdio.h>
#include <mntent.h>

void pline(std::ostream &out, std::string target, std::string src,
		std::string size, std::string used, std::string avail, std::string perc,
		std::string terminator = "\n") {
	out << std::setw(16) << std::left << target
		<< std::setw(16) << std::left << src
		<< std::setw(5) << std::right << size
		<< std::setw(6) << used
		<< std::setw(6) << avail
		<< std::setw(6) << perc
		<< terminator;
}

std::string red{"\x1b\x5b\x39\x31\x6d"},
	green{"\x1b\x5b\x39\x32\x6d"},
	blue{"\x1b\x5b\x39\x34\x6d"},
	brightwhite{"\x1b\x5b\x39\x37\x6d"},
	reset{"\x1b\x5b\x6d\x0f"};

std::string tput(std::vector<std::string> args) {
	Subprocess sp{"/usr/bin/tput", args};
	auto code = sp.run();
	auto &br = sp.br();
	std::string res{};
	while(!br.eof())
		res += br.read();
	return res;
}

void setupColorCodes() {
	red = tput({"setaf", "9"});
	green = tput({"setaf", "10"});
	blue = tput({"setaf", "12"});
	brightwhite = tput({"setaf", "15"});
	reset = tput({"sgr0"});
}

void usageBar(std::ostream &out, int perc, int width = 60) {
	static std::string bar(80, '=');
	auto hwidth = (width * perc + 99) / 100, rwidth = width - hwidth;
	out << "["
		<< ((perc > 90) ? red : blue) << bar.substr(0, hwidth) << reset
		<< brightwhite << bar.substr(0, rwidth) << reset
		<< "]";
}

std::string toHuman(long long bytes) {
	//static std::vector<std::string> suffixes{"B", "KiB", "MiB", "GiB", "TiB"};
	static std::vector<std::string> suffixes{"B", "K", "M", "G", "T"};
	int idx = 0;
	while(idx < suffixes.size() && bytes >= 1000) {
		idx++;
		//bytes /= 1024;
		bytes /= 1000;
	}
	return std::to_string(bytes) + suffixes[idx];
}

void subproc_method() {
	// could run with df piped in:
	// df -Hl -x fuse.ciopfs -x tmpfs -x devtmpfs
	// --output=target,source,pcent,size,used,avail --total
	//auto input = util::file::slurp("./stdin");
	//auto lines = util::split(input, "\n");

	Subprocess df{"/usr/bin/df", {"-Hl",
		"-x", "fuse.ciopfs", "-x", "tmpfs", "-x", "devtmpfs",
		"--output=target,source,pcent,size,used,avail",
		"--total"}};
	auto code = df.run();
	std::vector<std::string> lines{};
	auto &br = df.br();
	br.setBlocking(true);
	while(!br.eof()) {
		lines.push_back(br.read());
		//std::cout << "line: " << lines.back() << std::endl;
	}

	std::vector<std::vector<std::string>> fields{};
	for(size_t idx = 1; idx < lines.size(); ++idx) {
	//for(auto &line : lines) {
		auto &line = lines[idx];
		auto lfields = util::split(line, " ");
		if(lfields.size() == 0) continue;
		fields.push_back(lfields);
		//std::cout << "target: " << fields[0] << std::endl;
	}
	//std::cout << input << std::endl;
	std::sort(fields.begin(), fields.end());

	for(auto &lfields : fields) {
		//for(size_t i = 0; i < lfields.size(); ++i) std::cout << i << ": \"" << lfields[i] << "\"" << std::endl;
		pline(std::cout,
				lfields[0], lfields[1], lfields[3], lfields[4], lfields[5], lfields[2],
				"  ");
/*void pline(std::ostream &out, std::string target, std::string src,
		std::string size, std::string used, std::string avail, std::string perc,
		std::string terminator = "\n") {*/
		//std::cout << lfields[2] << " ";
		usageBar(std::cout, util::fromString<int>(lfields[2]));
		std::cout << std::endl;
	}
}

void syscall_method() {
	std::map<std::string, std::string> fs{};
	FILE *f = setmntent("/proc/self/mounts", "r");
	struct mntent mbuf;
	char mnbuf[1024 * 8 + 1];
	while(NULL != getmntent_r(f, &mbuf, mnbuf, 1024 * 8)) {
		if(mbuf.mnt_fsname[0] != '/') continue;
		//std::cout << "===========================" << std::endl;
		//std::cout << mbuf.mnt_fsname << ": " << mbuf.mnt_dir << std::endl;

		struct statvfs buf;
		int z = statvfs(mbuf.mnt_dir, &buf);
		/*
		std::cout << "block size: " << buf.f_bsize << std::endl
			<< "fragment size: " << buf.f_frsize << std::endl
			<< "size in fragments: " << buf.f_blocks << std::endl
			<< "free blocks: " << buf.f_bavail << std::endl
			<< "inodes: " << buf.f_files << std::endl
			<< "inodes avail: " << buf.f_favail << std::endl
			<< std::endl;*/

		auto size = buf.f_blocks * buf.f_frsize;
		auto free = buf.f_bavail * buf.f_bsize;
		auto used = size - free;

		pline(std::cout,
				mbuf.mnt_dir, mbuf.mnt_fsname,
				toHuman(size), toHuman(used), toHuman(free),
				std::to_string(100 * used / size) + "%",
				"  ");
		usageBar(std::cout, 100 * used / size);
		std::cout << std::endl;
	}
	endmntent(f);
}

int main(int, char **) {
	// defaults to hardcoded values
	//setupColorCodes();
	//std::cout << red << "RED" << blue << "BLUE" << reset << std::endl;

	pline(std::cout, "Mount Point", "Device", "Size", "Used", "Free", "Use%");

	//subproc_method();
	syscall_method();
	return 0;
}

