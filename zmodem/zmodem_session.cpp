#include "zmodem_session.h"
#include "putty.h"
#include "crctab.c"
#include "native_putty_controller.h"
#include "zmodem_file.h"
#include "PuttyFileDialog.h"
#include "base/file_util.h"

#include "atlconv.h" 
#include "zmodem.h"
base::Lock ZmodemSession::fsmLock_;
std::auto_ptr<Fsm::FiniteStateMachine> ZmodemSession::fsm_;

const char HEX_PREFIX[] = {ZPAD, ZPAD, ZDLE, ZHEX};
const char BIN32_PREFIX[] = {ZPAD, ZPAD, ZDLE, ZBIN32};
const char HEX_ARRAY[] = "0123456789abcdef";


inline int hex2int(char hex)
{
	if (hex >= '0' && hex <='9')
		return hex - '0';
	if (hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	return 0;
}

unsigned char decHex(const hex_str_t *h)
{
    return (hex2int(h->hex[0]) << 4) | hex2int(h->hex[1]);
}

void encHex(const unsigned char c, hex_str_t *h)
{
    h->hex[0] = HEX_ARRAY[(c&0xF0) >> 4];
    h->hex[1] = HEX_ARRAY[c&0x0F];
}

void convHex2Plain(const hex_t *hexframe, frame_t* frame)
{
    frame->type = decHex(&(hexframe->type));
    frame->flag[0] = decHex(&(hexframe->flag[0]));
    frame->flag[1] = decHex(&(hexframe->flag[1]));
    frame->flag[2] = decHex(&(hexframe->flag[2]));
    frame->flag[3] = decHex(&(hexframe->flag[3]));
    frame->crc = decHex(&(hexframe->crc[0]))<<8 | decHex(&(hexframe->crc[1]));
}

void convPlain2Hex(const frame_t* frame, hex_t *hexframe)
{
    encHex(frame->type, &(hexframe->type));
    encHex(frame->flag[0], &(hexframe->flag[0]));
    encHex(frame->flag[1], &(hexframe->flag[1]));
    encHex(frame->flag[2], &(hexframe->flag[2]));
    encHex(frame->flag[3], &(hexframe->flag[3]));
    encHex(frame->crc >> 8, &(hexframe->crc [0]));
    encHex(frame->crc & 0x00FF, &(hexframe->crc [1]));
}

unsigned short calcFrameCrc(const frame_t *frame)
{
    int i = 0;
    unsigned short crc;
    crc = updcrc((frame->type & 0x7f), 0);
    for (i = 0; i < 4; i++){
        crc = updcrc(frame->flag[i], crc);
    }
    crc = updcrc(0,updcrc(0,crc));
    return crc;
}

unsigned long calcFrameCrc32(const frame32_t *frame)
{
    int i = 0;
    unsigned long crc = 0xFFFFFFFFL;
    crc = UPDC32((frame->type & 0x7f), crc);
    for (i = 0; i < 4; i++){
        crc = UPDC32(frame->flag[i], crc);
    }
    crc = ~crc;;
    return crc;
}

unsigned long calcBufferCrc32(const char *buf, const unsigned len)
{
    int i = 0;
    unsigned long crc = 0xFFFFFFFFL;
    for (i = 0; i < len; i++){
        crc = UPDC32(buf[i], crc);
    }
    crc = ~crc;;
    return crc;
}

unsigned getPos(frame_t* frame)
{
	unsigned rxpos = frame->flag[ZP3] & 0377;
	rxpos = (rxpos<<8) + (frame->flag[ZP2] & 0377);
	rxpos = (rxpos<<8) + (frame->flag[ZP1] & 0377);
	rxpos = (rxpos<<8) + (frame->flag[ZP0] & 0377);
	return rxpos;
}

Fsm::FiniteStateMachine* ZmodemSession::getZmodemFsm()
{
	if (NULL == fsm_.get())
    {
        base::AutoLock lock(fsmLock_);
        if (NULL == fsm_.get())
        {
            Fsm::FiniteStateMachine* fsm = new Fsm::FiniteStateMachine;
			(*fsm) += FSM_STATE(IDLE_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::initState);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(CHK_FRAME_TYPE_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));

			(*fsm) += FSM_STATE(CHK_FRAME_TYPE_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::checkFrametype);
			(*fsm) +=      FSM_EVENT(PARSE_HEX_EVT,   CHANGE_STATE(PARSE_HEX_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN_EVT,   CHANGE_STATE(PARSE_BIN_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN32_EVT, CHANGE_STATE(PARSE_BIN32_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));

			(*fsm) += FSM_STATE(PARSE_HEX_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseHexFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseHexFrame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(PARSE_BIN_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseBinFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseBinFrame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(PARSE_BIN32_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseBin32Frame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseBin32Frame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(HANDLE_FRAME_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::handleFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(CHK_FRAME_TYPE_STATE));
			(*fsm) +=      FSM_EVENT(FILE_SELECTED_EVT,  CHANGE_STATE(FILE_SELECTED_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(WAIT_DATA_EVT,  CHANGE_STATE(WAIT_DATA_STATE));
			(*fsm) +=      FSM_EVENT(SEND_ZDATA_EVT,  CHANGE_STATE(SEND_ZDATA_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(SEND_ZDATA_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(100));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   &ZmodemSession::sendZdata);
			(*fsm) +=      FSM_EVENT(SEND_ZDATA_EVT,   CHANGE_STATE(SEND_ZDATA_STATE));
			(*fsm) +=      FSM_EVENT(SEND_ZDATA_LATER_EVT,   CHANGE_STATE(SEND_FLOW_CTRL_STATE));
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,CHANGE_STATE(CHK_FRAME_TYPE_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, &ZmodemSession::sendZrpos);
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(SEND_FLOW_CTRL_STATE);
			(*fsm) += 	   FSM_EVENT(Fsm::ENTRY_EVT,	 NEW_TIMER(50));
			(*fsm) +=	   FSM_EVENT(RESET_EVT		  ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=	   FSM_EVENT(DESTROY_EVT	  ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=	   FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(SEND_ZDATA_STATE));
			(*fsm) +=	   FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(WAIT_DATA_STATE);
			//(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(1000));
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(FILE_SELECTED_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::sendFileInfo);
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(DESTROY_EVT      ,  CHANGE_STATE(DESTROY_STATE));
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(CHK_FRAME_TYPE_STATE));

			(*fsm) += FSM_STATE(DESTROY_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::deleteSelf);

			(*fsm) += FSM_STATE(END_STATE);	
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  CHANGE_STATE(IDLE_STATE));

			fsm_.reset(fsm);
        }

    }
    return fsm_.get();
}

//-----------------------------------------------------------------------------

ZmodemSession::ZmodemSession(NativePuttyController* frontend)
	: Fsm::Session(getZmodemFsm(), 0)
	, frontend_(frontend)
	, zmodemFile_(NULL)
	, lastEscaped_(false)
	, bufferParsed_(false)
	, isDestroyed_(false)
{
	inputFrame_ = new frame_t;
	sendFinOnReset_ = false;
	isSz_ = true;
	file_select_state_ = FILE_SELECT_NONE;
	tick_ = 0;

	int i;
	for (i=0;i<256;i++) {	
		if (i & 0140){
			zsendline_tab[i]=0;
		}else {
			switch(i)
			{
			case ZDLE:
			case XOFF: /* ^Q */
			case XON: /* ^S */
			case (XOFF | 0200):
			case (XON | 0200):
				zsendline_tab[i]=1;
				break;
			case 020: /* ^P */
			case 0220:
				zsendline_tab[i]=1;
				break;
			case 015:
			case 0215:
				zsendline_tab[i]=1;
				break;
			default:
				zsendline_tab[i]=1;
			}
		}
	}
}

//-----------------------------------------------------------------------------

ZmodemSession::~ZmodemSession()
{
	delete inputFrame_;
}

//-----------------------------------------------------------------------------

void ZmodemSession::initState()
{
	if (zmodemFile_ || sendFinOnReset_){
		delete zmodemFile_;
		zmodemFile_ = NULL;
		if (!isSz_ && (inputFrame_->type == ZFIN)){
			frontend_->send("OO", 2);
		}else{
			frame_t frame;
			memset(&frame, 0, sizeof(frame_t));
			frame.type = ZFIN;
			sendFrame(frame);
		}
		if (!isToDelete()) asynHandleEvent(RESET_EVT);
	}
	buffer_.clear();
	buffer_.reserve(1024 * 16);
	decodeIndex_ = 0;
	lastCheckExcaped_ = 0;
	lastCheckExcapedSaved_ = 0;
	dataCrc_ = 0xFFFFFFFFL;
	recv_len_ = 0;
	lastEscaped_ = false;
	sendFinOnReset_ = false;
	uploadFilePath_.clear();
	file_select_state_ = FILE_SELECT_NONE;
	tick_ = 0;
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::checkFrametype()
{
	for (; decodeIndex_ < buffer_.length() 
		&& ZPAD != buffer_[decodeIndex_] ; decodeIndex_ ++);
	for (; decodeIndex_ < buffer_.length() 
		&& ZPAD == buffer_[decodeIndex_] ; decodeIndex_ ++);

	if (decodeIndex_ + 2 >= buffer_.length()){
		handleEvent(RESET_EVT);
		return;
	}

	if (ZDLE != buffer_[decodeIndex_++]){
		handleEvent(RESET_EVT);
		return;
	}

	int frametype = buffer_[decodeIndex_++];
	if (ZHEX == frametype){
			eatBuffer();
            handleEvent(PARSE_HEX_EVT);
			return;
	}else if (ZBIN == frametype){
			eatBuffer();
			handleEvent(PARSE_BIN_EVT);
			return;
	}else if (ZBIN32 == frametype){
			eatBuffer();
			handleEvent(PARSE_BIN32_EVT);
			return;
	}else{
		//output("\r\nonly support(HEX,BIN,BIN32) frame\r\n");
		handleEvent(RESET_EVT);
		return;
	}
}

//-----------------------------------------------------------------------------

void ZmodemSession::parseHexFrame()
{
	if (decodeIndex_ + sizeof(hex_t) >= buffer_.length())
		return;
	hex_t  hexframe;
	memcpy(&hexframe, curBuffer(), sizeof(hex_t));
	decodeIndex_ += sizeof(hex_t);

	frame_t frame;
    convHex2Plain(&hexframe, &frame);
    if (frame.crc != calcFrameCrc(&frame)){
		output("crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }

	int old_index = decodeIndex_;
	for (; decodeIndex_ < buffer_.length() 
		&& ('\r' == buffer_[decodeIndex_] 
			|| '\n' == buffer_[decodeIndex_]
			|| -118 == buffer_[decodeIndex_]) ; decodeIndex_ ++);
	if (old_index == decodeIndex_){
		output("no line seed found!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	if (frame.type != ZACK && frame.type != ZFIN && buffer_[decodeIndex_++] != XON){
		output("XON expected!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer();
	memcpy(inputFrame_, &frame, sizeof(frame_t));
	handleEvent(HANDLE_FRAME_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::parseBinFrame()
{
	if (decodeIndex_ + sizeof(frame_t) >= buffer_.length())
		return;
	frame_t frame;
	int frame_len = 0;
    if (!decodeEscapeStruct<frame_t>(decodeIndex_, frame_len, frame)){
		return;
	}
	decodeIndex_ += frame_len;

    if (frame.crc != calcFrameCrc(&frame)){
		output("bin crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer();
	memcpy(inputFrame_, &frame, sizeof(frame_t));
	handleEvent(HANDLE_FRAME_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::parseBin32Frame()
{
	if (decodeIndex_ + sizeof(frame32_t) > buffer_.length())
		return;
	frame32_t frame;
	int frame_len = 0;
	if (!decodeEscapeStruct<frame32_t>(decodeIndex_, frame_len, frame)){
		return;
	}
	decodeIndex_ += frame_len;

    if (frame.crc != calcFrameCrc32(&frame)){
		output("bin32 crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer();
	memcpy(inputFrame_, &frame, sizeof(frame_t));
	handleEvent(HANDLE_FRAME_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleFrame()
{
	bufferParsed_ = true;
	switch (inputFrame_->type){
    case ZRQINIT:
        return sendZrinit();

    case ZFILE: 
		isSz_ = true;
		return handleZfile();
    case ZDATA:
		return handleZdata();
    case ZEOF:
		if (zmodemFile_){
			delete zmodemFile_;
			zmodemFile_ = NULL;
		}
		return sendZrinit();
    case ZFIN:
		sendFinOnReset_ = true;
		handleEvent(RESET_EVT);
		return;
    case ZRINIT:
		isSz_ = false;
		if (file_select_state_ == FILE_SELECT_NONE){
			file_select_state_ = FILE_SELECTING;
			PuttyFileDialogSingleton::instance()->showOpenDialog(
				frontend_->getNativeParentWindow(), this);
			sendFinOnReset_ = true;
			//no timer for user to select file
			cancelTimer();
		}else if (file_select_state_ == FILE_SELECTED && zmodemFile_ && !zmodemFile_->isGood()){
			//complete or send other files;
			sendBin32FrameHeader(ZCOMPL, 0);
			handleEvent(RESET_EVT);
			file_select_state_ = FILE_SELECT_NONE;;
		}
		return;
    case ZRPOS:
		if (!zmodemFile_){
			sendFinOnReset_ = true;
			sendBin32FrameHeader(ZCAN, 0);
			sendBin32FrameHeader(ZABORT, 0);
			handleEvent(RESET_EVT);
			return;
		}
		zmodemFile_->setPos(getPos(inputFrame_));
		sendBin32FrameHeader(ZDATA, zmodemFile_->getPos());
		sendZdata();
		return;
    case ZNAK:
		if (decodeIndex_ < buffer_.length()){
			handleEvent(NETWORK_INPUT_EVT);
		}
		return;
    case ZSINIT:
    case ZACK:
    case ZSKIP:
    case ZABORT:
    case ZFERR:
    case ZCRC:
    case ZCHALLENGE:
    case ZCOMPL:
    case ZCAN:
    case ZFREECNT:
    case ZCOMMAND:
    case ZSTDERR: 

    default:
        output("invalid frame type!\r\n");
		bufferParsed_ = false;
        handleEvent(RESET_EVT);
        return ;


    }
    return ;


}

//-----------------------------------------------------------------------------

void ZmodemSession::sendZdata()
{
	const unsigned BUFFER_LEN = 1024;
	char buffer[BUFFER_LEN + 16] = {0};

	if (frontend_->send_buffer_size() > 1024*1024){
		if (!isToDelete()) asynHandleEvent(SEND_ZDATA_LATER_EVT);
		return;
	}

	unsigned len = zmodemFile_->read(buffer, BUFFER_LEN);
	char frameend = zmodemFile_->isGood() ? ZCRCG : ZCRCE;
	send_zsda32(buffer, len, frameend);
	std::string report_line(zmodemFile_->getProgressLine());
	flow_control_fresh_lastline(frontend_->term, zmodemFile_->getPrompt().length(), 
		report_line.c_str(), report_line.length());
		
	if(!zmodemFile_->isGood()){
		sendBin32FrameHeader(ZEOF, zmodemFile_->getPos());
		term_fresh_lastline(frontend_->term, zmodemFile_->getPrompt().length(), 
				report_line.c_str(), report_line.length());
		term_data(frontend_->term, 0, "\r\n", 2);
		return;
	}else{
		if (!isToDelete()) asynHandleEvent(SEND_ZDATA_EVT);
		return;
	}
}

//-----------------------------------------------------------------------------

void ZmodemSession::sendFrame(frame_t& frame)
{
    frame.crc = calcFrameCrc(&frame);
    hex_t hexframe;
    convPlain2Hex(&frame, &hexframe);

    char buf[32] = {0};
    int len = 0;
    memcpy(buf+len, HEX_PREFIX, 4);
    len += 4;
    memcpy(buf+len, &hexframe, sizeof (hex_t));
    len += sizeof (hex_t);
	buf[len++] = '\r';
	buf[len++] = 0212;
	if (frame.type != ZFIN && frame.type != ZACK){
		buf[len++] = XON;
	}
    frontend_->send(buf, len);
}
//-----------------------------------------------------------------------------

void ZmodemSession::sendFrameHeader(unsigned char type, long pos)
{
	frame_t frame;
    memset(&frame, 0, sizeof(frame_t));
    frame.type = type;
	frame.flag[ZP0] = (unsigned char)(pos);
	frame.flag[ZP1] = (unsigned char)(pos>>8);
	frame.flag[ZP2] = (unsigned char)(pos>>16);
	frame.flag[ZP3] = (unsigned char)(pos>>24);
    sendFrame(frame);
}

//-----------------------------------------------------------------------------

void ZmodemSession::sendBin32FrameHeader(unsigned char type, long pos)
{
	frame32_t frame;
    memset(&frame, 0, sizeof(frame_t));
    frame.type = type;
	frame.flag[ZP0] = pos;
	frame.flag[ZP1] = pos>>8;
	frame.flag[ZP2] = pos>>16;
	frame.flag[ZP3] = pos>>24;
    sendBin32Frame(frame);
}

//-----------------------------------------------------------------------------

void ZmodemSession::sendBin32Frame(frame32_t& frame)
{
    frame.crc = calcFrameCrc32(&frame);

    char buf[256] = {0};
    int len = 0;
    memcpy(buf+len, BIN32_PREFIX, 4);
    len += 4;

    len += convert2zline(buf+len, sizeof(buf) -len, (char*)&frame, sizeof(frame));
    frontend_->send(buf, len);
}
//-----------------------------------------------------------------------------

unsigned ZmodemSession::convert2zline(char* dest, const unsigned dest_size, 
		const char* src, const unsigned src_len)
{
	char lastsent = 0;
	int ret_len = 0;
	for (int i = 0; i < src_len && ret_len < dest_size; i++){
		char c = src[i];
		unsigned char escape_value = (zsendline_tab[(unsigned char) (c&=0377)]);
		if (0 ==  escape_value){
			dest[ret_len++] = (lastsent = c); 
		}else if (1 ==  escape_value){
			dest[ret_len++] = ZDLE;
			c ^= 0100;
			dest[ret_len++] = (lastsent = c);
		}else if (2 ==  escape_value){
			if ((lastsent & 0177) != '@') {
				dest[ret_len++] = (lastsent = c);
			} else {
				dest[ret_len++] = (ZDLE);
				c ^= 0100;
				dest[ret_len++] = (lastsent = c);
			}
		}

	}
	return ret_len;

}

//-----------------------------------------------------------------------------

void ZmodemSession::sendZrinit()
{
	frame_t frame;
    memset(&frame, 0, sizeof(frame_t));
    frame.type = ZRINIT;
    frame.flag[ZF0] = CANFC32|CANFDX|CANOVIO;
	sendFrame(frame);
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleZfile()
{
	unsigned oldIndex = decodeIndex_;
	std::string filename(curBuffer());
	decodeIndex_ += filename.length() + 1;
	std::string fileinfo(curBuffer());
	decodeIndex_ += fileinfo.length() + 1;

	if (decodeIndex_ + 6 > buffer_.length()){
		decodeIndex_ = oldIndex;
        handleEvent(WAIT_DATA_EVT);
        return ;
	}
	int crc_len = 0;
	unsigned long recv_crc = decodeCrc32(decodeIndex_ + 2, crc_len);
	if (crc_len == 0){
		decodeIndex_ = oldIndex;
		handleEvent(WAIT_DATA_EVT);
        return ;
	}
	buffer_[decodeIndex_] = buffer_[decodeIndex_+1];
	decodeIndex_++;
	unsigned long crc = calcBufferCrc32(buffer_.c_str() + oldIndex, decodeIndex_ - oldIndex);

	decodeIndex_++;
	decodeIndex_ += crc_len;
	if (*curBuffer() == XON){
		decodeIndex_++;
	}

	if (recv_crc != crc){
		output("zfile frame crc invalid!\r\n");
		bufferParsed_ = false;
		sendFinOnReset_ = true;
        handleEvent(RESET_EVT);
        return ;
	}
	eatBuffer();
	recv_len_ = 0;

	if (zmodemFile_)
		delete zmodemFile_;
	zmodemFile_ = new ZmodemFile(conf_get_str( frontend_->cfg, CONF_default_log_path), filename, fileinfo);
	term_fresh_lastline(frontend_->term, 0, 
		zmodemFile_->getPrompt().c_str(), zmodemFile_->getPrompt().length());

	sendFrameHeader(ZRPOS, zmodemFile_->getPos());
}

//-----------------------------------------------------------------------------

void ZmodemSession::send_zsda32(char *buf, size_t length, char frameend)
{
	char send_buf[2048+128];
	size_t send_len = 0;
	int c;
	unsigned long crc;
	int i;

	send_len = convert2zline(send_buf, sizeof(send_buf), buf, length);
	send_buf[send_len++] = ZDLE;
	send_buf[send_len++] = frameend;

	//crc includes the frameend
	buf[length] = frameend;
	crc = calcBufferCrc32(buf, length+1);
	send_len +=convert2zline(send_buf+send_len, sizeof(send_buf) - send_len, (char*)&crc, sizeof(crc)); 
	if (frameend == ZCRCW) {
		send_buf[send_len++] = (XON);  
	}
	frontend_->send(send_buf, send_len);
}

//-----------------------------------------------------------------------------

void ZmodemSession::sendFileInfo()
{
	USES_CONVERSION;
	base::PlatformFileInfo info;
	bool res = GetFileInfo(uploadFilePath_, &info);
	std::string basename(W2A(uploadFilePath_.BaseName().value().c_str()));
	if (res == false){
		std::string out(std::string("can't get info of file:") + basename + "\r\n");
		output(out.c_str());
		bufferParsed_ = false;
		handleEvent(RESET_EVT);
		return;
	}
	char filedata[1024] = {0};
	unsigned filedata_len = 0;
	memcpy(filedata + filedata_len, basename.c_str(), basename.length() +1);
	filedata_len += basename.length() +1;
	snprintf(filedata + filedata_len, sizeof(filedata_len) - filedata_len, "%lu %lo 100644 0 1 %lu", 
		(long)info.size, (long)(info.last_modified.ToInternalValue()/1000000), (long)info.size);
	filedata_len += strlen(filedata + filedata_len);
	filedata[filedata_len++] = 0;

	frame32_t frame;
	frame.type = ZFILE;
	frame.flag[ZF0] = ZCBIN;	/* file conversion request */
	frame.flag[ZF1] = ZF1_ZMCLOB;	/* file management request */
	frame.flag[ZF2] = 0;	/* file transport request */
	frame.flag[ZF3] = 0;
	sendBin32Frame(frame);
	send_zsda32(filedata, filedata_len, ZCRCW);

	if (zmodemFile_){
		delete zmodemFile_;
		zmodemFile_ = NULL;
	}
	zmodemFile_ = new ZmodemFile(W2A(uploadFilePath_.value().c_str()), basename, info.size);
	term_fresh_lastline(frontend_->term, 0, 
		zmodemFile_->getPrompt().c_str(), zmodemFile_->getPrompt().length());
}
//-----------------------------------------------------------------------------

unsigned short ZmodemSession::decodeCrc(const int index, int& consume_len)
{
	unsigned short ret = 0;
	decodeEscapeStruct<unsigned short>(index, consume_len, ret);
	return ret;
}

//-----------------------------------------------------------------------------

unsigned long ZmodemSession::decodeCrc32(const int index, int& consume_len)
{
	unsigned long ret = 0;
	decodeEscapeStruct<unsigned long>(index, consume_len, ret);
	return ret;
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleZdata()
{
	//curBuffer() with len buffer_.length() - decodeIndex_
	//offset in inputFrame_

	for (; lastCheckExcaped_ < buffer_.length() - 1; lastCheckExcaped_++, lastCheckExcapedSaved_++){
		if (buffer_[lastCheckExcaped_] == ZDLE){
			if (lastCheckExcaped_ + 6 > buffer_.length()){
				handleEvent(WAIT_DATA_EVT);
				return;
			}
			if (ZCRCE == buffer_[lastCheckExcaped_ + 1]){
				unsigned long calc_crc = ~UPDC32(buffer_[lastCheckExcaped_+1], dataCrc_);
				int consume_len = 0;
				unsigned long recv_crc = decodeCrc32(lastCheckExcaped_+2, consume_len);
				if (consume_len == 0){
					handleEvent(WAIT_DATA_EVT);
					return;
				}

				if (calc_crc == recv_crc){
					lastCheckExcaped_ += 1 + consume_len;
					if (!zmodemFile_->write(curBuffer(), lastCheckExcapedSaved_ - decodeIndex_)){
						handleEvent(RESET_EVT);
						return;
					}
					lastCheckExcapedSaved_ = lastCheckExcaped_;
					decodeIndex_ = lastCheckExcaped_+1;
					dataCrc_ = 0xFFFFFFFFL;
					
					std::string report_line(zmodemFile_->getProgressLine());
					term_fresh_lastline(frontend_->term, zmodemFile_->getPrompt().length(), 
						report_line.c_str(), report_line.length());
					term_data(frontend_->term, 0, "\r\n", 2);
					handleEvent(NETWORK_INPUT_EVT);
					return;
				}
			}else if (ZCRCG == buffer_[lastCheckExcaped_ + 1]){	
				unsigned long calc_crc = ~UPDC32(unsigned char(buffer_[lastCheckExcaped_+1]), dataCrc_);
				int consume_len = 0;
				unsigned long recv_crc = decodeCrc32(lastCheckExcaped_+2, consume_len);
				if (consume_len == 0){
					handleEvent(WAIT_DATA_EVT);
					return;
				}

				if (calc_crc == recv_crc){
					assert(lastCheckExcapedSaved_- decodeIndex_ == 1024);
					lastCheckExcaped_ += 1 + consume_len;
					if (!zmodemFile_->write(curBuffer(), lastCheckExcapedSaved_ - decodeIndex_)){
						handleEvent(RESET_EVT);
						return;
					}
					lastCheckExcapedSaved_ = lastCheckExcaped_;
					decodeIndex_ = lastCheckExcaped_+1;
					dataCrc_ = 0xFFFFFFFFL;
					continue;
				}else {
					handleEvent(RESET_EVT);
					return;
				}
				//else it is normal char

			}else if (ZCRCQ == buffer_[lastCheckExcaped_ + 1]){
				unsigned long calc_crc = ~UPDC32(buffer_[lastCheckExcaped_+1], dataCrc_);
				int consume_len = 0;
				unsigned long recv_crc = decodeCrc32(lastCheckExcaped_+2, consume_len);
				if (consume_len == 0){
					handleEvent(WAIT_DATA_EVT);
					return;
				}

				if (calc_crc == recv_crc){
					lastCheckExcaped_ += 1 + consume_len;
					if (!zmodemFile_->write(curBuffer(), lastCheckExcapedSaved_ - decodeIndex_)){
						handleEvent(RESET_EVT);
						return;
					}
					lastCheckExcapedSaved_ = lastCheckExcaped_;
					decodeIndex_ = lastCheckExcaped_+1;
					dataCrc_ = 0xFFFFFFFFL;
					sendFrameHeader(ZNAK, zmodemFile_->getPos());
					continue;
				}

			}else if (ZCRCW == buffer_[lastCheckExcaped_ + 1]){
				unsigned long calc_crc = ~UPDC32(buffer_[lastCheckExcaped_+1], dataCrc_);
				int consume_len = 0;
				unsigned long recv_crc = decodeCrc32(lastCheckExcaped_+2, consume_len);
				if (consume_len == 0){
					handleEvent(WAIT_DATA_EVT);
					return;
				}

				if (calc_crc == recv_crc){
					lastCheckExcaped_ += 1 + consume_len;
					if (!zmodemFile_->write(curBuffer(), lastCheckExcapedSaved_ - decodeIndex_)){
						handleEvent(RESET_EVT);
						return;
					}
					lastCheckExcapedSaved_ = lastCheckExcaped_;
					decodeIndex_ = lastCheckExcaped_+1;
					dataCrc_ = 0xFFFFFFFFL;
					sendFrameHeader(ZNAK, zmodemFile_->getPos());
					continue;
				}
			}else{
				lastCheckExcaped_++;
				buffer_[lastCheckExcapedSaved_] = buffer_[lastCheckExcaped_] ^ 0x40;
				dataCrc_ = UPDC32(unsigned char(buffer_[lastCheckExcapedSaved_]), dataCrc_);
			}
		}else{
			buffer_[lastCheckExcapedSaved_] = buffer_[lastCheckExcaped_] ;
			dataCrc_ = UPDC32(unsigned char(buffer_[lastCheckExcapedSaved_]), dataCrc_);
		}
	}
	eatBuffer();
	//zmodemFile_->write(curBuffer(), len);
	//decodeIndex_ += len;
	//eatBuffer();
	///recv_len_ += len;
	std::string report_line(zmodemFile_->getProgressLine());
	flow_control_fresh_lastline(frontend_->term, zmodemFile_->getPrompt().length(),
		report_line.c_str(), report_line.length());
	handleEvent(WAIT_DATA_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::sendZrpos()
{
	sendFrameHeader(ZRPOS, zmodemFile_->getPos());
}
//-----------------------------------------------------------------------------

int ZmodemSession::processNetworkInput(const char* const str, const int len)
{	
	if (!isDoingRz()){
		if (len < 7) return false;
	}
	if (getCurState().getId() ==  HANDLE_FRAME_STATE
		&& len == 1
		&& (*str == 'C' || *str == 'G' )){
		output("It has been timeout for file selection! Server starts to try xmodem/ymodem which is not supported!\r\n");
		reset();
		return true;
	}

	bufferParsed_ = false;
	buffer_.append(str, len);

	handleEvent(NETWORK_INPUT_EVT);
	return isDoingRz();
}

//-----------------------------------------------------------------------------

int ZmodemSession::onFileSelected(const FilePath& path)
{
	uploadFilePath_ = path;
	file_select_state_ = FILE_SELECTED;;
	handleEvent(FILE_SELECTED_EVT);
	return 0;
}

//-----------------------------------------------------------------------------

void ZmodemSession::reset()
{
	const char canistr[] =
	{
		24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0
	};
	frontend_->send(canistr, strlen(canistr));
	handleEvent(RESET_EVT);
	bufferParsed_ = false;
}

//-----------------------------------------------------------------------------

void ZmodemSession::output(const char* str)
{
	term_data(frontend_->term, 0, str, strlen(str));
}

//-----------------------------------------------------------------------------

void ZmodemSession::destroy()
{
	setDelete();
	asynHandleEvent(DESTROY_EVT);
}


void ZmodemSession::deleteSelf(Fsm::Session* session)
{
	ZmodemSession* zSession = dynamic_cast<ZmodemSession*>(session);
	delete zSession->frontend_;
	zSession->AddRef();
	zSession->Release();//	delete zSession in release
}


//-----------------------------------------------------------------------------

void ZmodemSession::flow_control_fresh_lastline(Terminal *term, int headerlen, const char *data, int len)
{
	uint64_t now = GetTickCount64();
	uint64_t diff = now - tick_;
	bool ignore = ((now / 100) % 10) >= 8; //20% must flow control
	if (ignore || (now - tick_ > 120)){
		tick_ = now;
		term_fresh_lastline(term, headerlen, data, len);
	}
	else
	{
		tick_ = tick_;
	}
}

//-----------------------------------------------------------------------------
