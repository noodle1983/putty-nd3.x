#include "zmodem_file.h"

ZmodemFile::ZmodemFile(const std::string filename, const std::string fileinfo)
	: filename_(filename),
	file_size_(0),
	file_time_(0)
{
	parseInfo(fileinfo);
}

bool ZmodemFile::write(const char* buf, unsigned len)
{

	return 0;
}
unsigned ZmodemFile::getPos()
{
	return 0;
}

bool ZmodemFile::parseInfo(const std::string& fileinfo)
{

}