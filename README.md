# Multithreading-C-code
Multithreading usage for implementing "client, tocken and servers managerment" (Tocken bucket system implementation).

This project implements the Tocken Bucket system implementation. 
[Lines of Code : 1200+]
- There are two server threads (can be increased to improve the performance)
- One Input packet thread
- One token thread

https://lh3.googleusercontent.com/4NkxGQWXVJK2VBVA1ahepMT0H2v4P1HC4PlTfn0Hn-HM50CAjaCi8JZpiwefFFFse_FWeg=s170


* Function parses input arguments supplied from commandline, to get values of all the options and check for errors
* The command line syntax for warmup2 is as follows : warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]
* A commandline option is a commandline argument that begins with a - character in a commandline syntax specification
* The -n option specifies the total number of packets to arrive
* If the -t option is specified, tsfile is a trace specification file that you should use to drive your emulation
* In this case, you should ignore the -lambda, -mu, -P, and -num commandline options and run your emulation in the trace-driven mode
* If the -t option is not used, you should run your emulation in the deterministic mode
* The default value (i.e., if it's not specified in a commandline option) for lambda is 1 (packets per second), the default value for mu is 0.35
  (packets per second), the default value for r is 1.5 (tokens per second), the default value for B is 10 (tokens), the default value for P is 3 
  (tokens), and the default value for num is 20 (packets). B, P, and num must be positive integers with a maximum value of 2147483647 (0x7fffffff). 
  lambda, mu, and r must be positive real numbers
*/

High level features :
- Signal management.
- Multhreading management.
- Mutex implementation.
- No deadlock & no busy waiting.

