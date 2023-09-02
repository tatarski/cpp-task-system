
#include "CallBackExecutor.h"
namespace TaskSystem {
	CallBackExecutor::ExecStatus CallBackExecutor::ExecuteStep(int threadIndex, int threadCount) {
		int oldTaskIndex;
		int newTaskIndex;

		// Acquite some task index
		do {
			oldTaskIndex = completeC;
			newTaskIndex = oldTaskIndex + 1;

		} while (!completeC.compare_exchange_weak(oldTaskIndex, newTaskIndex));

		// Stop if index out of callback list
		if (newTaskIndex - 1 >= callbackCount) {
			setCallbacksComplete();
			return ExecStatus::ES_Stop;

		}

		// Call callback with task context index
	 	tc->onCompleteCallbacks[newTaskIndex - 1](tc->id);

		if (newTaskIndex == callbackCount) {
			setCallbacksComplete();
			return ExecStatus::ES_Stop;
		}
		else {
			return ExecStatus::ES_Continue;
		}
	};
}