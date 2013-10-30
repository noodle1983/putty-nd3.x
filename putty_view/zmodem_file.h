#ifndef ZMODEM_FILE_H
#define ZMODEM_FILE_H

#include <string>

class ZmodemFile
{
public:
	ZmodemFile(const std::string filename, const std::string fileinfo);

	bool write(const char* buf, unsigned len);
	unsigned getPos();

private:
	bool parseInfo(const std::string& fileinfo);

	std::string filename_;
	unsigned file_size_;
	unsigned file_time_;

};

#endif /* ZMODEM_FILE_H */
