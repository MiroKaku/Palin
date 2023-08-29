#include "Window.StackPanel.h"


namespace Mi::Window
{
    DWORD GetControlStyle(_In_ ControlType Control)
    {
        switch (Control) {
            case ControlType::Label:
                return 0;
            case ControlType::Edit:
                return WS_TABSTOP | WS_BORDER | ES_LEFT;
            case ControlType::ComboBox:
                return WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL;
            case ControlType::Button:
                return WS_TABSTOP | BS_DEFPUSHBUTTON;
            case ControlType::Radio:
                return WS_TABSTOP | BS_AUTORADIOBUTTON;
            case ControlType::CheckBox:
                return WS_TABSTOP | BS_AUTOCHECKBOX;
            case ControlType::ThreeState:
                return WS_TABSTOP | BS_AUTO3STATE;
            default:
                return 0;
        }
    }

    LPCWSTR GetControlClassName(ControlType Control)
    {
        switch (Control) {
            default:
                return nullptr;
            case ControlType::Label:
                return WC_STATIC;
            case ControlType::Edit:
                return WC_EDIT;
            case ControlType::ComboBox:
                return WC_COMBOBOX;
            case ControlType::Button:
            case ControlType::Radio:
            case ControlType::CheckBox:
            case ControlType::ThreeState:
                return WC_BUTTON;
        }
    }

    ControlType GetControlType(_In_ HWND Control)
    {
        // The maximum length for lpszClassName is 256.
        wchar_t ClassName[256]{};

        if (GetClassName(Control, ClassName, _countof(ClassName))) {
            const std::wstring_view View(ClassName);

            if (View == WC_STATIC) {
                return ControlType::Label;
            }
            if (View == WC_EDIT) {
                return ControlType::Edit;
            }
            if (View == WC_BUTTON) {
                const auto Style = GetWindowLongPtr(Control, GWL_STYLE);

                if ((Style & BS_AUTOCHECKBOX) == BS_AUTOCHECKBOX) {
                    return ControlType::CheckBox;
                }
                if ((Style & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON) {
                    return ControlType::Radio;
                }
                if ((Style & BS_AUTO3STATE) == BS_AUTO3STATE) {
                    return ControlType::ThreeState;
                }
                return ControlType::Button;
            }
            if (View == WC_COMBOBOX) {
                return ControlType::ComboBox;
            }
        }

        return static_cast<ControlType>(0);
    }

    StackPanel::StackPanel(
        _In_ HWND       ParentWindow,
        _In_ HINSTANCE  Instance,
        _In_ int        MarginX,
        _In_ int        MarginY,
        _In_ int        StepAmount,
        _In_ int        Width,
        _In_ int        Height
    )
        : mParentWindow(ParentWindow)
        , mInstance(Instance)
        , mMarginX(MarginX)
        , mWidth(Width)
        , mHeight(Height)
        , mOffsetY{ MarginY, StepAmount }
    {
    }

    HWND StackPanel::CreateControl(
        _In_ ControlType Control,
        _In_ LPCTSTR     WindowName,
        _In_opt_ DWORD   Style,
        _In_opt_ int     OffsetX,
        _In_opt_ int     OffsetY,
        _In_opt_ int     Width,
        _In_opt_ int     Height
    )
    {
        auto StepY = 0;

        if (OffsetX == -1) {
            OffsetX = mMarginX;
        }
        else {
            OffsetX = OffsetX + mMarginX;
        }

        if (OffsetY == -1) {
            OffsetY = mOffsetY.Value;
        }
        else {
            OffsetY = OffsetY + (OffsetY < -1 ? mOffsetY.Step(OffsetY) /* back */ : mOffsetY.Value);
        }

        if (Width == -1) {
            Width  = mWidth;
        }

        if (Height == -1) {
            Height = mHeight;
        }
        else {
            StepY = Height / 2;
        }

        const auto Window = CreateControlWindow(GetControlClassName(Control), WindowName,
            Style | GetControlStyle(Control), OffsetX, OffsetY, Width, Height);
        if (Window) {
            SetOffsetY(Control, StepY);
        }

        return Window;
    }

    HWND StackPanel::CreateControlWindow(
        _In_ LPCWSTR ClassName,
        _In_ LPCTSTR WindowName,
        _In_ DWORD   Style,
        _In_ int     OffsetX,
        _In_ int     OffsetY,
        _In_ int     Width,
        _In_ int     Height
    ) const
    {
        return winrt::check_pointer(CreateWindowExW(0, ClassName, WindowName, Style | mCommonStyle,
            OffsetX, OffsetY, Width, Height,
            mParentWindow, nullptr, mInstance, nullptr));
    }

    int StackPanel::SetOffsetY(ControlType Control, _In_ int Offset)
    {
        switch (Control)
        {
            case ControlType::Label:
                return mOffsetY.Step(20 + Offset);
            case ControlType::Edit:
            case ControlType::ComboBox:
            case ControlType::Button:
            case ControlType::CheckBox:
            case ControlType::Radio:
            case ControlType::ThreeState:
                return mOffsetY.Step(mOffsetY.StepAmount + Offset);
            default:
                return 0;
        }
    }

}
