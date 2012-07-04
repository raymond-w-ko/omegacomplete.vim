#pragma once

class Teleprompter
{
public:
    static void GlobalInit();
    static Teleprompter* Instance() { return instance_; }
    ~Teleprompter();

    void Clear();
    void SetCurrentWord(const std::string& word);
    void AppendText(const std::vector<StringPair>& text_list);
    void AppendText(const std::vector<std::string>& text_list);
    void AppendText(const std::string& text);
    void Redraw();
    void Show(bool flag);

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

    const unsigned width_;
    const unsigned height_;
    HINSTANCE hinstance_;
    HWND hwnd_;

    std::string word_;
    std::vector<std::string> text_list_;
};
