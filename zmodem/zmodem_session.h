#ifndef ZMODEM_SESSION_H
#define ZMODEM_SESSION_H

#include "Fsm.h"
#include "Action.h"
#include "State.h"
#include "Session.h"
#include <iostream>
#include <memory>
#include <base/synchronization/lock.h>
#include <base/file_path.h>
#include <string>
#include <fstream>
#include "zmodem.h"

class NativePuttyController;
struct frame_tag;
typedef struct frame_tag frame_t;
class ZmodemFile;

struct FileInfo{
	std::string filename;
	std::fstream fStream;

};

class ZmodemSession: public Fsm::Session
{
public:
	enum MyState
	{
		IDLE_STATE = 1,
		CHK_FRAME_TYPE_STATE,
		PARSE_HEX_STATE,
		PARSE_BIN_STATE,
		PARSE_BIN32_STATE,
		HANDLE_FRAME_STATE,
		WAIT_DATA_STATE,
		FILE_SELECTED_STATE,
		END_STATE
	};
	enum MyEvent
	{
		NETWORK_INPUT_EVT = 0,
		RESET_EVT,
		PARSE_HEX_EVT,
		PARSE_BIN_EVT,
		PARSE_BIN32_EVT,
		HANDLE_FRAME_EVT,
		WAIT_DATA_EVT,
		FILE_SELECTED_EVT,
		NEXT_EVT
	};
	typedef enum{
		DECODE_ERROR  = -1,
		DECODE_DONE   = 0,
		DECODE_PARTLY = 1
	} DecodeResult;
	static Fsm::FiniteStateMachine* getZmodemFsm();

	ZmodemSession(NativePuttyController* frontend);
	virtual ~ZmodemSession();
	void initState();
	int processNetworkInput(const char* const str, const int len, std::string& output);
	int onFileSelected(const FilePath& path);
	void reset();


	size_t dataCrcMatched(const size_t begin);
	unsigned short decodeCrc(const int index, int& consume_len);
	unsigned long decodeCrc32(const int index, int& consume_len);
	void sendFrameHeader(unsigned char type, long pos);
	void sendFrame(frame_t& frame);
	
	void sendBin32FrameHeader(unsigned char type, long pos);
	void sendBin32Frame(frame32_t& frame);
	unsigned convert2zline(char* dest, const unsigned dest_size, 
		const char* src, const unsigned src_len);

	void checkIfStartRz();
	void checkFrametype();
	void parseHexFrame();
	void parseBinFrame();
	void parseBin32Frame();
	void handleFrame();
	void sendZrinit();
	void handleZfile();
	void handleZdata();
	void sendZrpos();
	void sendFileInfo();

	const char* curBuffer(){return buffer_.c_str()+decodeIndex_;}
	void eatBuffer(){
		buffer_ = buffer_.substr(decodeIndex_);
		lastCheckExcaped_ = (lastCheckExcaped_ >= decodeIndex_) ? (lastCheckExcaped_ - decodeIndex_) : 0;
		lastCheckExcapedSaved_ = (lastCheckExcapedSaved_ >= decodeIndex_) ? (lastCheckExcapedSaved_ - decodeIndex_) : 0;
		decodeIndex_=0;
	}

	bool isDoingRz(){return getCurState().getId() !=  IDLE_STATE;};
	int lengthToBeDecode(){return buffer_.length() - decodeIndex_;};
	const char* bufferToBeDecode(){return buffer_.c_str() + decodeIndex_;}

	template<typename ReturnStruct>
	bool decodeEscapeStruct(const int index, int& consume_len, ReturnStruct& ret)
	{
		consume_len = 0;
		char crc_buffer[sizeof(ReturnStruct)] = {0};
		unsigned i, j;
		for (i = index, j = 0; j < sizeof(ReturnStruct) && i < buffer_.length(); i++, j++){
			if (buffer_[i] == ZDLE){
				if (i + 1 < buffer_.length()){
					crc_buffer[j] = buffer_[i+1] ^ 0x40;
					i++;
					consume_len += 2;
				}else{
					break;
				}
			}else{
				crc_buffer[j] = buffer_[i];
				consume_len ++;
			}
		}
		if (j < sizeof(ReturnStruct)){
			consume_len = 0;
			return false;
		}

		memcpy(&ret, crc_buffer, sizeof(ReturnStruct));
		return true;
	}
private:
	static base::Lock fsmLock_;
	static std::auto_ptr<Fsm::FiniteStateMachine> fsm_;

	frame_t* inputFrame_;
	std::string buffer_;
	unsigned decodeIndex_;
	unsigned lastCheckExcaped_;
	unsigned lastCheckExcapedSaved_;
	unsigned long dataCrc_;
	std::string output_;
	int recv_len_;
	ZmodemFile* zmodemFile_;
	NativePuttyController* frontend_;
	bool lastEscaped_;

	bool sendFinOnReset_;
	FilePath uploadFilePath_;
	unsigned char zsendline_tab[256];
};

#endif /* ZMODEM_SESSION_H */
