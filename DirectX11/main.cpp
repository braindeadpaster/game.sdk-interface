#include "main.hpp"
#include "gamesdk_ui.h"
#include <cmath>
c_main gui;

int main( int, char** ) {
    ui::size = ImVec2( 239, 257 ); // game.sdk login size
    WNDCLASSEXW wc = { sizeof( wc ), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, L"DirectX11", NULL };
    ::RegisterClassExW( &wc );
    HWND hwnd = ::CreateWindowW( wc.lpszClassName, L"DirectX11 Window", WS_POPUP, GetSystemMetrics( SM_CXSCREEN ) / 2 - ui::size.x / 2, GetSystemMetrics( SM_CYSCREEN ) / 2 - ui::size.y / 2, ui::size.x, ui::size.y, NULL, NULL, wc.hInstance, NULL );

    IMGUI_CHECKVERSION( );

    CreateDeviceD3D( hwnd );
    {
        ::c_context context( hwnd );

        ImGuiIO& io = ImGui::GetIO( ); io.IniFilename = nullptr;
        gui.initialize( io );

        ::ShowWindow( hwnd, SW_SHOWDEFAULT );
        ::UpdateWindow( hwnd );

        bool done = false;
        while ( !done ) {
            MSG msg;
            while ( ::PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) ) {
                ::TranslateMessage( &msg );
                ::DispatchMessage( &msg );
                if ( msg.message == WM_QUIT )
                    done = true;
            }
            if ( done )
                break;

            if ( g_SwapChainOccluded && g_pSwapChain->Present( 0, DXGI_PRESENT_TEST ) == DXGI_STATUS_OCCLUDED ) {
                ::Sleep( 10 );
                continue;
            }
            g_SwapChainOccluded = false;

            if ( g_ResizeWidth != 0 && g_ResizeHeight != 0 ) {
                CleanupRenderTarget( );
                g_pSwapChain->ResizeBuffers( 0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0 );
                g_ResizeWidth = g_ResizeHeight = 0;
                CreateRenderTarget( );
            }

            {
                ::c_frame frame{};

                RECT rect;
                GetWindowRect( hwnd, &rect );

                if ( GImGui->MovingWindow != NULL ) {
                    MoveWindow( hwnd, rect.left + ImGui::GetMouseDragDelta( ).x, rect.top + ImGui::GetMouseDragDelta( ).y, ui::size.x, ui::size.y, TRUE );
                }

                static GameSdkState gs;
                static int   screen = 0;   // 0 = login, 1 = loader
                static int   phase  = 0;   // 0 = idle, 1 = fading out, 2 = fading in
                static float alpha  = 1.f;
                const  float FADE   = 4.0f; // ~0.25s per half

                float dt = ImGui::GetIO( ).DeltaTime; if ( dt <= 0.f ) dt = 1.f / 60.f;

                // ----- fade transition (login -> loader) -----
                if ( phase == 1 ) {                 // fade login out
                    alpha -= FADE * dt;
                    if ( alpha <= 0.f ) {
                        alpha  = 0.f;
                        screen = 1;                 // swap to the loader
                        phase  = 2;
                        ui::size = ImVec2( 303, 236 );
                        MoveWindow( hwnd, rect.left, rect.top, (int)ui::size.x, (int)ui::size.y, TRUE );
                    }
                } else if ( phase == 2 ) {          // fade loader in
                    alpha += FADE * dt;
                    if ( alpha >= 1.f ) { alpha = 1.f; phase = 0; }
                }

                ImGui::PushFont( fonts[font].get( 13 ) ); //  Tahoma 13
                ImGui::PushStyleVar( ImGuiStyleVar_Alpha, alpha );

                bool titlebar_held = false;
                if ( screen == 0 ) {
                    DrawLogin( gs );
                    titlebar_held = gs.titlebar_held;
                    if ( phase == 0 && gs.login_ok )   // correct credentials -> start the fade
                        phase = 1;
                } else {
                    DrawLoader( gs );
                    titlebar_held = gs.titlebar_held;
                }

                ImGui::PopStyleVar( );
                ImGui::PopFont( );

                // Smooth OS-window drag (k = 14), driven by whichever form is active.
                static bool  s_dragging = false;
                static POINT s_grab = { 0, 0 };
                static float s_cx = 0.f, s_cy = 0.f;
                static bool  s_init = false;

                POINT cur; GetCursorPos( &cur );
                if ( !s_init ) { s_cx = (float)rect.left; s_cy = (float)rect.top; s_init = true; }

                if ( titlebar_held && !s_dragging ) {
                    s_dragging = true;
                    s_grab.x = cur.x - rect.left;
                    s_grab.y = cur.y - rect.top;
                    s_cx = (float)rect.left;
                    s_cy = (float)rect.top;
                }
                if ( !titlebar_held )
                    s_dragging = false;

                float goal_x = (float)rect.left, goal_y = (float)rect.top;
                if ( s_dragging ) { goal_x = (float)( cur.x - s_grab.x ); goal_y = (float)( cur.y - s_grab.y ); }

                float t = 1.f - expf( -14.f * dt );
                s_cx += ( goal_x - s_cx ) * t;
                s_cy += ( goal_y - s_cy ) * t;

                if ( s_dragging || fabsf( goal_x - s_cx ) > 0.5f || fabsf( goal_y - s_cy ) > 0.5f )
                    MoveWindow( hwnd, (int)( s_cx + 0.5f ), (int)( s_cy + 0.5f ), (int)ui::size.x, (int)ui::size.y, TRUE );
            }
        }
    }
    CleanupDeviceD3D( );

    ::DestroyWindow( hwnd );
    ::UnregisterClassW( wc.lpszClassName, wc.hInstance );
}