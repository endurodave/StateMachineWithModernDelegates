#ifndef _DELEGATE_MSG_H
#define _DELEGATE_MSG_H

/// @file
/// @brief Delegate inter-thread message base class. 

#include "IInvoker.h"
#include "DelegateOpt.h"
#include "Semaphore.h"
#include "make_tuple_heap.h"
#include <tuple>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace dmq {

// Async delegate message priority
enum class Priority
{
	LOW,
	NORMAL,
	HIGH
};

/// @brief Base class for all delegate inter-thread messages
class DelegateMsg
{
public:
	/// Constructor
	/// @param[in] invoker - the invoker instance the delegate is registered with.
	DelegateMsg(std::shared_ptr<IThreadInvoker> invoker, Priority priority) :
		m_priority(priority), m_invoker(invoker)
	{
	}

	virtual ~DelegateMsg() = default;

	/// Get the delegate invoker instance the delegate is registered with.
	/// @return The invoker instance. 
	std::shared_ptr<IThreadInvoker> GetInvoker() const { return m_invoker; }

	/// Get the delegate message priority
	/// @return Delegate message priority
	Priority GetPriority() { return m_priority; }

private:
	/// The IThreadInvoker instance used to invoke the target function 
    /// on the destination thread of control
	std::shared_ptr<IThreadInvoker> m_invoker;

	/// The delegate message priority
	Priority m_priority = Priority::NORMAL;
};

}

#endif
