#ifndef ZMODEM_FILE_H
#define ZMODEM_FILE_H

#include <string>
#include <fstream>

class ZmodemFile
{
public:
	ZmodemFile(
	const std::string& dir, 
	const std::string& filename, 
	const std::string& fileinfo);

	~ZmodemFile();

	bool write(const char* buf, unsigned len);
	unsigned getPos();
	bool isCompleted(){return pos_ == file_size_;}
	const std::string& getFilename(){return filename_;}
	const std::string& getPrompt(){return prompt_;}
	std::string getProgressLine();

	ZmodemFile(const std::string& filepath, const std::string& basename, unsigned long filesize);
	unsigned read(char*buf, unsigned size);
	void setPos(unsigned pos);

	bool isGood(){return file_.good();}

private:
	bool parseInfo(const std::string& fileinfo);

	std::fstream file_;
	std::string filename_;
	unsigned long file_size_;
	unsigned long file_time_;
	unsigned long pos_;
	std::string prompt_;

};

#endif /* ZMODEM_FILE_H */
