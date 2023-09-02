#pragma once
#include <atomic>
/// <summary>
/// Generate unique ids.
/// </summary>
class IdGenerator {
private:
	std::atomic<int> free_id;
public:
	IdGenerator() : free_id(0) {}

	// Non copyable and non copy constructble
	IdGenerator& operator=(IdGenerator&) = delete;
	IdGenerator(IdGenerator&) = delete;

	/// <summary>
	/// Get next unique id.
	/// </summary>
	/// <returns>Returns unique id.</returns>
	int getId() {
		int oldId, newId;
		// Value of free_id can only be incremented. Only exchange values if freeId has not changed during increment.
		do {
			oldId = free_id;
			newId = oldId + 1;
		} while (!free_id.compare_exchange_weak(oldId, newId));
		return oldId;
	}
};