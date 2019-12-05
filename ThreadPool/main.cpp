#include <iostream>

#include "ThreadPool.h"



int main()
{
	ThreadPool pool(2);
	pool.enqueue([] {std::cout << "hello world :" << std::this_thread::get_id() << endl; });
	pool.enqueue([] {std::cout << "hello world :" << std::this_thread::get_id() << endl; });
	pool.stop();
	return 0;
}