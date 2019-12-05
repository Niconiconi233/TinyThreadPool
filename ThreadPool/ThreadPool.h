#pragma once

#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <vector>
#include <iostream>

using std::cout;
using std::endl;

class ThreadPool
{
public:
	ThreadPool(size_t n);
	template<typename F, typename... Args>
	auto enqueue(F&& f, Args&&... args)->std::future<typename std::result_of<F(Args...)>::type>;

	void stop()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		{
			this->stop_ = true;
		}
		cond_.notify_all();
	}

	~ThreadPool();

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()> > tasks;
	std::mutex mutex_;
	std::condition_variable cond_;
	volatile bool stop_;

};

inline ThreadPool::ThreadPool(size_t threads) :stop_(false)
{
	for (size_t i = 0; i < threads; ++i)
	{
		workers.emplace_back([this] {
			for (;;)
			{
				std::function<void()> task;
				{
					std::unique_lock<std::mutex> lock(mutex_);
					cond_.wait(lock, [this] {return this->stop_ || !this->tasks.empty(); });
					if (this->stop_ && this->tasks.empty())
						return;
					task = std::move(this->tasks.front());
					this->tasks.pop();
				}
				task();
			}
			});
	}
}

template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared<std::packaged_task<return_type()> >(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (stop_)
			throw std::runtime_error("error");

		tasks.emplace([task]() {(*task)(); });//保存的函数实际是该函数 拷贝
		cout << "main " << std::this_thread::get_id() << endl;
	}
	cond_.notify_one();
	return res;

}

inline ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(mutex_);
		stop_ = true;
	}
	cond_.notify_all();
	for (auto& i : workers)
		i.join();
}
