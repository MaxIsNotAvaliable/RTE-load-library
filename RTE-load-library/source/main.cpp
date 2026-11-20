#include "injector/injector.h"
#include "gui/crender.h"

#include "dnd.h"

#include <base.h>
#include <math/math.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

#include <encryption/encstring.h>
#include <encryption/sha256.h>
#include <byte_data/icons.hpp>

#include <thread>
#include <dwmapi.h>
#include <future>

#include <gdiplus.h>
#include "injector/ProcessManager.h"


HWND window = NULL;

static CRenderApp render;
static bool finished = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMessage, wParam, lParam))
        return 0L;

    if (!IsIconic(hWnd) && render.show_animation.AnimationEnded(Animation::back))
        ShowWindow(hWnd, SW_MINIMIZE);

    switch (uMessage)
    {
    case WM_SYSCOMMAND:
        render.show_animation.Start(Animation::forward);
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0L;
        break;

    case WM_DESTROY:
        PostQuitMessage(NULL);
        return 0L;

    case WM_LBUTTONDOWN:
        render.position = MAKEPOINTS(lParam);
        return 0L;

    case WM_MOUSEMOVE:
        if (wParam == MK_LBUTTON)
        {
            const POINTS points = MAKEPOINTS(lParam);
            RECT rect = RECT{};

            GetWindowRect(hWnd, &rect);

            rect.left += points.x - render.position.x;
            rect.top += points.y - render.position.y;

            if (render.position.x >= render.margin &&
                render.position.x <= render.size.x - render.margin &&
                render.position.y >= render.margin && render.position.y <= 48 + render.margin) // 48 is titlebar height
                SetWindowPos(
                    hWnd, HWND_TOPMOST,
                    rect.left, rect.top, 0, 0,
                    SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0L;

    default:
        return DefWindowProcA(hWnd, uMessage, wParam, lParam);
    }

    return DefWindowProcA(hWnd, uMessage, wParam, lParam);
}

void InitializeWindow(HINSTANCE hInstance, int nShowCmd)
{
    const char* class_name = "RTE_LOAD_LIBRARY_CLASS";
    const char* window_name = "RTE Load library";

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;

    RegisterClassExA(&wc);

    render.show_animation.SetDelay(0.5f);
    render.show_animation.Start(Animation::forward);
    render.show_animation.SetDelay(0.f);
    render.margin = 20;
    render.size = { 720 + render.margin * 2, 420 + render.margin * 2 };
    render.position = { 100, 100 };
    //CreateWindowEx(WS_EX_ACCEPTFILES, "YourWindowClass", "Drag and Drop Example", WS_OVERLAPPEDWINDOW,
    //    CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);


    window = CreateWindowExA(
        WS_EX_TRANSPARENT | WS_EX_ACCEPTFILES |
        0,
        class_name,
        window_name,
        WS_POPUP,
        render.position.x, render.position.y,
        render.size.x, render.size.y,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    SetLayeredWindowAttributes(window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    

    HRESULT hr = OleInitialize(NULL);
    if (SUCCEEDED(hr)) {
        hr = RegisterDragDrop(window, render.pDropTarget);
        int a = 10;
    }


    {
        RECT client_area{};
        GetClientRect(window, &client_area);

        RECT window_area{};
        GetWindowRect(window, &window_area);

        POINT diff{};
        ClientToScreen(window, &diff);

        const MARGINS margins{
            window_area.left + (diff.x - window_area.left),
            window_area.top + (diff.y - window_area.top),
            client_area.right,
            client_area.bottom
        };

        DwmExtendFrameIntoClientArea(window, &margins);
    }

    render.Initialize(window, wc, nShowCmd);
    finished = true;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    CoInitialize(NULL);

    auto fnAdditionalThread = [&]()
        {
            while (!finished)
            {
                CProcessManager::UpdateProcessList();

                int result;
                int id = InjectQueueHandler::Execute(result);
                if (id >= 0)
                {
                    InjectQueueHandler::SetResult(id, result);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            }
        };


    std::thread thWin(InitializeWindow, hInstance, nShowCmd);
    std::thread thExtra(fnAdditionalThread);

    thWin.join();
    thExtra.join();

    if (window != NULL)
        RevokeDragDrop(window);

    CoUninitialize();

    return EXIT_SUCCESS;
}