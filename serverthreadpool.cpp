#include <vector>
#include <thread>
#include <condition_variable>
#include <functional>
#include <queue>

class serverthreadpool {

private:
	int threadnumber = 3;
	int activeThreads = 0;
	std::queue<std::function<void ()>> connectQueue;
	std::vector<std::thread> threadVector;
	std::mutex mtx_connectRequest;
	std::condition_variable convar;

	void threadloop() {
		while (true) {
			std::function<void()> connect;
			{
				std::unique_lock<std::mutex> lock(mtx_connectRequest);
				convar.wait(lock, [this] { return !connectQueue.empty(); });
				activeThreads++;
				connect = connectQueue.front();
				connectQueue.pop();
			}
			connect();
			activeThreads--;
		}
	}

public:
	serverthreadpool(int threads) {
		threadnumber = threads;
	}

	void start() {
		threadVector.resize(threadnumber);
		for (int i = 0; i <= threadnumber - 1; i++) {
			threadVector[i] = std::thread(&serverthreadpool::threadloop, this);
		}
	}

	void acceptConnection(const std::function<void()>& clientConnection) {
		{
			std::unique_lock<std::mutex> lock(mtx_connectRequest);
			connectQueue.push(clientConnection);
		}
		convar.notify_one();
	}


};