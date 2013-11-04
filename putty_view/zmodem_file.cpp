#include "zmodem_file.h"
#include <stdio.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>


int mkdir(const char* dir, int attr);

void createDir(const std::string& thePath)
{
    if (thePath.empty())
        return;

    size_t pos = 0;
	while ((pos = thePath.find_first_of("/\\", pos)) != std::string::npos) 
    {
        std::string preDir = thePath.substr(0, pos);
        pos++;
        DIR* dir = opendir(preDir.c_str());
        if (dir != NULL)
        {
            closedir(dir);
            continue;
        }
        if (errno != ENOENT)
        {
            return;
        }

        mkdir(preDir.c_str(), 0774); 
    }
    DIR* dir = opendir(thePath.c_str());
    if (dir != NULL)
    {
        closedir(dir);
        return;
    }
    if (errno != ENOENT)
    {
        return;
    }
    mkdir(thePath.c_str(), 0774); 
}


ZmodemFile::ZmodemFile(
	const std::string& dir, 
	const std::string& filename, 
	const std::string& fileinfo)
	: filename_(filename),
	file_size_(0),
	file_time_(0),
	pos_(0)
{
	parseInfo(fileinfo);

	std::string full_path = dir + "/" + filename;
	unsigned found = full_path.find_last_of("/\\");
	createDir(full_path.substr(0,found));
	file_.open(full_path.c_str(), std::fstream::out|std::fstream::binary|std::fstream::trunc);
}

bool ZmodemFile::write(const char* buf, unsigned len)
{
	if (!file_.is_open() || len + pos_ > file_size_){
		return 0;
	}
	file_.write(buf, len);
	pos_ += len;
	return 1;
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