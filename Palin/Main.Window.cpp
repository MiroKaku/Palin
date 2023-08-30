#include "Main.Window.h"


namespace Mi::Palin
{
    inline winrt::fire_and_forget ShutdownAndThenPostQuitMessage(
        _In_ winrt::Windows::System::DispatcherQueueController const& Controller,
        _In_ int ExitCode
    )
    {
        const auto Queue = Controller.DispatcherQueue();

        co_await Controller.ShutdownQueueAsync();
        co_await winrt::resume_foreground(Queue);
        PostQuitMessage(ExitCode);
        co_return;
    }

    MainWindow::~MainWindow()
    {
        Destroy();
    }

    MainWindow::MainWindow(const std::shared_ptr<App>& App)
        : mApp(App)
    {
    }

    void MainWindow::Create(int Width, int Height)
    {
        /* Dispatcher queue */
        mDispatcherQueueController = CreateDispatcherQueueController(
            DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE);

        CreateMainWindow(CLASS_NAME, TITLE_NAME, CW_USEDEFAULT, 0, Width, Height, SW_SHOW);
        CreateControls();

        /* Compositor */
        mCompositor = winrt::Windows::UI::Composition::Compositor();

        mRoot       = mCompositor.CreateContainerVisual();
        mRoot.RelativeSizeAdjustment({ 1.0f, 1.0f });
        mRoot.Size  ({ -224.0f, 0.0f });
        mRoot.Offset({  224.0f, 0.0f, 0.0f });

        mTarget     = CreateDesktopWindowTarget(mCompositor, mMainWindow, true);
        mTarget.Root(mRoot);

        mTopVisual  = mCompositor.CreateContainerVisual();
        mContent    = mCompositor.CreateSpriteVisual();
        mBrush      = mCompositor.CreateSurfaceBrush();

        mTopVisual.RelativeSizeAdjustment({ 1, 1 });
        mRoot.Children().InsertAtTop(mTopVisual);

        mContent.AnchorPoint({ 0.5f, 0.5f });
        mContent.RelativeOffsetAdjustment({ 0.5f, 0.5f, 0 });
        mContent.RelativeSizeAdjustment({ 1, 1 });
        mContent.Size({ -80, -80 });
        mContent.Brush(mBrush);

        mBrush.HorizontalAlignmentRatio(0.5f);
        mBrush.VerticalAlignmentRatio(0.5f);
        mBrush.Stretch(winrt::Windows::UI::Composition::CompositionStretch::Uniform);

        const auto Shadow = mCompositor.CreateDropShadow();
        Shadow.Mask(mBrush);
        mContent.Shadow(Shadow);

        mTopVisual.Children().InsertAtTop(mContent);
    }

    void MainWindow::Destroy()
    {
        if (mApp) {
            mApp->RegisterClosedRevoke(nullptr);
        }

        if (mCboWindows) {
            DestroyWindow(mCboWindows);
        }
        if (mTxtSharedName) {
            DestroyWindow(mTxtSharedName);
        }
        if (mTxtSharedHandle) {
            DestroyWindow(mTxtSharedHandle);
        }
        if (mBtnSwitch) {
            DestroyWindow(mBtnSwitch);
        }
        if (mCboRotationMode) {
            DestroyWindow(mCboRotationMode);
        }
        if (mChkNtHandle) {
            DestroyWindow(mChkNtHandle);
        }
        if (mChkKeyedMutex) {
            DestroyWindow(mChkKeyedMutex);
        }
        if (mTxtAcquireKey) {
            DestroyWindow(mTxtAcquireKey);
        }
        if (mTxtReleaseKey) {
            DestroyWindow(mTxtReleaseKey);
        }
        if (mTxtTimeout) {
            DestroyWindow(mTxtTimeout);
        }

        mCboWindows      = nullptr;
        mTxtSharedName   = nullptr;
        mTxtSharedHandle = nullptr;
        mBtnSwitch       = nullptr;
        mCboRotationMode = nullptr;
        mChkNtHandle     = nullptr;
        mChkKeyedMutex   = nullptr;
        mTxtAcquireKey   = nullptr;
        mTxtReleaseKey   = nullptr;
        mTxtTimeout      = nullptr;

        mBrush           = nullptr;
        mContent         = nullptr;
        mTopVisual       = nullptr;
        mTarget          = nullptr;
        mRoot            = nullptr;
        mCompositor      = nullptr;
        mDispatcherQueueController = nullptr;
    }
    
    void MainWindow::CreateControls()
    {
        constexpr auto MarginX    = 12;
        constexpr auto MarginY    = 40;
        constexpr auto Width      = 224;
        constexpr auto Height     = 24;
        constexpr auto StepAmount = 36;

        auto Controls = Window::StackPanel(mMainWindow, HINST_THISCOMPONENT, MarginX, MarginY, StepAmount, Width, Height);

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Windows:"));
        mCboWindows = winrt::check_pointer(Controls.CreateControl(Window::ControlType::ComboBox, L""));

        // Populate window combo box and register for updates
        mWindowList = std::make_unique<Core::WindowList>();
        winrt::check_pointer(mWindowList.get());
        mWindowList->RegisterComboBoxForUpdates(mCboWindows);

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Shared Name:"));
        mTxtSharedName = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Edit, L""));

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Shared Handle (Hex):"));
        mTxtSharedHandle = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Edit, L""));

        mBtnSwitch = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Button, L"Start", 0,
            -1, -1, -1, 48));
        
        mApp->RegisterClosedRevoke([this]
        {
            if (mStarted == true) {
                ComboBox_SetCurSel(mCboWindows, -1);
                PostMessage(mMainWindow, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), reinterpret_cast<LPARAM>(mBtnSwitch));
            }
        });

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Rotation Mode:"));
        mCboRotationMode = winrt::check_pointer(Controls.CreateControl(Window::ControlType::ComboBox, L""));

        static const std::map<DXGI_MODE_ROTATION, const wchar_t* const> RotationModes = {
            { DXGI_MODE_ROTATION_UNSPECIFIED, L"Unspecified      " },
            { DXGI_MODE_ROTATION_IDENTITY   , L"Rotation Identity" },
            { DXGI_MODE_ROTATION_ROTATE90   , L"Rotation 90      " },
            { DXGI_MODE_ROTATION_ROTATE180  , L"Rotation 180     " },
            { DXGI_MODE_ROTATION_ROTATE270  , L"Rotation 270     " },
        };

        for (auto& [Mode, Name] : RotationModes) {
            if (const auto Index = ComboBox_AddString(mCboRotationMode, Name); Index >= 0) {
                ComboBox_SetItemData(mCboRotationMode, Index, Mode);
            }
        }

        ComboBox_SetCurSel(mCboRotationMode, 1);

        mChkNtHandle   = winrt::check_pointer(Controls.CreateControl(Window::ControlType::CheckBox, L"NT Handle",
            0, -1, -1, Width / 2 - MarginX - 10));
        mChkKeyedMutex = winrt::check_pointer(Controls.CreateControl(Window::ControlType::CheckBox, L"Keyed Mutex",
            0, Width / 2, -StepAmount, Width / 2 - 4));

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Acquire Key:"));
        mTxtAcquireKey = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Edit, L"1", WS_DISABLED));

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Release Key:"));
        mTxtReleaseKey = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Edit, L"0", WS_DISABLED));

        winrt::check_pointer(Controls.CreateControl(Window::ControlType::Label, L"Timeout:"));
        mTxtTimeout = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Edit, L"-1", WS_DISABLED));
    }

    LRESULT MainWindow::MessageHandler(
        _In_ UINT   Message,
        _In_ WPARAM WParam,
        _In_ LPARAM LParam) noexcept
    {
        if (WM_DESTROY == Message) {
            mDispatcherQueueController.DispatcherQueue().TryEnqueue([this]
            {
                mApp->StopPlay();
            });

            ShutdownAndThenPostQuitMessage(mDispatcherQueueController, 0);
            return 0;
        }

        if (Message == WM_COMMAND) {
            if (const auto Sender = GET_WM_COMMAND_HWND(WParam, LParam)) {
                return CommandHandler(Sender,
                    GET_WM_COMMAND_CMD(WParam, LParam), GET_WM_COMMAND_ID(WParam, LParam));
            }
        }

        if (Message == WM_CTLCOLORSTATIC) {
            return Window::StaticControlColorMessageHandler(RGB(0xEF, 0xE4, 0xB0), WParam, LParam);
        }

        return DesktopWindow::MessageHandler(Message, WParam, LParam);
    }

    LRESULT MainWindow::CommandHandler(HWND Sender, int Command, int /*Id*/)
    {
        LRESULT Result = 0;

        switch (Window::GetControlType(Sender)) {
            default:
            {
                break;
            }
            case Window::ControlType::ComboBox:
            {
                if (Command == CBN_DROPDOWN) {
                    Result = ComboBox_Dropdown(Sender);
                }
                break;
            }
            case Window::ControlType::Button:
            {
                if (Command == BN_CLICKED) {
                    Result = Button_Clicked(Sender);
                }
                break;
            }
            case Window::ControlType::CheckBox:
            {
                if (Command == BN_CLICKED) {
                    Result = CheckBox_Clicked(Sender);
                }
                break;
            }
            case Window::ControlType::Edit:
            {
                if (Command == EN_CHANGE) {
                    Result = Edit_Changed(Sender);
                }
                break;
            }
        }

        return Result;
    }

    LRESULT MainWindow::Edit_Changed(HWND Sender)
    {
        if (Sender == mTxtSharedName) {
            if (Edit_GetTextLength(Sender) > 0) {
                Edit_Enable(mTxtSharedHandle, FALSE);
            }
            else {
                Edit_Enable(mTxtSharedHandle, TRUE);
            }
        }
        if (Sender == mTxtSharedHandle) {
            if (Edit_GetTextLength(Sender) > 0) {
                Edit_Enable(mTxtSharedName, FALSE);
            }
            else {
                Edit_Enable(mTxtSharedName, TRUE);
            }
        }

        return 0;
    }

    HRESULT WINAPI DwmGetDxSharedSurface(
        _In_ const HWND Window,
        _Out_opt_ HANDLE* DxSurface,
        _Out_opt_ LUID* AdapterLuid,
        _Out_opt_ DXGI_FORMAT* Format,
        _Out_opt_ ULONG* Flags,
        _Out_opt_ UINT64* UpdateId
    )
    {
        static decltype(DwmGetDxSharedSurface)* DwmGetDxSharedSurface_ = nullptr;

        if (DwmGetDxSharedSurface_ == nullptr) {

            const auto User32 = GetModuleHandleW(L"user32");
            DwmGetDxSharedSurface_ = reinterpret_cast<decltype(DwmGetDxSharedSurface_)>(
                GetProcAddress(User32, "DwmGetDxSharedSurface"));

            if (DwmGetDxSharedSurface_ == nullptr) {
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        return DwmGetDxSharedSurface_(Window, DxSurface, AdapterLuid, Format, Flags, UpdateId);
    }

    LRESULT MainWindow::Button_Clicked(HWND Sender)
    {
        if (Sender != mBtnSwitch) {
            return 0;
        }

        if (mStarted) {
            if (SUCCEEDED(mApp->StopPlay())) {
                Button_SetText(Sender, L"Start");
                mBrush.Surface(nullptr);

                mStarted = false;
            }
        }
        else {
            do {
                HWND TargetWindow;
                DXGI_MODE_ROTATION  Mode;

                // Target Window
                {
                    const auto Index = ComboBox_GetCurSel(mCboWindows);
                    if (Index == -1) {
                        MessageBox(mMainWindow, L"Invalid: 'Windows' no selected.", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }
                    TargetWindow = reinterpret_cast<HWND>(ComboBox_GetItemData(
                        mCboWindows, Index));
                    if (!IsWindow(TargetWindow)) {
                        MessageBox(mMainWindow, L"Invalid: Target Window.", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }
                }

                // Rotation Mode
                {
                    const auto Index = ComboBox_GetCurSel(mCboRotationMode);
                    if (Index == -1) {
                        MessageBox(mMainWindow, L"Invalid: 'RotationMode' no selected.", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }
                    Mode = static_cast<DXGI_MODE_ROTATION>(ComboBox_GetItemData(
                        mCboRotationMode, Index));
                }

                bool IsUseKeyedMutex;
                UINT32 AcquireKey   = 0;
                UINT32 ReleaseKey   = 0;
                UINT32 Timeout      = 0;

                // KeyedMutex
                {
                    IsUseKeyedMutex = Button_GetCheck(mChkKeyedMutex);
                    if (IsUseKeyedMutex) {
                        wchar_t Buffer[64]{};

                        if (Edit_GetText(mTxtAcquireKey, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Acquire Key.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        AcquireKey = std::stoul(Buffer);

                        if (Edit_GetText(mTxtReleaseKey, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Release Key.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        ReleaseKey = std::stoul(Buffer);

                        if (Edit_GetText(mTxtTimeout, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Timeout value.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        Timeout = std::stoul(Buffer);
                    }
                }

                if (IsWindowEnabled(mTxtSharedName) && (Edit_GetTextLength(mTxtSharedName) > 0)) {
                    wchar_t SharedName[256]{};
                    if (Edit_GetText(mTxtSharedName, SharedName, _countof(SharedName)) == 0) {
                        MessageBox(mMainWindow, L"Invalid: Shared Name.", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }

                    if (FAILED(mApp->StartPlayingFromSharedName(TargetWindow, SharedName, Mode,
                        IsUseKeyedMutex, AcquireKey, ReleaseKey, Timeout))) {

                        MessageBox(mMainWindow, L"Failed: Start failed. (1)", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }

                    Button_SetText(Sender, L"Stop");
                    mBrush.Surface(CreateCompositionSurfaceForSwapChain(mCompositor, mApp->GetSwapChain().get()));

                    mStarted = true;
                }
                else {
                    HANDLE SharedHandle  = nullptr;
                    auto   TargetProcess = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&::CloseHandle)>(
                        nullptr, [](HANDLE Handle) -> BOOL { if (Handle) return ::CloseHandle(Handle); return TRUE; });

                    if (Edit_GetTextLength(mTxtSharedHandle) > 0) {
                        wchar_t Buffer[64]{};
                        if (Edit_GetText(mTxtSharedHandle, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Shared Handle.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        SharedHandle = reinterpret_cast<HANDLE>(static_cast<size_t>(std::stoull(Buffer, nullptr, 16)));
                    }
                    else {
                        if (FAILED(DwmGetDxSharedSurface(TargetWindow, &SharedHandle, nullptr, nullptr, nullptr, nullptr))) {
                            MessageBox(mMainWindow, L"Failed: Get shared handle from window.", TITLE_NAME, MB_OK | MB_ICONERROR);
                        }
                    }

                    if (FAILED(mApp->StartPlayingFromSharedHandle(TargetWindow, SharedHandle, Mode, Button_GetCheck(mChkNtHandle),
                        IsUseKeyedMutex, AcquireKey, ReleaseKey, Timeout))) {

                        MessageBox(mMainWindow, L"Failed: Start failed. (2)", TITLE_NAME, MB_OK | MB_ICONERROR);
                        break;
                    }

                    Button_SetText(Sender, L"Stop");
                    mBrush.Surface(CreateCompositionSurfaceForSwapChain(mCompositor, mApp->GetSwapChain().get()));

                    mStarted = true;
                }
            } while (false);
        }

        return 0;
    }

    LRESULT MainWindow::CheckBox_Clicked(HWND Sender)
    {
        if (Sender == mChkKeyedMutex) {
            if (Button_GetCheck(Sender)) {
                Edit_Enable(mTxtAcquireKey, TRUE);
                Edit_Enable(mTxtReleaseKey, TRUE);
                Edit_Enable(mTxtTimeout, TRUE);
            }
            else {
                Edit_Enable(mTxtAcquireKey, FALSE);
                Edit_Enable(mTxtReleaseKey, FALSE);
                Edit_Enable(mTxtTimeout, FALSE);
            }
        }

        return 0;
    }

    LRESULT MainWindow::ComboBox_Dropdown(HWND Sender)
    {
        if (Sender == mCboWindows) {
            mWindowList->Update();
        }

        return 0;
    }

}
