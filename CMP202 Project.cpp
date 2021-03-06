/*
	This project measures the performance of a threaded brute force string search algorithm in a DNA alphabet sequence.
	It varies the number of threads used and iterations, after which the results are stored.

	The text file has a bit over 20 million characters. The algortihm stores the index of all pattern matches into a vector.
	
	The Benchmark function manages all of the threads, measures the time performance, and stores it into a .cvs file.
	
	The text is split up into chunks, proportionate to the number of threads. Then, each thread runs the brute force algorithm.

	A separate thread waits, with a condition variable, for all the search threads to finish. After that it prints out the contents 
	of the vector i.e. where a match has occured, it also summarises the total match numbers, how long it took, and the number of threads used.

	In the main function, the pattern and the number of threads to use are specified and the benchmark function is called.
*/
#include <iostream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <fstream>
#include <string>
#include <vector>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <sstream>

using std::cout;
using std::thread;
using std::mutex;
using std::string;
using std::condition_variable;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::ifstream;
using std::ofstream;
using std::istreambuf_iterator;

typedef std::chrono::steady_clock the_clock;

std::vector<int> matchedIndex; // stores indexes of all pattern matches
mutex vector_mutex;
mutex print_mutex;
condition_variable print_cv;
std::atomic<bool> wakeUp; // used with print_cv

void bruteForce(string keyword, string* text, int start, int end)
{
	int keywordLen = (int)keyword.length();
	int textLen = end;
	int i; // text iterator
	int j; // pattern iterator

	for (i = start, j = 0; i < textLen && j < keywordLen; i++)
	{
		if (keyword[j] == text->at(i)) {
			j++;
		}
		
		else // no match
		{
			i -= j;
			j = 0;
		}
		// pattern found
		if (j == keywordLen) {
			// protecting access and storing where the match was found
			vector_mutex.lock();
			matchedIndex.push_back(i - (j - 1));
			vector_mutex.unlock();
			
			// resetting the pattern iterator
			j = 0;
		}
	}
}

void LoadFile(string &text)
{
	std::ifstream in("sequence20m.txt", std::ios::in | std::ios::binary);
	if (in)
	{
		in.seekg(0, std::ios::end);
		text.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&text[0], text.size());
		in.close();
	}

	else
	{
		cout << "\nUnable to load text file! \nPlease make sure sequence20m.txt is in the same folder you're executing the code from!\n\n";
	}
}

void Benchmark(int numOfThreads, string pattern, string &text, std::stringstream &fileToWrite)
{
	wakeUp = false; // we want the printing thread below to wait until the string search has finished

	thread displayResult([&](std::atomic<bool>* wakeUp)
	{
		std::unique_lock<std::mutex> lock(print_mutex);

		while (!wakeUp->load())
			print_cv.wait(lock);

		if (matchedIndex.empty()) {
			cout << "No matches found";
		}

		else
		{
			// sorting the indexes in ascending order
			matchedIndex.erase(unique(matchedIndex.begin(), matchedIndex.end()), matchedIndex.end());
			std::sort(matchedIndex.begin(), matchedIndex.end());

			for (auto& match : matchedIndex)
			{
				cout << "\nFound match at index: " << match;
			}

			cout << "\n\nFound " << matchedIndex.size() << " matches";
		}
	}, &wakeUp);

	// splitting the text into separate chunks for each thread
	int textPerChunk = (int)std::floorf((float)text.size() / (float)numOfThreads); // minimum num of chars per thread
	int threadsWithMoreText = text.size() % numOfThreads; // num of threads that will need to get + 1 char

	std::vector<thread> threads;

	int startChunk = 0;
	int endOfChunk = textPerChunk;

	int rightOverlap = (int)pattern.size() - 1;

	bool addThreadsWithMoreText = false;

	the_clock::time_point start = the_clock::now();

	for (int i = 0; i < numOfThreads; i++)
	{
		if (i >= threadsWithMoreText && threadsWithMoreText != 0)
			addThreadsWithMoreText = true;

		/* This adds a range overlap if the pattern is split across different text chunks. */

		if (i == numOfThreads - 1) // the last thread shouldn't go over the end of the string array
			rightOverlap = 0;

		else // otherwise we can overlap
			rightOverlap = (int)pattern.size() - 1;

		// creating the string matching threads
		if (addThreadsWithMoreText)
			threads.push_back(thread(bruteForce, pattern, &text, startChunk, endOfChunk + rightOverlap + 1)); // adding 1 more char to endOfChunk

		else
			threads.push_back(thread(bruteForce, pattern, &text, startChunk, endOfChunk + rightOverlap));

		// switching to next chunk indexes
		startChunk = endOfChunk;
		endOfChunk += textPerChunk;
	}

	for (auto& thread : threads)
	{
		thread.join();
	}

	the_clock::time_point end = the_clock::now();
	auto time_taken = duration_cast<milliseconds>(end - start).count();
	
	fileToWrite << numOfThreads << "," << time_taken << "," << std::endl;
	
	// after the string matching threads are done, we wake up the printer thread
	wakeUp.store(true);
	print_cv.notify_one();

	displayResult.join();

	matchedIndex.clear();
	cout << "\nTime taken: " << time_taken << "ms using " << numOfThreads << " thread(s)\n";
}

int main()
{
	// the number of threads is doubled each time until it reaches the maxThreads
	int startThreads = 1; // initial threads
	int maxThreads = 128; // number of threads to build up to
	int iterations = 1; // how many times to benchmark, originally 100

	string pattern = "tgttaaatt";
	string text; 
	LoadFile(text);

	std::stringstream fileToWrite;
	fileToWrite << "threads, time (ms)" << std::endl;
	
	for (int threadNum = startThreads; threadNum <= maxThreads; threadNum = threadNum * 2)
	{
		for (int j = 0; j < iterations; j++)
		{
			Benchmark(threadNum, pattern, text, fileToWrite);
		}
	}

	ofstream file("benchmark.csv");
	file << fileToWrite.str();
	file.close();

	system("pause"); // prevents cmd window from instantly closing after execution
	return 0;
}
