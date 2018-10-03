#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>

template <class T>
class Queue
{
public:

	template<class U>
	void push(U&& val)
	{
		static_assert(
			std::is_convertible_v<std::decay_t<T>, std::decay_t<U>>,
			"push must accept forwarding reference on convertable to std::decay<T>"
		);
		std::unique_lock lk(m_m);
		m_queue.push(std::forward<T>(val));
		lk.unlock();
		m_cv.notify_one();
	}


	T pull()
	{
		std::unique_lock lk(m_m);
		m_cv.wait(lk, [&q=m_queue] { return !q.empty(); });
		auto ret_val = std::move(m_queue.front());
		m_queue.pop();
		return ret_val;
	}

private:

	std::mutex m_m;
	std::condition_variable m_cv;
	std::queue<T> m_queue;
};

int main()
{
	Queue<int> int_queue;

	std::mutex stream_mutext;

	std::thread read(
		[&q=int_queue, &m=stream_mutext]
		{
			for (size_t i = 10; i != 0; --i)
			{
				auto val = q.pull();
				std::lock_guard lck(m);
				std::cout << "'" << val
				<< "'\twas read    by\t"
				<< std::this_thread::get_id() << '\n'; 
			}
		}
	);

	auto &&q = int_queue;
	auto &&m = stream_mutext;
	std::srand(std::time(0));
	for (size_t i = 0; i != 10; ++i)
	{
		auto val = std::rand();
		q.push(val); 
		std::lock_guard lck(m);
		std::cout << "'" << val
			<< "'\twas written by\t"
			<< std::this_thread::get_id() << '\n'; 
	}

	read.join();

	return 0;
}
