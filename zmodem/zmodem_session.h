#ifndef ZMODEM_SESSION_H
#define ZMODEM_SESSION_H

#include "Fsm.h"
#include "Action.h"
#include "State.h"
#include "Session.h"
#include <iostream>
#include <memory>
#include <base/synchronization/lock.h>
#include <string>

class ZmodemSession: public Fsm::Session
{
public:
	enum MyState
	{
		IDLE_STATE = 1,
		CHK_FRAME_TYPE_STATE,

		END_STATE
	};
	enum MyEvent
	{
		NETWORK_INPUT_EVT = 0,
		RESET_EVT,
		PARSE_HEX_EVT,
		PARSE_BIN_EVT,
		PARSE_BIN32_EVT,
		NEXT_EVT
	};
	typedef enum{
		DECODE_ERROR  = -1,
		DECODE_DONE   = 0,
		DECODE_PARTLY = 1
	} DecodeResult;
	static Fsm::FiniteStateMachine* getZmodemFsm();

	ZmodemSession(void* frontend);
	virtual ~ZmodemSession();
	void initState();
	int processNetworkInput(const char* const str, const int len);

	bool isDoingRz(){return getCurState().getId() !=  IDLE_STATE;};
	int lengthToBeDecode(){return buffer_.length() - decodeIndex_;};
	const char* bufferToBeDecode(){return buffer_.c_str() + decodeIndex_;}

	void checkIfStartRz();
	void checkFrametype();

	template<typename TheStruct>
	DecodeResult decode(TheStruct& object)
	{
		if (lengthToBeDecode() < sizeof(TheStruct))
			return DECODE_PARTLY;
		memcpy(&object, bufferToBeDecode(), sizeof(TheStruct));
		decodeIndex_ += sizeof(TheStruct);
		return DECODE_DONE;
	}
private:
	static base::Lock fsmLock_;
	static std::auto_ptr<Fsm::FiniteStateMachine> fsm_;

	std::string buffer_;
	int decodeIndex_;
	void* frontend_;
};

#endif /* ZMODEM_SESSION_H */
