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

#include "color.hpp"

// get a human readable size from a byte count
std::string toHuman(long long bytes, bool si = false);

// print a formatted line to out (handles field alignment)
void pline(std::ostream &out, std::string target, std::string src,
		std::string size, std::string used, std::string avail, std::string perc,
		std::string terminator = "\n");

static constexpr int MAX_USAGE_BAR_WIDTH = 80;
// print a colored usage bar to out
void usageBar(std::ostream &out, int perc, int width = 60);


std::string toHuman(long long bytes, bool si) {
	static std::vector<std::string> suffixesSI{"B", "KiB", "MiB", "GiB", "TiB"};
	static std::vector<std::string> suffixes10{"B", "K", "M", "G", "T"};
	auto &suffixes = (si ? suffixesSI : suffixes10);
	auto div = (si ? 1024 : 1000);

	int idx = 0;
	while(idx < suffixes.size() && bytes >= div) {
		idx++;
		bytes /= div;
	}
	return std::to_string(bytes) + suffixes[idx];
}

void pline(std::ostream &out, std::string target, std::string src,
		std::string size, std::string used, std::string avail, std::string perc,
		std::string terminator) {
	out << std::setw(16) << std::left << target
		<< std::setw(16) << std::left << src
		<< std::setw(5) << std::right << size
		<< std::setw(6) << used
		<< std::setw(6) << avail
		<< std::setw(6) << perc
		<< terminator;
}

void usageBar(std::ostream &out, int perc, int width) {
	static std::string bar(MAX_USAGE_BAR_WIDTH, '=');
	auto hwidth = (width * perc + 99) / 100, rwidth = width - hwidth;
	out << "["
		<< ((perc > 90) ? color::red : color::blue)
		<< bar.substr(0, hwidth) << color::reset
		<< color::brightwhite << bar.substr(0, rwidth) << color::reset
		<< "]";
}

std::vector<std::string> df() {
	Subprocess df{"/usr/bin/df", {"-Hl",
		"-x", "fuse.ciopfs", "-x", "tmpfs", "-x", "devtmpfs",
		"--output=target,source,pcent,size,used,avail"}};
	auto code = df.run();
	std::vector<std::string> lines{};
	auto &br = df.br();
	br.setBlocking(true);
	while(!br.eof()) {
		lines.push_back(br.read());
	}
	return lines;
}

void subproc_method() {
	// could run with df piped in:
	// df -Hl -x fuse.ciopfs -x tmpfs -x devtmpfs
	// --output=target,source,pcent,size,used,avail --total
	//auto input = util::file::slurp("./stdin");
	//auto lines = util::split(input, "\n");
	auto lines = df();

	std::vector<std::vector<std::string>> fields{};
	for(size_t idx = 1; idx < lines.size(); ++idx) {
		auto &line = lines[idx];
		auto lfields = util::split(line, " ");
		if(lfields.size() == 0) continue;
		fields.push_back(lfields);
	}
	std::sort(fields.begin(), fields.end());

	for(auto &lfields : fields) {
		pline(std::cout,
				lfields[0], lfields[1], lfields[3], lfields[4], lfields[5], lfields[2],
				"  ");
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
		// ignore non-/dev/ mount sources
		if(mbuf.mnt_fsname[0] != '/') continue;

		struct statvfs buf;
		int z = statvfs(mbuf.mnt_dir, &buf);

		// inode total: buf.f_files, available: f._favail
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

	pline(std::cout, "Mount Point", "Device", "Size", "Used", "Free", "Use%");

	//subproc_method();
	syscall_method();

	return 0;
}

