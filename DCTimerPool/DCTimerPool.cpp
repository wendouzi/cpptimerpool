#include "DCTimerPool.h"

TimerPool::TimerPool()
	: m_run(true)
	, m_thread{ [this]() { run(); } }
{

}

TimerPool::~TimerPool()
{
	stop();

	if (m_thread.joinable())
		m_thread.join();
}

TimerPool::TimerHandle TimerPool::createTimer()
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);

	auto newTimer = std::make_shared<Timer>(*this);
	m_timers.emplace_front(newTimer);

	return newTimer;
}

void TimerPool::run()
{
	std::unique_lock<decltype(m_mutex)> lock(m_mutex);

	while (m_run)
	{
		auto nowTime = Clock::now();

		auto wakeTime = Clock::time_point::max();

		for (auto& t : m_timers)
		{
			const auto expiryTime = t->nextExpiry();

			if (nowTime >= expiryTime)
				t->fire();
			else if (expiryTime < wakeTime)
				wakeTime = expiryTime;
		}

		m_cond.wait_until(lock, wakeTime, [this]() { return ! m_run; });
	}
}

void TimerPool::stop()
{
	std::lock_guard<decltype(m_mutex)> lock(m_mutex);
	m_run = false;
	m_cond.notify_all();
}
