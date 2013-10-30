#ifndef ZMODEM_FILE_H
#define ZMODEM_FILE_H

#include <string>

class ZmodemFile
{
public:
	ZmodemFile(const std::string filename, const std::string fileinfo);

	bool write(const char* buf, unsigned len);
	unsigned getPos();
	bool isCompleted(){return pos_ == file_size_;}

private:
	bool parseInfo(const std::string& fileinfo);

	std::string filename_;
	unsigned long file_size_;
	unsigned long file_time_;
	unsigned long pos_;

};

#endif /* ZMODEM_FILE_H */
