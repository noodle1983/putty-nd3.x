#include "zmodem_session.h"
#include "putty.h"
#include "crctab.c"
#include "native_putty_controller.h"

#include "zmodem.h"
base::Lock ZmodemSession::fsmLock_;
std::auto_ptr<Fsm::FiniteStateMachine> ZmodemSession::fsm_;

const char HEX_PREFIX[] = {ZPAD, ZPAD, ZDLE, ZHEX};
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

			(*fsm) += FSM_STATE(CHK_FRAME_TYPE_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::checkFrametype);
			(*fsm) +=      FSM_EVENT(PARSE_HEX_EVT,   CHANGE_STATE(PARSE_HEX_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN_EVT,   CHANGE_STATE(PARSE_BIN_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN32_EVT, CHANGE_STATE(PARSE_BIN32_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(PARSE_HEX_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseHexFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseHexFrame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(PARSE_BIN_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseBinFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseBinFrame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

			(*fsm) += FSM_STATE(PARSE_BIN32_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::parseBin32Frame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::parseBin32Frame);
			(*fsm) +=      FSM_EVENT(HANDLE_FRAME_EVT,  CHANGE_STATE(HANDLE_FRAME_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());


			(*fsm) += FSM_STATE(HANDLE_FRAME_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,  &ZmodemSession::handleFrame);
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(CHK_FRAME_TYPE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());

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
{
	output_.reserve(128);
	inputFrame_ = new frame_t;
}

//-----------------------------------------------------------------------------

ZmodemSession::~ZmodemSession()
{
	delete inputFrame_;
}

//-----------------------------------------------------------------------------

void ZmodemSession::initState()
{
	buffer_.clear();
	decodeIndex_ = 0;
}

//-----------------------------------------------------------------------------

void ZmodemSession::checkFrametype()
{
	for (; decodeIndex_ < buffer_.length() 
		&& ZPAD != buffer_[decodeIndex_] ; decodeIndex_ ++){
		output_.push_back(buffer_[decodeIndex_]);
	}
	for (; decodeIndex_ < buffer_.length() 
		&& ZPAD == buffer_[decodeIndex_] ; decodeIndex_ ++);

	if (decodeIndex_ + 2 >= buffer_.length()){
		handleEvent(RESET_EVT);
		return;
	}

	if (ZDLE != buffer_[decodeIndex_++]){
		output_.append("expect the leading ZDLE\r\n");
		handleEvent(RESET_EVT);
		return;
	}

	int frametype = buffer_[decodeIndex_++];
	if (ZHEX == frametype){
            output_.append("\r\nhex frame \r\n");
            handleEvent(PARSE_HEX_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else if (ZBIN == frametype){
			output_.append("\r\nbin frame \r\n");
			handleEvent(PARSE_BIN_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else if (ZBIN32 == frametype){
			output_.append("\r\nbin frame \r\n");
			handleEvent(PARSE_BIN32_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else{
		output_.append("\r\nonly support(HEX,BIN,BIN32) frame\r\n");
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
		output_.append("crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }

	int old_index = decodeIndex_;
	for (; decodeIndex_ < buffer_.length() 
		&& ('\r' == buffer_[decodeIndex_] 
			|| '\n' == buffer_[decodeIndex_]
			|| -118 == buffer_[decodeIndex_]) ; decodeIndex_ ++);
	if (old_index == decodeIndex_){
		output_.append("no line seed found!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	if (frame.type != ZACK && frame.type != ZFIN && buffer_[decodeIndex_++] != XON){
		output_.append("XON expected!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer(decodeIndex_);
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
    memcpy(&frame, curBuffer(), sizeof(frame_t));
	decodeIndex_ += sizeof(frame_t);

    if (frame.crc != calcFrameCrc(&frame)){
		output_.append("bin32 crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer(decodeIndex_);
	memcpy(inputFrame_, &frame, sizeof(frame_t));
	handleEvent(HANDLE_FRAME_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::parseBin32Frame()
{
	if (decodeIndex_ + sizeof(frame32_t) >= buffer_.length())
		return;
	frame32_t frame;
    memcpy(&frame, curBuffer(), sizeof(frame32_t));
	decodeIndex_ += sizeof(frame32_t);

    if (frame.crc != calcFrameCrc32(&frame)){
		output_.append("bin32 crc error!\r\n");
        handleEvent(RESET_EVT);
        return ;
    }
	eatBuffer(decodeIndex_);
	memcpy(inputFrame_, &frame, sizeof(frame_t));
	handleEvent(HANDLE_FRAME_EVT);
	return;
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleFrame()
{
	switch (inputFrame_->type){
    case ZRQINIT:
        return handleZrqinit();

    case ZFILE: 
		return handleZfile();
    case ZRINIT:
    case ZSINIT:
    case ZACK:
    case ZSKIP:
    case ZNAK:
    case ZABORT:
    case ZFIN:
    case ZRPOS:
    case ZDATA:
    case ZEOF:
    case ZFERR:
    case ZCRC:
    case ZCHALLENGE:
    case ZCOMPL:
    case ZCAN:
    case ZFREECNT:
    case ZCOMMAND:
    case ZSTDERR: 

    default:
        output_.append("invalid frame type!\r\n");
        handleEvent(RESET_EVT);
        return ;


    }
    return ;


}

//-----------------------------------------------------------------------------

void ZmodemSession::sendFrame(char type)
{
	frame_t frame;
    memset(&frame, 0, sizeof(frame_t));
    frame.type = type;
    frame.flag[ZF0] = CANFC32|CANFDX|CANOVIO;
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
	if (type != ZFIN && type != ZACK){
		buf[len++] = XON;
	}
    frontend_->send(buf, len);
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleZrqinit()
{
	sendFrame(ZRINIT);
}

//-----------------------------------------------------------------------------

void ZmodemSession::handleZfile()
{
	unsigned oldIndex = decodeIndex_;
	std::string filename(curBuffer());
	decodeIndex_ += filename.length() + 1;
	std::string fileinfo(curBuffer());
	decodeIndex_ += fileinfo.length() + 1;

	if (decodeIndex_ + 7 > buffer_.length()){
		output_.append("zfile frame invalid!\r\n");
        handleEvent(RESET_EVT);
        return ;
	}
	buffer_[decodeIndex_] = buffer_[decodeIndex_+1];
	decodeIndex_++;
	unsigned long crc = calcBufferCrc32(buffer_.c_str() + oldIndex, decodeIndex_ - oldIndex);

	decodeIndex_++;
	unsigned long recv_crc = 0;
	memcpy(&recv_crc, curBuffer(), sizeof (unsigned long));
	decodeIndex_ += sizeof (unsigned long);

	if (*curBuffer() == XON){
		decodeIndex_++;
	}

	if (recv_crc != crc){
		output_.append("zfile frame crc invalid!\r\n");
        handleEvent(RESET_EVT);
        return ;
	}
	eatBuffer(decodeIndex_);

	sendFrame(ZNAK);
}

//-----------------------------------------------------------------------------

int ZmodemSession::processNetworkInput(const char* const str, const int len, std::string& output)
{
	buffer_.append(str, len);
	handleEvent(NETWORK_INPUT_EVT);
	output = output_;
	output_.clear();
	output_.reserve(128);
	return isDoingRz();
}

//-----------------------------------------------------------------------------