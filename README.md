![License MIT](https://img.shields.io/github/license/BehaviorTree/BehaviorTree.CPP?color=blue)
[![conan Ubuntu](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_ubuntu.yml/badge.svg)](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_ubuntu.yml)
[![conan Ubuntu](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_clang.yml/badge.svg)](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_clang.yml)
[![conan Windows](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/endurodave/StateMachineWithModernDelegates/actions/workflows/cmake_windows.yml)

# C++ State Machine with Asynchronous Delegates

A framework combining [C++ State Machine](https://github.com/endurodave/StateMachine) with the [DelegateMQ](https://github.com/endurodave/DelegateMQ) asynchronous delegate library.

# Table of Contents

- [C++ State Machine with Asynchronous Delegates](#c-state-machine-with-asynchronous-delegates)
- [Table of Contents](#table-of-contents)
- [Introduction](#introduction)
- [Getting Started](#getting-started)
- [Asynchronous Delegate Callbacks](#asynchronous-delegate-callbacks)
- [Self-Test Subsystem](#self-test-subsystem)
  - [SelfTestEngine](#selftestengine)
  - [CentrifugeTest](#centrifugetest)
  - [Timer](#timer)
- [Poll Events](#poll-events)
- [User Interface](#user-interface)
- [Run-Time](#run-time)
- [Conclusion](#conclusion)
- [References](#references)


# Introduction

<p>A software-based Finite State Machines (FSM) is an implementation method used to decompose a design into states and events. Simple embedded devices with no operating system employ single threading such that the state machines run on a single &ldquo;thread&rdquo;. More complex systems use multithreading to divvy up the processing.</p>

<p>This repository combines state machines and asynchronous delegates into a single project. The goal for the article is to provide a complete working project with threads, timers, events, and state machines all working together. To illustrate the concept, the example project implements a state-based self-test engine utilizing asynchronous communication between threads.</p>

<p>Related GitHub repositories:</p>

<ul>
    <li><a href="https://github.com/endurodave/DelegateMQ">DelegateMQ in C++</a> - a header-only library enables function invocations on any callable, either synchronously, asynchronously, or on a remote endpoint</li>
    <li><a href="https://github.com/endurodave/StateMachine">State Machine Design in C++</a> - a compact C++ state machine</li>
</ul>

# Getting Started
[CMake](https://cmake.org/) is used to create the project build files on any Windows or Linux machine.

1. Clone the repository.
2. From the repository root, run the following CMake command:   
   `cmake -B Build .`
3. Build and run the project within the `Build` directory. 

# Asynchronous Delegate Callbacks

<p>If you&rsquo;re not familiar with a delegate, the concept is quite simple. A delegate can be thought of as a super function pointer. In C++, there&#39;s no pointer type capable of pointing to all the possible function variations: instance member, virtual, const, static, and free (global). A function pointer can&rsquo;t point to instance member functions, and pointers to member functions have all sorts of limitations. However, delegate classes can, in a type-safe way, point to any function provided the function signature matches. In short, a delegate points to any function with a matching signature to support anonymous function invocation.</p>

<p>Asynchronous delegates take the concept a bit further and permits anonymous invocation of any function on a client specified thread of control. The function and all arguments are safely called from a destination thread simplifying inter-thread communication and eliminating cross-threading errors.&nbsp;</p>

<p>The DelegateMQ library is used throughout to provide asynchronous callbacks making&nbsp;an effective publisher and subscriber mechanism. A publisher exposes a delegate container interface and one or more subscribers add delegate instances to the container to receive anonymous callbacks.&nbsp;</p>

<p>The first place it&#39;s used is within the <code>SelfTest</code> class where the <code>SelfTest::CompletedCallback</code>&nbsp;delegate container allows subscribers to add delegates. Whenever a self-test completes a <code>SelfTest::CompletedCallback</code> callback is invoked notifying&nbsp;registered clients. <code>SelfTestEngine</code> registers with both&nbsp;<code>CentrifugeTest</code> and <code>PressureTest</code> to get asynchronously informed when the test is complete.</p>

<p>The second location is the user interface registers&nbsp;with <code>SelfTestEngine::StatusCallback</code>. This allows a client, running on another thread, to register and receive status callbacks during execution. <code>MulticastDelegateSafe&lt;&gt;</code> allows the client to specify the exact callback thread making is easy to avoid cross-threading errors.</p>

<p>The final location is within the <code>Timer</code> class, which fires periodic callbacks on a registered callback function. A generic, low-speed timer capable of calling a function on the client-specified thread is quite useful for event driven state machines where you might want to poll for some condition to occur. In this case, the <code>Timer</code> class is used to inject poll events into the state machine instances.</p>

# Self-Test Subsystem

<p>Self-tests execute a series of tests on hardware and mechanical systems to ensure correct operation. In this example, there are four state machine classes implementing our self-test subsystem as shown in the inheritance diagram below:</p>

<p align="center"><img height="191" src="Figure_1.png" width="377" /></p>

<p align="center"><strong>Figure 1: Self-Test Subsystem Inheritance Diagram</strong></p>

## SelfTestEngine

<p><code>SelfTestEngine</code> is thread-safe and the main point of contact for client&rsquo;s utilizing the self-test subsystem. <code>CentrifugeTest </code>and <code>PressureTest </code>are members of SelfTestEngine. <code>SelfTestEngine </code>is responsible for sequencing the individual self-tests in the correct order as shown in the state diagram below. &nbsp;</p>

<p align="center"><img height="370" src="Figure_2.png" width="530" /></p>

<p align="center"><strong>Figure 2: SelfTestEngine State Machine</strong></p>

<p>The <code>Start </code>event initiates the self-test engine.&nbsp;&nbsp;<code>SelfTestEngine::Start()</code> is an asynchronous function that reinvokes the <code>Start()</code> function if the caller is not on the correct execution thread. Perform a simple check whether the caller is executing on the desired thread of control. If not, a temporary asynchronous delegate is created on the stack and then invoked. The delegate and all the caller&rsquo;s original function arguments are duplicated on the heap and the function is reinvoked on <code>m_thread</code>. This is an elegant way to create asynchronous API&rsquo;s with the absolute minimum of effort. Since <code>Start()</code> is asynchronous, &nbsp;it is thread-safe to be called by any client running on any thread.&nbsp;</p>

<pre lang="c++">
void SelfTestEngine::Start(const StartData* data)
{
    // Is the caller executing on m_thread?
    if (m_thread.GetThreadId() != Thread::GetCurrentThreadId())
    {
        // Create an asynchronous delegate and reinvoke the function call on m_thread
        auto delegate = MakeDelegate(this, &amp;SelfTestEngine::Start, m_thread);
        delegate(data);
        return;
    }

    BEGIN_TRANSITION_MAP                                    // - Current State -
        TRANSITION_MAP_ENTRY (ST_START_CENTRIFUGE_TEST)     // ST_IDLE
        TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)                // ST_COMPLETED
        TRANSITION_MAP_ENTRY (CANNOT_HAPPEN)                // ST_FAILED
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                // ST_START_CENTRIFUGE_TEST
        TRANSITION_MAP_ENTRY (EVENT_IGNORED)                // ST_START_PRESSURE_TEST
    END_TRANSITION_MAP(data)
}</pre>

<p>When each self-test completes, the <code>Complete </code>event fires causing the next self-test to start. After all of the tests are done, the state machine transitions to <code>Completed&nbsp;</code>and back to <code>Idle</code>. If the <code>Cancel </code>event is generated at any time during execution, a transition to the <code>Failed </code>state occurs.</p>

<p>The <code>SelfTest </code>base class provides three states common to all <code>SelfTest</code>-derived state machines: <code>Idle</code>, <code>Completed</code>, and <code>Failed</code>. <code>SelfTestEngine </code>then adds two more states: <code>StartCentrifugeTest </code>and <code>StartPressureTest</code>.</p>

<p><code>SelfTestEngine </code>has one public event function, <code>Start()</code>, that starts the self-tests. <code>SelfTestEngine::StatusCallback</code> is an asynchronous callback allowing client&rsquo;s to register for status updates during testing. A <code>Thread </code>instance is also contained within the class. All self-test state machine execution occurs on this thread.</p>

<pre lang="c++">
class SelfTestEngine : public SelfTest
{
public:
    // Clients register for asynchronous self-test status callbacks
    static MulticastDelegateSafe&lt;void(const SelfTestStatus&amp;)&gt; StatusCallback;

    // Singleton instance of SelfTestEngine
    static SelfTestEngine&amp; GetInstance();

    // Start the self-tests. This is a thread-safe asycnhronous function. 
    void Start(const StartData* data);

    Thread&amp; GetThread() { return m_thread; }
    static void InvokeStatusCallback(std::string msg);

private:
    SelfTestEngine();
    void Complete();

    // Sub self-test state machines 
    CentrifugeTest m_centrifugeTest;
    PressureTest m_pressureTest;

    // Worker thread used by all self-tests
    Thread m_thread;

    StartData m_startData;

    // State enumeration order must match the order of state method entries
    // in the state map.
    enum States
    {
        ST_START_CENTRIFUGE_TEST = SelfTest::ST_MAX_STATES,
        ST_START_PRESSURE_TEST,
        ST_MAX_STATES
    };

    // Define the state machine state functions with event data type
    STATE_DECLARE(SelfTestEngine,     StartCentrifugeTest,    StartData)
    STATE_DECLARE(SelfTestEngine,     StartPressureTest,      NoEventData)

    // State map to define state object order. Each state map entry defines a
    // state object.
    BEGIN_STATE_MAP
        STATE_MAP_ENTRY(&amp;Idle)
        STATE_MAP_ENTRY(&amp;Completed)
        STATE_MAP_ENTRY(&amp;Failed)
        STATE_MAP_ENTRY(&amp;StartCentrifugeTest)
        STATE_MAP_ENTRY(&amp;StartPressureTest)
    END_STATE_MAP    
};</pre>

<p>As mentioned previously, the <code>SelfTestEngine </code>registers for asynchronous callbacks from each sub self-tests (i.e. <code>CentrifugeTest </code>and <code>PressureTest</code>) as shown below. When a sub self-test state machine completes, the <code>SelfTestEngine::Complete()</code> function is called. When a sub self-test state machine fails, the <code>SelfTestEngine::Cancel()</code> function is called.</p>

<pre lang="c++">
SelfTestEngine::SelfTestEngine() :
    SelfTest(ST_MAX_STATES),
    m_thread(&quot;SelfTestEngine&quot;)
{
    // Register for callbacks when sub self-test state machines complete or fail
    m_centrifugeTest.CompletedCallback += MakeDelegate(this, &amp;SelfTestEngine::Complete, m_thread);
    m_centrifugeTest.FailedCallback += MakeDelegate&lt;SelfTest&gt;(this, &amp;SelfTest::Cancel, m_thread);
    m_pressureTest.CompletedCallback += MakeDelegate(this, &amp;SelfTestEngine::Complete, m_thread);
    m_pressureTest.FailedCallback += MakeDelegate&lt;SelfTest&gt;(this, &amp;SelfTest::Cancel, m_thread);
}</pre>

<p>The <code>SelfTest&nbsp;</code>base class generates the <code>CompletedCallback </code>and <code>FailedCallback </code>within the <code>Completed </code>and <code>Failed</code> states respectively as seen below:</p>

<pre lang="c++">
STATE_DEFINE(SelfTest, Completed, NoEventData)
{
    SelfTestEngine::InvokeStatusCallback(&quot;SelfTest::ST_Completed&quot;);

    if (CompletedCallback)
        CompletedCallback();

    InternalEvent(ST_IDLE);
}

STATE_DEFINE(SelfTest, Failed, NoEventData)
{
    SelfTestEngine::InvokeStatusCallback(&quot;SelfTest::ST_Failed&quot;);

    if (FailedCallback)
        FailedCallback();

    InternalEvent(ST_IDLE);
}</pre>

<p>One might ask why the state machines use asynchronous delegate callbacks. If the state machines are on the same thread, why not use a normal, synchronous callback instead? The problem to prevent is a callback into a currently executing state machine, that is, the call stack wrapping back around into the same class instance. For example, the following call sequence should be prevented: <code>SelfTestEngine </code>calls <code>CentrifugeTest </code>calls back <code>SelfTestEngine</code>. An asynchronous callback allows the stack to unwind and prevents this unwanted behavior.</p>

## CentrifugeTest

<p>The <code>CentrifugeTest </code>state machine diagram shown below implements the centrifuge self-test described in &quot;<a href="https://github.com/endurodave/StateMachine"><strong>State Machine Design in C++</strong></a>&quot;. <code>CentrifugeTest</code> uses&nbsp;state machine inheritance by inheriting the <code>Idle</code>, <code>Completed</code> and <code>Failed</code> states from the <code>SelfTest</code> class.&nbsp;The difference here is that the <code>Timer</code> class is used to provide <code>Poll </code>events via asynchronous delegate callbacks.</p>

<p align="center"><img height="765" src="CentrifugeTest.png" width="520" /></p>

<p align="center"><strong>Figure 3: CentrifugeTest State Machine</strong></p>

## Timer

<p>The <code>Timer </code>class provides a common mechanism to receive function callbacks by registering with <code>Expired</code>. <code>Start()</code> starts the callbacks at a particular interval. <code>Stop()</code> stops the callbacks.</p>

<pre lang="c++">
/// @brief A timer class provides periodic timer callbacks on the client's 
/// thread of control. Timer is thread safe.
class Timer 
{
public:
	/// Client's register with Expired to get timer callbacks
	SinglecastDelegate<void(void)> Expired;

	/// Constructor
	Timer(void);

	/// Destructor
	~Timer(void);

	/// Starts a timer for callbacks on the specified timeout interval.
	/// @param[in]	timeout - the timeout in milliseconds.
	void Start(std::chrono::milliseconds timeout);

	/// Stops a timer.
	void Stop();
...</pre>

<p>All <code>Timer </code>instances are stored in a private static list. The <code>Thread::Process()</code> loop periodically services all the timers within the list by calling <code>Timer::ProcessTimers()</code>. Client&rsquo;s registered with <code>Expired </code>are invoked whenever the timer expires.</p>

<pre lang="c++">
        case MSG_TIMER:
            Timer::ProcessTimers();
            break;</pre>

# Poll Events

<p><code>CentrifugeTest </code>has a <code>Timer<strong> </strong></code>instance and registers for callbacks. The callback function, a thread instance and a this pointer is provided to <code>Register()</code> facilitating the asynchronous callback mechanism.</p>

<pre lang="c++">
// Register for timer callbacks
m_pollTimer.Expired = MakeDelegate(this, &amp;CentrifugeTest::Poll, &amp;SelfTestEngine::GetInstance().GetThread());</pre>

<p>When the timer is started using <code>Start()</code>, the <code>Poll()</code> event function is&nbsp;periodically called at the interval specified. Notice that when the <code>Poll()</code> external event function is called, a transition to either WaitForAcceleration or WaitForDeceleration&nbsp;is performed based on the current state of the state machine. If <code>Poll()</code> is called at the wrong time, the event is silently ignored.</p>

<pre lang="c++">
void CentrifugeTest::Poll()
{
&nbsp; &nbsp; BEGIN_TRANSITION_MAP &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; // - Current State -
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (EVENT_IGNORED) &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; // ST_IDLE
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (EVENT_IGNORED) &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; // ST_COMPLETED
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (EVENT_IGNORED) &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; // ST_FAILED
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (EVENT_IGNORED) &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; // ST_START_TEST
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (ST_WAIT_FOR_ACCELERATION) &nbsp; &nbsp;// ST_ACCELERATION
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (ST_WAIT_FOR_ACCELERATION) &nbsp; &nbsp;// ST_WAIT_FOR_ACCELERATION
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (ST_WAIT_FOR_DECELERATION) &nbsp; &nbsp;// ST_DECELERATION
&nbsp; &nbsp; &nbsp; &nbsp; TRANSITION_MAP_ENTRY (ST_WAIT_FOR_DECELERATION) &nbsp; &nbsp;// ST_WAIT_FOR_DECELERATION
&nbsp; &nbsp; END_TRANSITION_MAP(NULL)
}

STATE_DEFINE(CentrifugeTest, Acceleration, NoEventData)
{
&nbsp; &nbsp; SelfTestEngine::InvokeStatusCallback(&quot;CentrifugeTest::ST_Acceleration&quot;);

&nbsp; &nbsp; // Start polling while waiting for centrifuge to ramp up to speed
&nbsp; &nbsp; m_pollTimer.Start(10);
}
</pre>

# User Interface

<p>The project doesn&rsquo;t have a user interface except the text console output. For this example, the &ldquo;user interface&rdquo; just outputs self-test status messages on the user interface thread via the <code>SelfTestEngineStatusCallback()</code> function:</p>

<pre lang="c++">
Thread userInterfaceThread(&quot;UserInterface&quot;);

void SelfTestEngineStatusCallback(const SelfTestStatus&amp; status)
{
    // Output status message to the console &quot;user interface&quot;
    cout &lt;&lt; status.message.c_str() &lt;&lt; endl;
}</pre>

<p>Before the self-test starts, the user interface registers with the <code>SelfTestEngine::StatusCallback</code> callback.</p>

<pre>
SelfTestEngine::StatusCallback += 
&nbsp;     MakeDelegate(&amp;SelfTestEngineStatusCallback, userInterfaceThread);</pre>

<p>The user interface thread here is just used to simulate callbacks to a GUI library normally running in a separate thread of control.</p>

# Run-Time

<p>The program&rsquo;s <code>main()</code> function is shown below. It creates the two threads, registers for callbacks from <code>SelfTestEngine</code>, then calls <code>Start()</code> to start the self-tests.</p>

<pre lang="c++">
int main(void)
{    
    // Create the worker threads
    userInterfaceThread.CreateThread();
    SelfTestEngine::GetInstance().GetThread().CreateThread();

    // Register for self-test engine callbacks
    SelfTestEngine::StatusCallback += MakeDelegate(&amp;SelfTestEngineStatusCallback, userInterfaceThread);
    SelfTestEngine::GetInstance().CompletedCallback += 
&nbsp;        MakeDelegate(&amp;SelfTestEngineCompleteCallback, userInterfaceThread);
    
    // Start the worker threads
    ThreadWin::StartAllThreads();

    // Start self-test engine
    StartData startData;
    startData.shortSelfTest = TRUE;
    SelfTestEngine::GetInstance().Start(&amp;startData);

    // Wait for self-test engine to complete 
    while (!selfTestEngineCompleted)
        Sleep(10);

    // Unregister for self-test engine callbacks
    SelfTestEngine::StatusCallback -= MakeDelegate(&amp;SelfTestEngineStatusCallback, userInterfaceThread);
    SelfTestEngine::GetInstance().CompletedCallback -= 
&nbsp;        MakeDelegate(&amp;SelfTestEngineCompleteCallback, userInterfaceThread);

    // Exit the worker threads
    userInterfaceThread.ExitThread();
    SelfTestEngine::GetInstance().GetThread().ExitThread();

    return 0;
}</pre>

<p><code>SelfTestEngine </code>generates asynchronous callbacks on the <code>UserInteface </code>thread. The <code>SelfTestEngineStatusCallback()</code> callback outputs the message to the console.</p>

<pre lang="c++">
void SelfTestEngineStatusCallback(const SelfTestStatus&amp; status)
{
      // Output status message to the console &quot;user interface&quot;
      cout &lt;&lt; status.message.c_str() &lt;&lt; endl;
}</pre>

<p>The <code>SelfTestEngineCompleteCallback()</code> callback sets a flag to let the <code>main()</code> loop exit.</p>

<pre lang="c++">
void SelfTestEngineCompleteCallback()
{
      selfTestEngineCompleted = TRUE;
}</pre>

<p>Running the project outputs the following console messages:</p>

<p align="center"><img height="385" src="Figure_4.png" width="628" /></p>

<p align="center"><strong>Figure 4: Console Output</strong></p>

# Conclusion

<p>The <code>StateMachine</code> and <code>Delegate&lt;&gt;</code> implementations can be used separately. Each is useful unto itself. However, combining the two offers a novel framework for multithreaded state-driven application development. The article has shown how to coordinate the behavior of state machines when multiple threads are used,&nbsp;which may not be entirely obvious when looking at simplistic, single threaded examples.</p>

<p>I&rsquo;ve successfully used ideas similar to this on many different PC and embedded projects. The code is portable to any platform with a small amount of effort. I particularly like idea of asynchronous delegate callbacks because it effectively hides inter-thread communication and the organization of the state machines makes creating and maintaining self-tests easy.</p>

# References

<ul>
	<li><a href="https://github.com/endurodave/StateMachine"><strong>State Machine Design in C++</strong></a> - by David Lafreniere</li>
    <li><a href="https://github.com/endurodave/DelegateMQ"><strong>Asynchronous Multicast Delegates in Modern C++</strong></a> - by David Lafreniere</li>
	<li><a href="https://github.com/endurodave/StateMachineWithThreads"><strong>C++ State Machine with Threads</strong></a> &ndash; by David Lafreniere</li>
	<li><a href="https://github.com/endurodave/StdWorkerThread"><strong>C++ std::thread Event Loop with Message Queue and Timer</strong></a> - by David Lafreniere</li>
</ul>




