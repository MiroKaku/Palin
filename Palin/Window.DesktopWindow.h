#pragma once


namespace Mi::Window
{
    class DesktopWindow
    {
    protected:
        ATOM mClassAtom  = 0;
        HWND mMainWindow = nullptr;

    public:
        virtual ~DesktopWindow();

        DesktopWindow() = default;
        DesktopWindow(      DesktopWindow&&) noexcept;
        DesktopWindow(const DesktopWindow& ) = delete;
        DesktopWindow& operator=(      DesktopWindow&&) noexcept;
        DesktopWindow& operator=(const DesktopWindow& ) = delete;

        static void RunLoop();

    protected:
        static DesktopWindow* GetThisFromHandle(_In_ HWND Window) noexcept;

        static LRESULT CALLBACK MessageHandler(
            _In_ HWND   Window,
            _In_ UINT   Message,
            _In_ WPARAM WParam,
            _In_ LPARAM LParam) noexcept;

        virtual LRESULT MessageHandler(
            _In_ UINT   Message,
            _In_ WPARAM WParam,
            _In_ LPARAM LParam) noexcept;

        virtual void CreateMainWindow(
            _In_ LPCWSTR ClassName,
            _In_ LPCWSTR TitleName,
            _In_ INT X,
            _In_ INT Y,
            _In_ INT Width,
            _In_ INT Height,
            _In_ INT CmdShow,
            _In_ COLORREF Background // GetSysColor(COLOR_WINDOW)
        );
    };

}
