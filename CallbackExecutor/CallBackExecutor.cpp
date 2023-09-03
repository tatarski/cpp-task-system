
#include "CallBackExecutor.h"
namespace TaskSystem {

	CallBackExecutor::ExecStatus CallBackExecutor::ExecuteStep(int threadIndex, int threadCount) {
		int oldTaskIndex = completeC;
		int newTaskIndex;

		// Early stop
		if (oldTaskIndex >= callbackCount) {
			setCallbacksComplete();
			return ExecStatus::ES_Stop;
		}

		// Acquite some task index
		do {
			oldTaskIndex = completeC;
			newTaskIndex = oldTaskIndex + 1;

		} while (!completeC.compare_exchange_weak(oldTaskIndex, newTaskIndex));

		// Index is out of callback list
		if (newTaskIndex - 1 >= callbackCount) {
			setCallbacksComplete();
			return ExecStatus::ES_Stop;

		}

		// Call callback with task context index
	 	tc->onCompleteCallbacks[newTaskIndex - 1](tc->id);

		// Stop if cur callback is last callback
		if (newTaskIndex == callbackCount) {
			setCallbacksComplete();
			return ExecStatus::ES_Stop;
		}
		else {
			return ExecStatus::ES_Continue;
		}
	};
}