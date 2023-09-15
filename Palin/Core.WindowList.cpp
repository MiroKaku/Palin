#include "Core.WindowList.h"



namespace Mi::Core
{
    std::wstring Window::GetTitleName() const
    {
        std::array<WCHAR, 1024> WindowText;
        ::GetWindowText(mWindow, WindowText.data(), (int)WindowText.size());
        return WindowText.data();
    }

    std::wstring Window::GetClassName() const
    {
        std::array<WCHAR, 1024> ClassName;
        ::GetClassName(mWindow, ClassName.data(), (int)ClassName.size());
        return ClassName.data();
    }

    bool Window::IsShellWindow() const
    {
        return mWindow == GetShellWindow();
    }

    bool Window::IsToolWindow() const
    {
        return !!(GetWindowLongW(mWindow, GWL_EXSTYLE) & WS_EX_TOOLWINDOW);
    }

    bool Window::IsVisible() const
    {
        return IsWindowVisible(mWindow);
    }

    bool Window::IsDisabled() const
    {
        return !!(GetWindowLong(mWindow, GWL_STYLE) & WS_DISABLED);
    }

    bool Window::IsCloaked() const
    {
        DWORD Cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(mWindow, DWMWA_CLOAKED, &Cloaked, sizeof(Cloaked)))) {
            if (Cloaked == DWM_CLOAKED_SHELL) {
                return true;
            }
        }

        return false;
    }

    static bool IsAltTabWindow(Window Window)
    {
        if (Window.GetTitleName().empty()) {
            return false;
        }

        if (!Window.IsVisible()) {
            return false;
        }

        if (static_cast<HWND>(Window) != GetAncestor(static_cast<HWND>(Window), GA_ROOT)) {
            return false;
        }

        if (Window.IsToolWindow()) {
            return false;
        }

        if (Window.IsCloaked()) {
            return false;
        }

        return true;
    }

    static thread_local WindowList* WindowListForThread;

    WindowList::~WindowList()
    {
        if (mWinEventHook) {
            UnhookWinEvent(mWinEventHook);
            mWinEventHook = nullptr;
        }
    }

    WindowList::WindowList()
    {
        WINRT_ASSERT(WindowListForThread == nullptr);
        WindowListForThread = this;

        Update();

        static const auto WinEventHandler = [](HWINEVENTHOOK /*WinEventHook*/, DWORD Event, HWND Window,
            LONG /*IdObject*/, LONG IdChild, DWORD /*IdEventThread*/, DWORD /*EventTime*/)
        {
            if (Event == EVENT_OBJECT_DESTROY) {
                if (IdChild == CHILDID_SELF) {
                    WindowListForThread->RemoveWindow(Window);
                    return;
                }
            }
        };

        mWinEventHook = winrt::check_pointer(SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY,
            nullptr, WinEventHandler, 0, 0, WINEVENT_OUTOFCONTEXT));
    }

    void WindowList::Update()
    {
        EnumWindows([](HWND Window, LPARAM LParam)
        {
            if (IsAltTabWindow(Core::Window{ Window })) {
                const auto That = reinterpret_cast<WindowList*>(LParam);
                That->AddWindow(Window);
            }

            return TRUE;
        }, reinterpret_cast<LPARAM>(this));
    }

    void WindowList::RegisterComboBoxForUpdates(_In_ HWND ComboBox)
    {
        mComboBoxes.emplace(ComboBox);
        ForceUpdateComboBox(ComboBox);
    }

    void WindowList::UnRegisterComboBox(_In_ HWND ComboBox)
    {
        mComboBoxes.erase(ComboBox);
    }

    void WindowList::AddWindow(_In_ HWND Window)
    {
        auto Guard = std::unique_lock(mMutex);

        if (!mWindows.contains(Window)) {
            mWindows.emplace(Window);

            for (auto& ComboBox : mComboBoxes) {
                const auto Index = ComboBox_AddString(ComboBox, Core::Window(Window).GetTitleName().c_str());

                if (Index != CB_ERR && Index != CB_ERRSPACE) {
                    ComboBox_SetItemData(ComboBox, Index, Window);
                }
            }
        }
    }

    bool WindowList::RemoveWindow(_In_ HWND Window)
    {
        auto Guard = std::unique_lock(mMutex);

        if (mWindows.contains(Window)) {
            mWindows.erase(Window);

            for (auto& ComboBox : mComboBoxes) {

                const auto ItemCount = static_cast<uint32_t>(ComboBox_GetCount(ComboBox));
                for (uint32_t Idx = 0ul; Idx < ItemCount; ++Idx) {

                    const auto TargetWindow = reinterpret_cast<HWND>(ComboBox_GetItemData(ComboBox, Idx));
                    if (TargetWindow == Window) {
                        ComboBox_DeleteString(ComboBox, Idx);
                        break;
                    }
                }
            }
        }

        return false;
    }

    void WindowList::ForceUpdateComboBox(_In_ HWND ComboBox)
    {
        auto Guard = std::unique_lock(mMutex);

        ComboBox_ResetContent(ComboBox);

        for (auto& Window : mWindows) {
            const auto Index = ComboBox_AddString(ComboBox, Core::Window(Window).GetTitleName().c_str());

            if (Index != CB_ERR && Index != CB_ERRSPACE) {
                ComboBox_SetItemData(ComboBox, Index, Window);
            }
        }
    }
}
