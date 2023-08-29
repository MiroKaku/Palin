#pragma once
#include <mutex>


namespace Mi::Core
{
    class Window
    {
        HWND mWindow = nullptr;

    public:
        explicit Window(HWND Handle) noexcept
            : mWindow(Handle) { }

        explicit operator HWND() const noexcept { return mWindow; }

        std::wstring GetTitleName() const;
        std::wstring GetClassName() const;

        [[nodiscard]] bool IsShellWindow  () const;
        [[nodiscard]] bool IsToolWindow   () const;
        [[nodiscard]] bool IsVisible      () const;
        [[nodiscard]] bool IsDisabled     () const;
        [[nodiscard]] bool IsCloaked      () const;
    };

    class WindowList
    {
        std::mutex mMutex;
        std::unordered_set<HWND> mWindows;
        std::unordered_set<HWND> mComboBoxes;

        HWINEVENTHOOK mWinEventHook = nullptr;

    public:
        ~WindowList();

        WindowList();
        WindowList(      WindowList&&) = delete;
        WindowList(const WindowList& ) = delete;
        WindowList& operator=(      WindowList&&) = delete;
        WindowList& operator=(const WindowList& ) = delete;

        void Update();

        void RegisterComboBoxForUpdates(_In_ HWND ComboBox);
        void UnRegisterComboBox(_In_ HWND ComboBox);

    private:
        void AddWindow   (_In_ HWND Window);
        bool RemoveWindow(_In_ HWND Window);
        void ForceUpdateComboBox(_In_ HWND ComboBox);
    };
}
