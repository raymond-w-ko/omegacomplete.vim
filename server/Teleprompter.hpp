#pragma once

class Teleprompter
{
public:
    static void GlobalInit();
    static Teleprompter* Instance() { return instance_; }
    ~Teleprompter();

private:
    static LRESULT CALLBACK WndProc(
        HWND hwnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam);

    Teleprompter();
    void guiThreadEntryPoint();

    static Teleprompter* instance_;

    boost::thread gui_thread_;
    boost::mutex mutex_;

    // all member variables for access in GUI thread only
};
