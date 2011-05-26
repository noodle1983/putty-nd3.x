
#include <tchar.h>
#include <windows.h>

#include <iostream>

#include "base/at_exit.h"
#include "base/command_line.h"

#include "message/message_loop.h"

class DummyTask : public Task
{
    static int count_;

public:
    void Run()
    {
        std::cout << count_++ << std::endl;
        if(count_ == 10)
        {
            MessageLoop::current()->Quit();
        }
        else
        {
            MessageLoop::current()->PostDelayedTask(new DummyTask(), 1000);
        }
    }
};

int DummyTask::count_ = 0;


int main()
{
    base::AtExitManager exit_manager;
    base::CommandLine::Init(0, NULL);

    MessageLoop loop;
    loop.PostDelayedTask(new DummyTask(), 1000);
    loop.Run();

    return 0;
}