#include "Window.DesktopWindow.h"


namespace Mi::Window
{
    DesktopWindow::DesktopWindow(DesktopWindow&& Other) noexcept
        : mClassAtom (std::exchange(Other.mClassAtom,  {}))
        , mMainWindow(std::exchange(Other.mMainWindow, {}))
    {
    }

    DesktopWindow& DesktopWindow::operator=(DesktopWindow&& Other) noexcept
    {
        if (this != &Other) {
            mClassAtom  = std::exchange(Other.mClassAtom,  {});
            mMainWindow = std::exchange(Other.mMainWindow, {});
        }

        return *this;
    }

    DesktopWindow::~DesktopWindow()
    {
        if (mMainWindow) {
            DestroyWindow(mMainWindow);
            mMainWindow = nullptr;
        }
        if (mClassAtom) {
            UnregisterClassW(reinterpret_cast<LPCWSTR>(mClassAtom), HINST_THISCOMPONENT);
            mClassAtom  = 0;
        }
    }

    DesktopWindow* DesktopWindow::GetThisFromHandle(HWND const Window) noexcept
    {
        return static_cast<DesktopWindow*>(reinterpret_cast<void*>(GetWindowLongPtrW(Window, GWLP_USERDATA)));
    }

    LRESULT CALLBACK DesktopWindow::MessageHandler(
        _In_ HWND const Window, _In_ UINT const Message, _In_ WPARAM const WParam, _In_ LPARAM const LParam
    ) noexcept
    {
        WINRT_ASSERT(Window);

        if (WM_NCCREATE == Message){
            auto That = static_cast<DesktopWindow*>(reinterpret_cast<CREATESTRUCT*>(LParam)->lpCreateParams);
            WINRT_ASSERT(That);

            WINRT_ASSERT(!That->mMainWindow);
            That->mMainWindow = Window;

            SetWindowLongPtrW(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(That));
        }
        else if (const auto That = GetThisFromHandle(Window)) {
            return That->MessageHandler(Message, WParam, LParam);
        }

        return DefWindowProcW(Window, Message, WParam, LParam);
    }

    void DesktopWindow::RunLoop()
    {
        // Message pump
        MSG Message{};
        while (GetMessage(&Message, nullptr, 0, 0)) {
            TranslateMessage(&Message);
            DispatchMessageW(&Message);
        }
    }

    LRESULT DesktopWindow::MessageHandler(
        _In_ UINT const Message, _In_ WPARAM const WParam, _In_ LPARAM const LParam
    ) noexcept
    {
        return DefWindowProcW(mMainWindow, Message, WParam, LParam);
    }

    void DesktopWindow::CreateMainWindow(
        _In_ const LPCWSTR  ClassName,
        _In_ const LPCWSTR  TitleName,
        _In_ const INT      X,
        _In_ const INT      Y,
        _In_ const INT      Width,
        _In_ const INT      Height,
        _In_ const INT      CmdShow,
        _In_ const COLORREF Background
    )
    {
        /* Legacy window */
        WNDCLASSEXW WindowClassDesc{};
        WindowClassDesc.cbSize        = sizeof(WNDCLASSEX);
        WindowClassDesc.hIcon         = LoadIconW  (nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
        WindowClassDesc.hIconSm       = LoadIconW  (nullptr, reinterpret_cast<LPCWSTR>(IDI_APPLICATION));
        WindowClassDesc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
        WindowClassDesc.hbrBackground = CreateSolidBrush(Background);
        WindowClassDesc.hInstance     = HINST_THISCOMPONENT;
        WindowClassDesc.lpszClassName = ClassName;
        WindowClassDesc.style         = CS_HREDRAW | CS_VREDRAW;
        WindowClassDesc.lpfnWndProc   = MessageHandler;

        mClassAtom = RegisterClassExW(&WindowClassDesc);
        winrt::check_bool(!!mClassAtom);

        CreateWindowExW(
            0,
            MAKEINTRESOURCEW(mClassAtom),
            TitleName,
            WS_OVERLAPPEDWINDOW,
            X, Y,
            Width, Height,
            HWND_DESKTOP,
            nullptr,
            HINST_THISCOMPONENT,
            this);
        winrt::check_pointer(mMainWindow);

        ShowWindow  (mMainWindow, CmdShow);
        UpdateWindow(mMainWindow);
    }

}
