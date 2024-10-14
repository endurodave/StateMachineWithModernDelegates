#include "DelegateLib.h"
#include "SelfTestEngine.h"
#include <iostream>
#include "WorkerThreadStd.h"
#include "DataTypes.h"

// @see https://github.com/endurodave/StateMachineWithModernDelegates
// David Lafreniere

using namespace std;
using namespace DelegateLib;

// A thread to capture self-test status callbacks for output to the "user interface"
WorkerThread userInterfaceThread("UserInterface");

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
	// Create the worker threads
	userInterfaceThread.CreateThread();
	SelfTestEngine::GetInstance().GetThread().CreateThread();

	// Register for self-test engine callbacks
	SelfTestEngine::StatusCallback += MakeDelegate(&SelfTestEngineStatusCallback, userInterfaceThread);
	SelfTestEngine::GetInstance().CompletedCallback += MakeDelegate(&SelfTestEngineCompleteCallback, userInterfaceThread);
	
#if USE_WIN32_THREADS
	// Start the worker threads
	ThreadWin::StartAllThreads();
#endif

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

	return 0;
}

