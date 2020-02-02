# Multithreaded Brute Force pattern matching
Brute Force pattern matching utilizing multiple CPU threads. The algorithm finds all matches in a DNA alphabet text file with 20 million characters.
![Graph of the measurements](https://raw.githubusercontent.com/deyansp/MultiThreadBruteForce/master/Images/graph.PNG)
![Svreenshot of the program running](https://raw.githubusercontent.com/deyansp/MultiThreadBruteForce/master/Images/running.JPG)
# Interpretation of results & conclusion
* The best performance occurred when using 8 threads, median time was 14ms.
* Much faster compared to using a single thread (71 ms).
* More than 8 threads yielded slightly worse performance due to the overhead of initialization and lack of other resources to run them simultaneously.
* 8 threads is the ideal choice here, since the chosen CPU can use 8 threads at a time. 
* The text is split up into even chunks, so all threads would finish at roughly the same time, therefore eliminating the need for more text chunks than available threads.
* This means that for this particular problem, the ideal number of threads is the maximum that the CPU can execute in parallel.
