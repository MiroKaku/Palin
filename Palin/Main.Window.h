#pragma once
#include "Main.App.h"
#include "Core.WindowList.h"
#include "Window.StackPanel.h"
#include "Window.DesktopWindow.h"


namespace Mi::Palin
{
    class MainWindow final : public Window::DesktopWindow
    {
        static constexpr wchar_t CLASS_NAME[] = L"Mi.Palin.Class";
        static constexpr wchar_t TITLE_NAME[] = L"Palin - DirectX Shared Texture Player";

        // Controls
        HWND mCboWindows        = nullptr;
        HWND mTxtSharedName     = nullptr;
        HWND mTxtSharedHandle   = nullptr;
        HWND mBtnSwitch         = nullptr;
        HWND mCboRotationMode   = nullptr;
        HWND mChkNtHandle       = nullptr;
        HWND mChkKeyedMutex     = nullptr;
        HWND mTxtAcquireKey     = nullptr;
        HWND mTxtReleaseKey     = nullptr;
        HWND mTxtTimeout        = nullptr;
        HWND mBtnLogging        = nullptr;

        bool mStarted           = false;
        bool mLogging           = false;
        std::unique_ptr<Core::WindowList> mWindowList;

        // Compositions
        winrt::Windows::System::DispatcherQueueController               mDispatcherQueueController{ nullptr };
        winrt::Windows::UI::Composition::Compositor                     mCompositor { nullptr };
        winrt::Windows::UI::Composition::ContainerVisual                mRoot       { nullptr };
        winrt::Windows::UI::Composition::ContainerVisual                mTopVisual  { nullptr };
        winrt::Windows::UI::Composition::SpriteVisual                   mContent    { nullptr };
        winrt::Windows::UI::Composition::CompositionSurfaceBrush        mBrush      { nullptr };
        winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget   mTarget     { nullptr };

        // App
        std::shared_ptr<App> mApp;

    public:
        ~MainWindow() override;
        explicit MainWindow(const std::shared_ptr<App>& App);

        MainWindow(      MainWindow&&) = delete;
        MainWindow(const MainWindow& ) = delete;
        MainWindow& operator=(      MainWindow&&) = delete;
        MainWindow& operator=(const MainWindow& ) = delete;

        void Create (int Width, int Height);
        void Destroy();

    private:
        void CreateControls();

        LRESULT MessageHandler(
            _In_ UINT   Message,
            _In_ WPARAM WParam,
            _In_ LPARAM LParam) noexcept override;

        LRESULT CommandHandler(HWND Sender, int Command, int Id);

        // Control::Edit
        LRESULT Edit_Changed(HWND Sender);

        // Control::Button
        LRESULT Button_Clicked(HWND Sender);

        // Control::CheckBox
        LRESULT CheckBox_Clicked(HWND Sender);

        // Control::ComboBox
        LRESULT ComboBox_Dropdown(HWND Sender);
    };
}
