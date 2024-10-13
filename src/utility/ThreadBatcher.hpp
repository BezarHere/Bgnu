#pragma once
#include <thread>
#include <vector>

template <typename Func, typename FuncExporter>
class ThreadBatcher
{
public:
	typedef uint32_t thread_batch_size_t;

	template <typename T>
	struct scoped_setter
	{
		inline ~scoped_setter() {
			ref = value;
		}

		T &ref;
		T value;
	};

	struct ThreadInfo
	{
		std::string name;
	};

	ThreadBatcher(uint32_t batch_size, Func function, FuncExporter exporter);
	inline ~ThreadBatcher() noexcept(false) {
		if (is_running())
		{
			throw std::runtime_error("ThreadBatcher deconstructed while running");
		}
	}

	void run(uint16_t jobs_count);

	inline void set_batch_size(uint32_t size) {
		m_batch_size = size;
	}

	inline uint32_t get_batch_size() const {
		return m_batch_size;
	}

	inline bool is_running() const {
		return m_running;
	}

private:
	bool m_running = false;
	thread_batch_size_t m_batch_size;
	Func m_function;
	FuncExporter m_exporter;
};

template<typename Func, typename FuncExporter>
inline ThreadBatcher<Func, FuncExporter>::ThreadBatcher(thread_batch_size_t batch_size,
																												Func function, FuncExporter exporter)
	: m_batch_size{batch_size},
	m_function{function}, m_exporter{exporter} {

	set_batch_size(m_batch_size);
}

template<typename Func, typename FuncExporter>
inline void ThreadBatcher<Func, FuncExporter>::run(const uint16_t jobs_count) {
	if (jobs_count == 0)
	{
		return;
	}

	m_running = true;
	scoped_setter<bool> _running_resetter{m_running, false};

	const auto worker_wrapper = [this](const IndexRange jobs_range) {
		for (size_t index = jobs_range.begin; index < jobs_range.end; index++)
		{
			this->m_exporter(
				index,
				this->m_function(index)
			);
		}

		};

	const size_t workers_to_deploy = std::min<uint32_t>(
		jobs_count,
		m_batch_size == 0 ? jobs_count : m_batch_size
	);

	std::unique_ptr<std::thread[]> _p_threads{new std::thread[workers_to_deploy]};
	std::thread *threads = _p_threads.get();

	const size_t jobs_per_worker =
		jobs_count / workers_to_deploy;

	// the remainder of the `jobs_per_worker` div
	const size_t work_overflow_size = jobs_count - (jobs_per_worker * workers_to_deploy);


	size_t jobs_left = jobs_count;
	size_t worker_index = 0;
	size_t worker_overflow_to_dispatch = work_overflow_size;
	while (jobs_left > 0 && jobs_left <= jobs_count)
	{
		const size_t overflow_job = worker_overflow_to_dispatch > 0 ? 1 : 0;
		const size_t jobs_to_dispatch = std::min(jobs_per_worker + overflow_job, jobs_left);
		jobs_left -= jobs_to_dispatch;

		if (worker_overflow_to_dispatch > 0)
		{
			worker_overflow_to_dispatch--;
		}

		threads[worker_index++] =
			std::thread{worker_wrapper, IndexRange(jobs_left, jobs_left + jobs_to_dispatch)};
	}

	const size_t workers_dispatched = worker_index;

	// worker index after the dispatching should the be equal to the worker deploy count
	// unless there was a worker not dispatched
	if (workers_dispatched != workers_to_deploy)
	{
		Logger::error(
			"Worker Ill-Dispatch: worker threads deployed %llu, but workers dispatched: %llu, jobs_left/count=%llu/%llu, ",
			workers_to_deploy,
			workers_dispatched,
			jobs_left,
			jobs_count
		);
	}


	for (size_t i = 0; i < workers_dispatched; i++)
	{
		threads[i].join();
	}

}
