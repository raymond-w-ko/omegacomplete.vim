#pragma once

class Stopwatch {
public:
	Stopwatch(bool start=false) {
        if (start)
			Start();
    }

	void Start() {
		start_ = boost::chrono::steady_clock::now();
    }

    void Stop() {
		end_ = boost::chrono::steady_clock::now();
    }

	//! Stops the timer and returns the result
	uint64_t StopResult() {
		Stop();
		return ResultNanoseconds();
	}

    //! You can get the result of the stopwatch start-stop sequence at
    //! your leisure.
    uint64_t ResultNanoseconds() {
        boost::chrono::nanoseconds ns = end_ - start_;
        return ns.count();
    }

    std::string ResultNanosecondsToString() {
        return boost::lexical_cast<std::string>(ResultNanoseconds());
    }

private:
    boost::chrono::steady_clock::time_point start_;
    boost::chrono::steady_clock::time_point end_;
};
