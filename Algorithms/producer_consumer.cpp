// Patryk Bandyra
#if 1
#include<iostream>
#include<thread>
#include<mutex>
#include<deque>
#include<random>
#include<chrono>

/*
	Starting parameters
*/
#define SLEEP_TIME 500	// sleep time for a thread in milliseconds
#define MAX_BUFFER_SIZE 32

/*
Wrapper around deque container
*/
class MyQueue {
private:
	std::deque<int> fifo;
	const unsigned int max_size;

public:
	MyQueue(int max_size) : max_size(max_size) {}
	void push_back(int value) {
		fifo.push_back(value);
	}
	void pop_front() {
		fifo.pop_front();
	}
	int get_head() {
		return fifo.front();
	}
	int get_current_size() {
		return fifo.size();
	}
	int count_even() {
		int even_counter = 0;
		for (auto i : fifo) {
			if (i % 2 == 0)
				++even_counter;
		}
		return even_counter;
	}
	int count_odd() {
		int odd_counter = 0;
		for (auto i : fifo) {
			if (i % 2 == 1)
				++odd_counter;
		}
		return odd_counter;
	}
	void print() {
		std::cout << "[ ";
		for (auto i : fifo) {
			std::cout << i << " ";
		}
		std::cout << "]" << std::endl;
	}
};

/*
	Global variables section
*/
std::mutex m;	// mutex to allow access to a buffer
std::condition_variable prod_even_cv, prod_odd_cv, cons_even_cv, cons_odd_cv;		// interprocess comunication
const unsigned int max_buffer_size = MAX_BUFFER_SIZE;
MyQueue buffer(max_buffer_size);	// fifo buffer

/*
	Functions section
*/

bool can_produce_even() {
	if (buffer.get_current_size() < max_buffer_size && buffer.count_even() < 10)
		return true;
	return false;
}

bool can_produce_odd() {
	if (buffer.get_current_size() < max_buffer_size && buffer.count_even() > buffer.count_odd())
		return true;
	return false;
}

bool can_consume_even() {
	if (buffer.get_current_size() >= 3 && buffer.get_head() % 2 == 0)
		return true;
	return false;
}

bool can_consume_odd() {
	if (buffer.get_current_size() >= 7 && buffer.get_head() % 2 == 1)
		return true;
	return false;
}

void fill_buffer() {
	for (int i = 0; i < max_buffer_size; ++i) {
		buffer.push_back(rand() % 100);
	}
}

void prod_even() {
	while (true) {
		std::unique_lock<std::mutex> locker(m);
		prod_even_cv.wait(locker, []() {return can_produce_even(); });
		// produce random even integer
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> distribution(0, 100);
		int product = (2 * distribution(gen)) % 100;
		// add product to the buffer
		buffer.push_back(product);
		// print info
		std::cout << "Even producent id: " << std::this_thread::get_id() << "; produced: " << product << std::endl;
		locker.unlock();
		if (can_produce_odd())
			prod_odd_cv.notify_all();
		else if (can_consume_even())
			cons_even_cv.notify_all();
		else if (can_consume_odd())
			cons_odd_cv.notify_all();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME + (rand() % 500)));
	}
}

void prod_odd() {
	while (true) {
		std::unique_lock<std::mutex> locker(m);
		prod_odd_cv.wait(locker, []() {return can_produce_odd(); });
		// produce random odd integer
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> distribution(0, 100);
		int product = (2 * distribution(gen) + 1) % 100;
		// add product to the buffer
		buffer.push_back(product);
		// print info
		std::cout << "Odd producent id: " << std::this_thread::get_id() << "; produced: " << product << std::endl;
		locker.unlock();
		if (can_produce_even())
			prod_even_cv.notify_all();
		else if (can_consume_odd())
			cons_odd_cv.notify_all();
		else if (can_consume_even())
			cons_even_cv.notify_all();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME + (rand() % 500)));
	}
}

void cons_even() {
	while (true) {
		std::unique_lock<std::mutex> locker(m);
		cons_even_cv.wait(locker, []() {return can_consume_even(); });
		// consume even integer
		int eaten = buffer.get_head();
		buffer.pop_front();
		// print info
		std::cout << "Even consumer id: " << std::this_thread::get_id() << "; consumed: " << eaten << std::endl;
		locker.unlock();
		if (can_consume_odd())
			cons_odd_cv.notify_all();
		else if (can_produce_even())
			prod_even_cv.notify_all();
		else if (can_produce_odd())
			prod_odd_cv.notify_all();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME + (rand() % 500)));
	}
}

void cons_odd() {
	while (true) {
		std::unique_lock<std::mutex> locker(m);
		cons_odd_cv.wait(locker, []() {return can_consume_odd(); });
		// consume even integer
		int eaten = buffer.get_head();
		buffer.pop_front();
		// print info
		std::cout << "Odd consumer id: " << std::this_thread::get_id() << "; consumed: " << eaten << std::endl;
		locker.unlock();
		if (can_consume_even())
			cons_even_cv.notify_all();
		else if (can_produce_odd())
			prod_odd_cv.notify_all();
		else if (can_produce_even())
			prod_even_cv.notify_all();
		else
			std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME + (rand() % 500)));
	}
}

/*
	option:
	- 0 -> 2 producents, 2 consumers
	- 1 -> 2 producents
	- 2 -> 2 consumers with empty buffer
	- 3 -> 2 consumers with filled buffer
	- 4 -> multiple producents and customres
*/
#define OPTION 4

#if OPTION == 0
int main() {
	std::thread A1(prod_even);
	std::thread A2(prod_odd);
	std::thread B1(cons_even);
	std::thread B2(cons_odd);

	A1.join();
	A2.join();
	B1.join();
	B2.join();
}
#endif
#if OPTION == 1
int main() {
	std::thread A1(prod_even);
	std::thread A2(prod_odd);

	A1.join();
	A2.join();
}
#endif
#if OPTION == 2
int main() {
	std::thread B1(cons_even);
	std::thread B2(cons_odd);

	B1.join();
	B2.join();
}
#endif
#if OPTION == 3
int main() {
	fill_buffer();
	buffer.print();
	std::thread B1(cons_even);
	std::thread B2(cons_odd);

	B1.join();
	B2.join();
}
#endif
#if OPTION == 4
int main() {
	std::thread A1(prod_even);
	std::thread A1a(prod_even);
	std::thread A1b(prod_even);
	std::thread A2(prod_odd);
	std::thread A2a(prod_odd);
	std::thread A2b(prod_odd);
	std::thread B1(cons_even);
	std::thread B1a(cons_even);
	std::thread B1b(cons_even);
	std::thread B2(cons_odd);
	std::thread B2a(cons_odd);
	std::thread B2b(cons_odd);

	A1.join();
	A1a.join();
	A1b.join();
	A2.join();
	A2a.join();
	A2b.join();
	B1.join();
	B1a.join();
	B1b.join();
	B2.join();
	B2a.join();
	B2b.join();
}
#endif

#endif