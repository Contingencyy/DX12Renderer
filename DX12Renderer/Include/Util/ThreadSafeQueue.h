#pragma once

template <typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue<T>() {}

	void Push(T value)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_InternalQueue.push(value);
	}

	bool TryPop(T& value)
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		if (m_InternalQueue.empty())
			return false;

		value = m_InternalQueue.front();
		m_InternalQueue.pop();
		return true;
	}

	bool Empty() const
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		return m_InternalQueue.empty();
	}

	std::size_t Size() const
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		return m_InternalQueue.size();
	}

private:
	std::queue<T> m_InternalQueue;
	mutable std::mutex m_QueueMutex;

};
