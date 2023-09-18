#include "Main.Window.h"
#include "Core.Console.h"


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
        Core::RedirectIOToConsole(5000);
        if (const auto Console = GetConsoleWindow()) {
            ShowWindow(Console, SW_HIDE);
            // Title
            SetConsoleTitle(L"Mi.Palin - Logging");
            // Disable 'Close' Button
            RemoveMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, 0x0);
        }
    }

    void MainWindow::Create(int Width, int Height)
    {
        /* Dispatcher queue */
        mDispatcherQueueController = CreateDispatcherQueueController(
            DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE);

        CreateMainWindow(CLASS_NAME, TITLE_NAME, CW_USEDEFAULT, 0, Width, Height, SW_SHOW, WINDOW_BACKGROUND);
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
            mApp->RegisterClosedRevoker(nullptr);
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
        if (mBtnLogging) {
            DestroyWindow(mCboWindows);
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
        mBtnLogging      = nullptr;

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
        
        mApp->RegisterClosedRevoker([this]
        {
            if (mStarted == true) {
                mDispatcherQueueController.DispatcherQueue().TryEnqueue([this]
                {
                    ComboBox_SetCurSel(mCboWindows, -1);
                    SendMessage(mMainWindow, WM_COMMAND, MAKEWPARAM(0, BN_CLICKED), reinterpret_cast<LPARAM>(mBtnSwitch));
                });
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

        mBtnLogging = winrt::check_pointer(Controls.CreateControl(Window::ControlType::Button, L"Turn on logging", 0,
            -1, -1, -1, 48));
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
                return CommandHandler(Sender,GET_WM_COMMAND_CMD(WParam, LParam), GET_WM_COMMAND_ID(WParam, LParam));
            }
        }

        if (Message == WM_CTLCOLORSTATIC) {
            return Window::StaticControlColorMessageHandler(WINDOW_BACKGROUND, WParam, LParam);
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

    LRESULT MainWindow::Button_Clicked(HWND Sender)
    {
        if (Sender == mBtnLogging) {
            if (mLogging) {
                mLogging = false;
                Button_SetText(Sender, L"Turn on logging");

                if (const auto Console = GetConsoleWindow()) {
                    ShowWindow(Console, SW_HIDE);
                }
            }
            else {
                mLogging = true;
                Button_SetText(Sender, L"Turn off logging");

                if (const auto Console = GetConsoleWindow()) {
                    ShowWindow(Console, SW_SHOW);
                }
            }

            return 0;
        }

        if (Sender == mBtnSwitch) {
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
                        mApp->SetRotationMode(static_cast<DXGI_MODE_ROTATION>(ComboBox_GetItemData(mCboRotationMode, Index)));
                    }

                    // KeyedMutex
                    if (Button_GetCheck(mChkKeyedMutex)) {
                        wchar_t Buffer[64]{};

                        if (Edit_GetText(mTxtAcquireKey, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Acquire Key.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        const auto AcquireKey = std::stoul(Buffer);

                        if (Edit_GetText(mTxtReleaseKey, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Release Key.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        const auto ReleaseKey = std::stoul(Buffer);

                        if (Edit_GetText(mTxtTimeout, Buffer, _countof(Buffer)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Timeout value.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }
                        const auto Timeout = std::stoul(Buffer);

                        mApp->SetKeyedMutex(true, AcquireKey, ReleaseKey, Timeout);
                    }

                    if (IsWindowEnabled(mTxtSharedName) && (Edit_GetTextLength(mTxtSharedName) > 0)) {
                        wchar_t SharedName[256]{};
                        if (Edit_GetText(mTxtSharedName, SharedName, _countof(SharedName)) == 0) {
                            MessageBox(mMainWindow, L"Invalid: Shared Name.", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }

                        if (FAILED(mApp->StartPlay(TargetWindow, SharedName))) {
                            MessageBox(mMainWindow, L"Failed: Start failed. (1)", TITLE_NAME, MB_OK | MB_ICONERROR);
                            break;
                        }

                        Button_SetText(Sender, L"Stop");
                        mBrush.Surface(CreateCompositionSurfaceForSwapChain(mCompositor, mApp->GetSwapChain().get()));

                        mStarted = true;
                    }
                    else {
                        if (Edit_GetTextLength(mTxtSharedHandle) > 0) {
                            wchar_t Buffer[64]{};
                            if (Edit_GetText(mTxtSharedHandle, Buffer, _countof(Buffer)) == 0) {
                                MessageBox(mMainWindow, L"Invalid: Shared Handle.", TITLE_NAME, MB_OK | MB_ICONERROR);
                                break;
                            }

                            HANDLE SharedHandle;
                            if (Buffer[0] == L'0' && (Buffer[1] == L'x' || Buffer[1] == L'X')) {
                                SharedHandle = reinterpret_cast<HANDLE>(static_cast<size_t>(std::stoull(&Buffer[2], nullptr, 16)));
                            }
                            else {
                                SharedHandle = reinterpret_cast<HANDLE>(static_cast<size_t>(std::stoull(&Buffer[0], nullptr, 16)));
                            }

                            if (FAILED(mApp->StartPlay(TargetWindow, SharedHandle, Button_GetCheck(mChkNtHandle)))) {
                                MessageBox(mMainWindow, L"Failed: Start failed. (2)", TITLE_NAME, MB_OK | MB_ICONERROR);
                                break;
                            }
                        }
                        else {
                            if (FAILED(mApp->StartPlay(TargetWindow))) {
                                MessageBox(mMainWindow, L"Failed: Start failed. (3)", TITLE_NAME, MB_OK | MB_ICONERROR);
                                break;
                            }
                        }

                        Button_SetText(Sender, L"Stop");
                        mBrush.Surface(CreateCompositionSurfaceForSwapChain(mCompositor, mApp->GetSwapChain().get()));

                        mStarted = true;
                    }
                } while (false);
            }

            return 0;
        }

        // Other
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
