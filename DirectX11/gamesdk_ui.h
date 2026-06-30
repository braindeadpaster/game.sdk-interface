// gamesdk_ui.h
// Hand-port of the game.sdk SlurUI menu (s.d/Form1 login + main loader/Form2 loader)
// to backend-agnostic Dear ImGui. Login accepts user == password == "slurricane".
#pragma once

#include "imgui/imgui.h"

struct GameSdkState
{
    // login (Form1)
    char username[64] = "";
    char password[64] = "";
    bool remember = false;
    bool login_error = false;   // shown after a failed attempt
    bool login_ok = false;      // set true on a correct login (host transitions to loader)

    // smooth text input (per field)
    int   focused_field = 0;    // 0 none, 1 username, 2 password
    float blink = 0.f;
    int   user_caret = 0,  pass_caret = 0;
    int   user_scroll = 0, pass_scroll = 0;
    float user_caretx = 0.f, pass_caretx = 0.f;

    // animated checkbox
    float remember_anim = 0.f;

    // loader (Form2)
    int  game_tab = 0;          // 0 = Rust

    // shared
    bool titlebar_held = false;
    bool request_close = false;
};

void DrawLogin(GameSdkState& s);    // 239 x 257
void DrawLoader(GameSdkState& s);   // 303 x 236
