#include "stdafx.hpp"

#include "Teleprompter.hpp"

Teleprompter* Teleprompter::instance_ = NULL;

const char* g_szClassName = "OmegaCompleteTeleprompter";

void Teleprompter::GlobalInit()
{
    instance_ = new Teleprompter;
}

Teleprompter::Teleprompter()
:
width_(1920 / 2),
height_(1080 / 2)
{
    gui_thread_ = boost::thread(
        &Teleprompter::guiThreadEntryPoint,
        this);
}

Teleprompter::~Teleprompter()
{
    gui_thread_.join();
}

void Teleprompter::guiThreadEntryPoint()
{
    hinstance_ = ::GetModuleHandle(NULL);

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
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

    // get monitor info
    POINT ptZero = { 0 };
    HMONITOR hmonPrimary = ::MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO monitor_info = { 0 };
    monitor_info.cbSize = sizeof(monitor_info);
    ::GetMonitorInfo(hmonPrimary, &monitor_info);

    const RECT& rcWork = monitor_info.rcWork;
    POINT ptCenter;
    ptCenter.x = rcWork.left + (rcWork.right - rcWork.left - width_) / 2;
    ptCenter.y = rcWork.top + (rcWork.bottom - rcWork.top - height_) / 2;

    hwnd_ = ::CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        g_szClassName,
        "The title of my window",
        WS_POPUP,
        ptCenter.x, ptCenter.y,
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
        HFONT hFont = ::CreateFont(
            font_height,
            0,0,0,FW_BOLD,0,0,0,0,0,0,2,0,
            "Calibri");
        HFONT hTmp = (HFONT)::SelectObject(hdc, hFont);

        ::SetBkMode(
            hdc,
            TRANSPARENT);

        Teleprompter::Instance()->mutex_.lock();

        unsigned y_offset = 10;

        // draw text
        ::SetTextAlign(hdc, TA_CENTER);
        ::SetTextColor(hdc, RGB(255, 255, 255));
        const std::string& word = Teleprompter::Instance()->word_;
        ::TextOut(
            hdc,
            center_x, y_offset,
            word.c_str(), word.size());
        y_offset += font_height;

        ::SetTextAlign(hdc, TA_CENTER);
        ::SetTextColor(hdc, RGB(0, 255, 0));
        foreach (const std::string& line,
                 Teleprompter::Instance()->text_list_)
        {
            ::TextOut(
                hdc,
                center_x, y_offset,
                line.c_str(), line.size());
            y_offset += font_height;
        }

        Teleprompter::Instance()->mutex_.unlock();

        // cleanup
        ::DeleteObject(SelectObject(hdc,hTmp));

        ::EndPaint(hwnd, &ps);
        break;
        }
    case WM_ERASEBKGND:
        // overriding this eliminates the annoying flickering that happens
        // when updating the text
        return 1;
        break;
    default:
        return DefWindowProc(hwnd,msg, wParam, lParam);
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
        SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER |
        (flag ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}
