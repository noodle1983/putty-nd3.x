#include "zmodem_session.h"
#include "putty.h"
#include "zmodem.h"

base::Lock ZmodemSession::fsmLock_;
std::auto_ptr<Fsm::FiniteStateMachine> ZmodemSession::fsm_;

const char HEX_PREFIX[] = {ZPAD, ZPAD, ZDLE, ZHEX};
const char HEX_ARRAY[] = "0123456789abcdef";

const char BIN_PREFIX[] = {ZPAD, ZDLE, ZBIN};
const char BIN32_PREFIX[] = {ZPAD, ZDLE, ZBIN32};

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
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  &ZmodemSession::checkFrametype);
			(*fsm) +=      FSM_EVENT(PARSE_HEX_EVT,   CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN_EVT,   CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(PARSE_BIN32_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(RESET_EVT        ,  CHANGE_STATE(IDLE_STATE));

			(*fsm) += FSM_STATE(CHK_FRAME_TYPE_STATE);
			(*fsm) +=      FSM_EVENT(Fsm::ENTRY_EVT,   NEW_TIMER(10 * 1000));
			(*fsm) +=      FSM_EVENT(NETWORK_INPUT_EVT,  CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::TIMEOUT_EVT, CHANGE_STATE(IDLE_STATE));
			(*fsm) +=      FSM_EVENT(Fsm::EXIT_EVT,    CANCEL_TIMER());



			fsm_.reset(fsm);
        }

    }
    return fsm_.get();
}

//-----------------------------------------------------------------------------

ZmodemSession::ZmodemSession(void* frontend)
	: Fsm::Session(getZmodemFsm(), 0)
	, frontend_(frontend)
{
	output_.reserve(128);
}

//-----------------------------------------------------------------------------

ZmodemSession::~ZmodemSession()
{

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
		logevent(frontend_, "expect the leading ZDLE\n");
		handleEvent(RESET_EVT);
		return;
	}

	int frametype = buffer_[decodeIndex_++];
	if (ZHEX == frametype){
            logevent(frontend_, "hex frame: ");
            handleEvent(PARSE_HEX_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else if (ZBIN == frametype){
			logevent(frontend_, "bin frame: ");
			handleEvent(PARSE_BIN_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else if (ZBIN32 == frametype){
			logevent(frontend_, "bin frame: ");
			handleEvent(PARSE_BIN32_EVT);
			eatBuffer(decodeIndex_);
			return;
	}else{
		logevent(frontend_, "only support(HEX,BIN,BIN32) frame\n");
		handleEvent(RESET_EVT);
		return;
	}
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