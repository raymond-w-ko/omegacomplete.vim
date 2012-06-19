#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

#ifdef WIN32

// from
// http://stackoverflow.com/questions/922829/c-boost-posix-time-elapsed-seconds-elapsed-fractional-seconds

//! \brief Stopwatch for timing performance values
//!
//! This stopwatch class is designed for timing performance of various
//! software operations.  If the values you get back a greater than a
//! few seconds, you should be using a different tool.
//! On a Core 2 Duo E6850 @ 3.00GHz, the start/stop sequence takes
//! approximately 230 nano seconds in the debug configuration and 180
//! nano seconds in the release configuration.  If you are timing on the
//! sub-microsecond scale, please take this into account and test it on
//! your machine.
class Stopwatch {
public:
    //! \param start if set to true will initialize and then start the
    //! timer.
	Stopwatch(bool start=false) {
        _start.QuadPart = 0;
        _stop.QuadPart = 0;
        if(start)
			Start();
    }

    //! Starts the stopwatch running
	void Start() {
		QueryPerformanceCounter(&_start);
    }

    //! Run this when the event being timed is complete
    void Stop() {
		QueryPerformanceCounter(&_stop);
    }

	//! Stops the timer and returns the result
	double StopResult() {
		Stop();
		return ResultNanoseconds();
	}

    //! You can get the result of the stopwatch start-stop sequence at
    //! your leisure.
    double ResultNanoseconds() {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double cyclesPerNanosecond = static_cast<double>(frequency.QuadPart) / 1000000000.0;

        LARGE_INTEGER elapsed;
        elapsed.QuadPart = _stop.QuadPart - _start.QuadPart;
        return elapsed.QuadPart / cyclesPerNanosecond;
    }

    void PrintResultNanoseconds() {
        std::cout << ResultNanoseconds() << "ns" << std::endl;
    }
    void PrintResultMicroseconds() {
        std::cout << ResultNanoseconds() / 100 << "us" << std::endl;
    }
    void PrintResultMilliseconds() {
        std::cout << ResultNanoseconds() / 100000 << "ms" << std::endl;
    }
    void PrintResultSeconds() {
        std::cout << ResultNanoseconds() / 1000000000 << "s" << std::endl;
    }

private:
    LARGE_INTEGER _start;
    LARGE_INTEGER _stop;
};

#endif

#endif //STOPWATCH_HPP
