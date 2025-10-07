//#define _CRTDBG_MAP_ALLOC
//#include <crtdbg.h>
//#include <stdlib.h>


#include <DXE.h>
#include <Application.h>
#include <EntryPoint.h>
#include <Layer.h>
#include <Renderer/Renderer.h>
#include <Renderer/RenderManager.h>
#include <Renderer/ShaderManager.h>
#include <InputManager.h>


#include "AppLayer.h"



class SpriteGen : public DXE::Application
{
public:
    using DXE::Application::Application;


    void Initialise() {
        DXE_INFO("Initialise App");
        SetWindowTextW(hwnd, L"SpriteGen");
        DXE::Renderer::Get()->InitState(hwnd);
        DXE::Layers::AddLayer(new AppLayer("MainLayer"));

    }
    void Shutdown() {
        //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    }

    LRESULT CALLBACK Application::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        LRESULT result = 0;

        AppLayer* app = dynamic_cast<AppLayer*> (DXE::Layers::GetLayer("MainLayer"));
        if (app && app->HandleInput(hwnd, msg, wParam, lParam)) { return result; }

        switch (msg) {
        case WM_CREATE: { DXE_INFO("WM_CREATE"); break; }
        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT: { running = false; break; }
        case WM_KEYDOWN: { DXE::Input::OnKeyDown(wParam, lParam); break; }
        case WM_KEYUP: { DXE::Input::OnKeyUp(wParam, lParam); break; }

        case WM_MOUSEMOVE: { DXE::Input::OnMouseMove({ LOWORD(lParam), HIWORD(lParam) }); break; }

        case WM_LBUTTONDOWN: { DXE::Input::OnMouseButtonDown(DXE::LeftButton); break; }
        case WM_RBUTTONDOWN: { DXE::Input::OnMouseButtonDown(DXE::RightButton); break; }
        case WM_MBUTTONDOWN: { DXE::Input::OnMouseButtonDown(DXE::MiddleButton); break; }
        case WM_XBUTTONDOWN: { DXE::Input::OnMouseButtonDown((GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? DXE::XButton1 : DXE::XButton2); break; }

        case WM_LBUTTONUP: { DXE::Input::OnMouseButtonUp(DXE::LeftButton); break; }
        case WM_RBUTTONUP: { DXE::Input::OnMouseButtonUp(DXE::RightButton); break; }
        case WM_MBUTTONUP: { DXE::Input::OnMouseButtonUp(DXE::MiddleButton); break; }
        case WM_XBUTTONUP: { DXE::Input::OnMouseButtonUp((GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? DXE::XButton1 : DXE::XButton2); break; }

        case WM_MOUSEWHEEL: { DXE::Input::OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA); break; }
        case WM_MOUSEHWHEEL: { DXE::Input::OnMouseHWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA); break; }

        case WM_KILLFOCUS: { DXE::Input::IsFocused() = false; break; }
        case WM_SETFOCUS: { DXE::Input::IsFocused() = true; break; }

        // allows updating app while window is moving/resizing
        case WM_SIZE: {
            resizing = true;
            InvalidateRect(hwnd, NULL, FALSE);
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);

            if (DXE::Renderer::Device()) {
                DXE::Renderer::Get()->ResizeBuffers(w, h);
                DXE::Input::ForceRelease();
                DXE::Application::RunLoop();
                resized = true;
            }

            return 0;
        }

        case WM_SIZING:
            resized = true;
            resizing = false;
            DXE::Input::ForceRelease();
            break;
        case WM_ENTERSIZEMOVE:
            resizing = true;
            DXE::Input::ForceRelease();
            break;
        case WM_EXITSIZEMOVE:
            resizing = false;
            DXE::Input::ForceRelease();
            break;
        default: { result = DefWindowProcW(hwnd, msg, wParam, lParam); break; }
        }

        return result;

    }

    void Update(float dt) {
        if (resized) {
            RECT clientRect;
            GetClientRect(DXE::Renderer::Hwnd(), &clientRect);
            windowWidth = clientRect.right - clientRect.left;
            windowHeight = clientRect.bottom - clientRect.top;
            windowAspectRatio = (float)windowWidth / (float)windowHeight;
        }

        FLOAT backgroundColor[4] = { 0.1f, 0.2f, 0.6f, 1.0f };
        auto rtv = DXE::Renderer::FrameBufferView();
        DXE::Renderer::Context()->ClearRenderTargetView(rtv, backgroundColor);
        DXE::Renderer::Context()->ClearDepthStencilView(DXE::Renderer::DepthBufferView(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        auto width = DXE::Application::Get()->windowWidth;
        auto height = DXE::Application::Get()->windowHeight;
        D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (FLOAT)width, (FLOAT)height, 0.0f, 1.0f };
        DXE::Renderer::Context()->RSSetViewports(1, &viewport);
        DXE::Renderer::Context()->OMSetRenderTargets(1, &rtv, DXE::Renderer::DepthBufferView());
    };
    void Render(float dt) {

        DXE::Renderer::Get()->SwapChain()->Present(1, 0);
        DXE::Application::Get()->resized = false;
    };
};

DXE::Application* DXE::CreateApplication(HINSTANCE hInstance, LPSTR lpCmdLine, int nShowCmd) {
    return new SpriteGen(hInstance, lpCmdLine, nShowCmd);
}