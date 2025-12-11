#include "DelegateMQ.h"
#include "SelfTestEngine.h"
#include <iostream>
#include "DataTypes.h"

// @see https://github.com/endurodave/StateMachineWithModernDelegates
// David Lafreniere

using namespace std;
using namespace dmq;

std::atomic<bool> processTimerExit = false;
static void ProcessTimers()
{
	while (!processTimerExit.load())
	{
		// Process all delegate-based timers
		Timer::ProcessTimers();
		std::this_thread::sleep_for(std::chrono::microseconds(50));
	}
}

// A thread to capture self-test status callbacks for output to the "user interface"
Thread userInterfaceThread("UserInterface");

// Simple flag to exit main loop
BOOL selfTestEngineCompleted = FALSE;

//------------------------------------------------------------------------------
// SelfTestEngineStatusCallback
//------------------------------------------------------------------------------
void SelfTestEngineStatusCallback(const SelfTestStatus& status)
{
	// Output status message to the console "user interface"
	cout << status.message.c_str() << endl;
}

//------------------------------------------------------------------------------
// SelfTestEngineCompleteCallback
//------------------------------------------------------------------------------
void SelfTestEngineCompleteCallback()
{
	selfTestEngineCompleted = TRUE;
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{	
	// Start the thread that will run ProcessTimers
	std::thread timerThread(ProcessTimers);

	// Create the worker threads
	userInterfaceThread.CreateThread();
	SelfTestEngine::GetInstance().GetThread().CreateThread();

	// Register for self-test engine callbacks
	SelfTestEngine::StatusCallback += MakeDelegate(&SelfTestEngineStatusCallback, userInterfaceThread);
	SelfTestEngine::GetInstance().CompletedCallback += MakeDelegate(&SelfTestEngineCompleteCallback, userInterfaceThread);
	
	// Start self-test engine
	StartData startData;
	startData.shortSelfTest = TRUE;
	SelfTestEngine::GetInstance().Start(&startData);

	// Wait for self-test engine to complete 
	while (!selfTestEngineCompleted)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// Unregister for self-test engine callbacks
	SelfTestEngine::StatusCallback -= MakeDelegate(&SelfTestEngineStatusCallback, userInterfaceThread);
	SelfTestEngine::GetInstance().CompletedCallback -= MakeDelegate(&SelfTestEngineCompleteCallback, userInterfaceThread);

	// Exit the worker threads
	userInterfaceThread.ExitThread();
	SelfTestEngine::GetInstance().GetThread().ExitThread();

	// Ensure the timer thread completes before main exits
	processTimerExit.store(true);
	if (timerThread.joinable())
		timerThread.join();

	return 0;
}

