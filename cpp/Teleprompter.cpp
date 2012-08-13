#include "stdafx.hpp"

#include "Teleprompter.hpp"

#ifdef TELEPROMPTER

Teleprompter* Teleprompter::instance_ = NULL;

const char* g_szClassName = "OmegaCompleteTeleprompter";

void Teleprompter::GlobalInit()
{
    instance_ = new Teleprompter;
}

Teleprompter::Teleprompter()
{
    calculateWindowDimAndLocation();

    gui_thread_ = boost::thread(
        &Teleprompter::guiThreadEntryPoint,
        this);
}

Teleprompter::~Teleprompter()
{
    gui_thread_.join();
}

void Teleprompter::calculateWindowDimAndLocation()
{
    // get monitor info
    POINT ptZero = { 0 };
    HMONITOR hmonPrimary = ::MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = { 0 };
    monitor_info.cbSize = sizeof(monitor_info);
    ::GetMonitorInfo(hmonPrimary, &monitor_info);

    monitor_width_ = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
    monitor_height_ = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
    width_ = monitor_width_;
    height_ = monitor_height_ / 2;

    //const RECT& rcWork = monitor_info.rcWork;
    //x_ = rcWork.left + (rcWork.right - rcWork.left - width_) / 2;
    //y_ = rcWork.top + (rcWork.bottom - rcWork.top - height_) / 2;
    x_ = 0;
    y_ = monitor_height_ / 2;
}

void Teleprompter::guiThreadEntryPoint()
{
    hinstance_ = ::GetModuleHandle(NULL);

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = _WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance_;
    wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!::RegisterClassEx(&wc))
    {
        ::MessageBox( 
            NULL,
            "Teleprompter Window Class Registration Failed!",
            "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    hwnd_ = ::CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        g_szClassName,
        "OmegaComplete Teleprompter",
        WS_POPUP,
        x_, y_,
        width_, height_,
        NULL,
        NULL,
        hinstance_,
        NULL);

    if (hwnd_ == NULL)
    {
        ::MessageBox(
            NULL,
            "Window Creation Failed!",
            "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    ::SetLayeredWindowAttributes(
        hwnd_,
        0,
        (255 * 33) / 100,
        LWA_ALPHA);

    ::ShowWindow(hwnd_, SW_SHOWNA);
    ::UpdateWindow(hwnd_);
    Show(false);

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0) > 0)
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return;
}

LRESULT CALLBACK Teleprompter::_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    return Teleprompter::Instance()->WndProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Teleprompter::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);
        RECT rcClient;
        ::GetClientRect(hwnd, &rcClient);
        unsigned center_x = (rcClient.right - rcClient.left) / 2;

        // paint background
        HBRUSH hBrush = ::CreateSolidBrush(RGB(0, 0, 0));
        ::FillRect(hdc, &rcClient, hBrush);
        ::DeleteObject(hBrush);

        const unsigned font_height = 80;
        const unsigned current_word_height = 24;
        HFONT current_word_font = createCurrentWordFont(current_word_height);
        HFONT completion_font = createCompletionFont(font_height);

        ::SetBkMode(
            hdc,
            TRANSPARENT);

        mutex_.lock();

        unsigned y_offset = 0;

        // draw text
        //::SetTextAlign(hdc, TA_CENTER);
        //::SetTextColor(hdc, RGB(255, 255, 255));
        //::SelectObject(hdc, current_word_font);
        //::TextOut(
            //hdc,
            //center_x, y_offset,
            //word_.c_str(), word_.size());
        //y_offset += current_word_height / 2;

        ::SetTextAlign(hdc, TA_CENTER);
        ::SetTextColor(hdc, RGB(0, 255, 0));
        ::SelectObject(hdc, completion_font);
        foreach (const std::string& line, text_list_)
        {
            ::TextOut(
                hdc,
                center_x, y_offset,
                line.c_str(), line.size());
            y_offset += font_height + 24;
        }

        mutex_.unlock();

        // cleanup
        ::DeleteObject(current_word_font);
        ::DeleteObject(completion_font);

        ::EndPaint(hwnd, &ps);
        break;
        }
    case WM_ERASEBKGND:
        // overriding this eliminates the annoying flickering that happens
        // when updating the text
        return 1;
        break;
    default:
        return ::DefWindowProc(hwnd,msg, wParam, lParam);
        break;
    }

    return 0;
}

void Teleprompter::AppendText(const std::vector<StringPair>& text_list)
{
    mutex_.lock();
    foreach (const StringPair& pair, text_list)
    {
        std::string text = pair.first;
        text += "   ";
        text += "[";
        text += pair.second;
        text += "]";
        text_list_.push_back(text);
    }
    mutex_.unlock();
}

void Teleprompter::AppendText(const std::vector<std::string>& text_list)
{
    mutex_.lock();
    foreach (const std::string& text, text_list)
    {
        text_list_.push_back(text);
    }
    mutex_.unlock();
}

void Teleprompter::AppendText(const std::string& text)
{
    mutex_.lock();
    text_list_.push_back(text);
    mutex_.unlock();
}

void Teleprompter::Redraw()
{
    ::InvalidateRect(hwnd_, NULL, TRUE);
    ::UpdateWindow(hwnd_);
}

void Teleprompter::Clear()
{
    mutex_.lock();
    text_list_.clear();
    mutex_.unlock();
}

void Teleprompter::SetCurrentWord(const std::string& word)
{
    mutex_.lock();
    word_ = word;
    mutex_.unlock();
}

void Teleprompter::Show(bool flag)
{
    ::SetWindowPos(
        hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
        (flag ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}

HFONT Teleprompter::createCurrentWordFont(unsigned height)
{
    HFONT hFont = ::CreateFont(
        height,
        0,  // average width
        0,  // escapement
        0,  // orientation
        FW_NORMAL,
        FALSE,  // italic
        FALSE,  // underline
        FALSE,  // strikeout
        ANSI_CHARSET,   // charset
        OUT_TT_PRECIS,  // output precision
        CLIP_DEFAULT_PRECIS,    // clipping precision
        CLEARTYPE_QUALITY,  // output quality
        DEFAULT_PITCH | FF_DONTCARE,    // pitch and family
        "Verdana"
        );

    return hFont;
}

HFONT Teleprompter::createCompletionFont(unsigned height)
{
    HFONT hFont = ::CreateFont(
        height,
        0,  // average width
        0,  // escapement
        0,  // orientation
        FW_SEMIBOLD,
        FALSE,  // italic
        FALSE,  // underline
        FALSE,  // strikeout
        ANSI_CHARSET,   // charset
        OUT_TT_PRECIS,  // output precision
        CLIP_DEFAULT_PRECIS,    // clipping precision
        CLEARTYPE_QUALITY,  // output quality
        DEFAULT_PITCH | FF_DONTCARE,    // pitch and family
        "Verdana"
        );

    return hFont;
}

#endif // TELEPROMPTER
