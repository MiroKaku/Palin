#include "Main.App.h"
#include "Main.Window.h"


namespace Mi::Palin
{
    extern"C" int WINAPI wWinMain(
        _In_     HINSTANCE  /*Instance*/,
        _In_opt_ HINSTANCE  /*PrevInstance*/,
        _In_     LPWSTR     /*CmdLine*/,
        _In_     int        /*ShowCmd*/
    )
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);

        auto App    = std::make_shared<Palin::App>();
        auto Window = MainWindow(App);

        try {
            Window.Create (1280, 720);
            Window.RunLoop();
        }
        catch (const winrt::hresult_error& Result) {
            return Result.code();
        }

        App->Close();

        return S_OK;
    }

}
