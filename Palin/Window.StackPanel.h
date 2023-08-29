#pragma once


namespace Mi::Window
{
    enum class ControlType
    {
        Label = 1,
        Edit,
        ComboBox,
        Button,
        Radio,
        CheckBox,
        ThreeState,
    };

    DWORD   GetControlStyle    (_In_ ControlType Control);
    LPCWSTR GetControlClassName(_In_ ControlType Control);

    ControlType GetControlType(_In_ HWND Control);

    inline HGDIOBJ SetBackgroundColorToWindowColor(HDC StaticColorHdc, COLORREF Color)
    {
        SetBkColor(StaticColorHdc, Color);
        SetDCBrushColor(StaticColorHdc, Color);
        return GetStockObject(DC_BRUSH);
    }

    inline LRESULT StaticControlColorMessageHandler(COLORREF Color, WPARAM WParam, LPARAM)
    {
        return reinterpret_cast<LRESULT>(SetBackgroundColorToWindowColor(reinterpret_cast<HDC>(WParam), Color));
    }

    class StackPanel
    {
        struct Stepper
        {
            int Value      = 0;
            int StepAmount = 0;

            int Step()
            {
                return Step(StepAmount);
            }

            int Step(const int Amount)
            {
                const auto OldValue = Value;
                Value += Amount;
                return OldValue;
            }
        };

        HWND        mParentWindow = nullptr;
        HINSTANCE   mInstance     = nullptr;
        DWORD       mCommonStyle  = WS_CHILD | WS_OVERLAPPED | WS_VISIBLE;
        int         mMarginX      = 12;
        int         mWidth        = 200;
        int         mHeight       = 30;
        Stepper     mOffsetY{};

    public:
        StackPanel(
            _In_ HWND       ParentWindow,
            _In_ HINSTANCE  Instance,
            _In_ int        MarginX,
            _In_ int        MarginY,
            _In_ int        StepAmount,
            _In_ int        Width,
            _In_ int        Height);

        HWND CreateControl(
            _In_ ControlType Control,
            _In_ LPCTSTR     WindowName,
            _In_opt_ DWORD   Style   = 0,
            _In_opt_ int     OffsetX = -1,
            _In_opt_ int     OffsetY = -1,
            _In_     int     Width   = -1,
            _In_     int     Height  = -1);

    private:
        HWND CreateControlWindow(
            _In_ LPCWSTR ClassName,
            _In_ LPCTSTR WindowName,
            _In_ DWORD   Style,
            _In_ int     OffsetX,
            _In_ int     OffsetY,
            _In_ int     Width,
            _In_ int     Height) const;

        int SetOffsetY(_In_ ControlType Control, _In_ int Offset);
    };

}
