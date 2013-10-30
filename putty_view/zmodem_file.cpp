#include "zmodem_file.h"
#include <stdio.h>
#include <assert.h>

ZmodemFile::ZmodemFile(const std::string filename, const std::string fileinfo)
	: filename_(filename),
	file_size_(0),
	file_time_(0),
	pos_(0)
{
	parseInfo(fileinfo);
}

bool ZmodemFile::write(const char* buf, unsigned len)
{
	assert(pos_ + len <= file_size_);
	pos_ += len;
	return 0;
}

unsigned ZmodemFile::getPos()
{
	return pos_;
}

bool ZmodemFile::parseInfo(const std::string& fileinfo)
{
	unsigned st_mode = 0;
	int file_left = 0;
	unsigned long  left_total = 0;
	file_size_ = 0;
	file_time_ = 0;
	sscanf(fileinfo.c_str(), "%lu %lo %o 0 %d %ld", &file_size_, &file_time_, &st_mode, &file_left, &left_total);
	return 1;
}