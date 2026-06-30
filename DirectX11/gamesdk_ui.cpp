// gamesdk_ui.cpp — hand-port of the game.sdk SlurUI menu to Dear ImGui.
#include "gamesdk_ui.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <string>
#include <climits>
#include <algorithm>

namespace {
    const ImU32 BG_LOGIN  = IM_COL32(14, 14, 14, 255);
    const ImU32 BG_LOADER = IM_COL32(15, 15, 15, 255);
    const ImU32 BAR       = IM_COL32(14, 14, 14, 255);
    const ImU32 PANEL     = IM_COL32(14, 14, 14, 255);
    const ImU32 BORDER    = IM_COL32(45, 45, 52, 255);
    const ImU32 ACCENT    = IM_COL32(60, 120, 220, 255);
    const ImU32 INPUT_BG  = IM_COL32(15, 15, 15, 255);
    const ImU32 INPUT_BRD = IM_COL32(30, 30, 30, 255);
    const ImU32 BTN_BG    = IM_COL32(25, 25, 25, 255);
    const ImU32 BTN_BRD   = IM_COL32(30, 30, 30, 255);
    const ImU32 DIMTEXT   = IM_COL32(50, 50, 50, 255);
    const ImU32 GREY70    = IM_COL32(70, 70, 70, 255);
    const ImU32 TYPED     = IM_COL32(175, 175, 180, 255);
    const ImU32 WHITE     = IM_COL32(236, 236, 240, 255);
    const ImU32 GREEN     = IM_COL32(70, 200, 120, 255);
    const ImU32 CB_ACCENT = IM_COL32(55, 66, 249, 255);

    float Approach(float c, float t, float k, float dt) { return c + (t - c) * (1.f - expf(-k * dt)); }

    ImU32 LerpC(ImU32 a, ImU32 b, float t)
    {
        t = t < 0 ? 0 : (t > 1 ? 1 : t);
        ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a), cb = ImGui::ColorConvertU32ToFloat4(b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(
            ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
            ca.z + (cb.z - ca.z) * t, ca.w + (cb.w - ca.w) * t));
    }

    std::string Display(const char* buf, bool password)
    {
        return password ? std::string(std::strlen(buf), '*') : std::string(buf);
    }
    int CaretPx(const std::string& d, int caret)
    {
        caret = std::max(0, std::min(caret, (int)d.size()));
        return (int)ImGui::CalcTextSize(d.substr(0, caret).c_str()).x;
    }
    void CopyStr(char* dst, int cap, const std::string& s)
    {
        int n = (int)std::min((size_t)(cap - 1), s.size());
        std::memcpy(dst, s.data(), n); dst[n] = 0;
    }

    void BeginForm(const char* id, float w, float h, ImU32 bg, GameSdkState& s, float barH)
    {
        ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bg);
        ImGuiWindowFlags f = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin(id, nullptr, f);

        ImVec2 o = ImGui::GetWindowPos();
        ImVec2 m = ImGui::GetIO().MousePos;
        bool over = m.x >= o.x && m.x <= o.x + w && m.y >= o.y && m.y <= o.y + barH;
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && over && ImGui::IsWindowHovered())
            s.titlebar_held = true;
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            s.titlebar_held = false;
    }

    void EndForm() { ImGui::End(); ImGui::PopStyleColor(1); ImGui::PopStyleVar(3); }

    void Section(ImDrawList* dl, ImVec2 o, float x, float y, float w, float h, const char* title)
    {
        auto P = [&](float a, float b) { return ImVec2(o.x + a, o.y + b); };
        dl->AddRectFilled(P(x, y), P(x + w, y + h), PANEL);
        dl->AddRect(P(x, y), P(x + w, y + h), BORDER);
        dl->AddRectFilled(P(x + 1, y + 1), P(x + w - 1, y + 3), ACCENT);
        ImVec2 ts = ImGui::CalcTextSize(title);
        float tx = x + (w - ts.x) * 0.5f, ty = y + 12;
        dl->AddLine(P(x + 10, ty + ts.y * 0.5f), P(tx - 8, ty + ts.y * 0.5f), BORDER);
        dl->AddLine(P(tx + ts.x + 8, ty + ts.y * 0.5f), P(x + w - 10, ty + ts.y * 0.5f), BORDER);
        dl->AddText(P(tx, ty), WHITE, title);
    }

    // custom single-line input with a gliding caret (smooth typing) + masking + placeholder
    void SmoothField(ImVec2 o, ImDrawList* dl, float x, float y, float w, int id,
                     char* buf, int cap, const char* ph, bool password,
                     int* caret, int* scroll, float* caretX, GameSdkState& s, bool* changed)
    {
        auto P = [&](float a, float b) { return ImVec2(o.x + a, o.y + b); };
        const float h = 32.f, pad = 8.f;
        ImGuiIO& io = ImGui::GetIO();
        float dt = io.DeltaTime > 0 ? io.DeltaTime : 1.f / 60.f;

        ImGui::SetCursorPos(ImVec2(x, y));
        char bid[16]; std::snprintf(bid, sizeof(bid), "##f%d", id);
        if (ImGui::InvisibleButton(bid, ImVec2(w, h)))
        {
            s.focused_field = id;
            std::string d = Display(buf, password);
            float mx = io.MousePos.x - (o.x + x + pad) + *scroll;
            int best = 0, bestd = INT_MAX;
            for (int i = 0; i <= (int)d.size(); ++i) { int dd = abs(CaretPx(d, i) - (int)mx); if (dd < bestd) { bestd = dd; best = i; } }
            *caret = best; s.blink = 0;
        }

        bool focused = s.focused_field == id;
        int len = (int)std::strlen(buf);

        if (focused)
        {
            for (int n = 0; n < io.InputQueueCharacters.Size; ++n)
            {
                ImWchar c = io.InputQueueCharacters[n];
                if (c >= 32 && c != 127 && len < cap - 1)
                {
                    std::string t(buf); *caret = std::min(*caret, (int)t.size());
                    t.insert(t.begin() + *caret, (char)c);
                    CopyStr(buf, cap, t);
                    (*caret)++; len = (int)std::strlen(buf); if (changed) *changed = true; s.blink = 0;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Backspace) && *caret > 0)
            { std::string t(buf); t.erase(t.begin() + (*caret - 1)); CopyStr(buf, cap, t); (*caret)--; len = (int)std::strlen(buf); if (changed) *changed = true; s.blink = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_Delete) && *caret < len)
            { std::string t(buf); t.erase(t.begin() + *caret); CopyStr(buf, cap, t); len = (int)std::strlen(buf); if (changed) *changed = true; s.blink = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)  && *caret > 0)   { (*caret)--; s.blink = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && *caret < len) { (*caret)++; s.blink = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_Home)) { *caret = 0; s.blink = 0; }
            if (ImGui::IsKeyPressed(ImGuiKey_End))  { *caret = len; s.blink = 0; }
            s.blink += dt;
        }

        std::string d = Display(buf, password);
        *caret = std::max(0, std::min(*caret, (int)d.size()));

        // glide caret toward target (the "smooth typing" feel)
        float targetX = (float)CaretPx(d, *caret);
        *caretX = Approach(*caretX, targetX, 22.f, dt);

        // keep caret visible
        int avail = (int)(w - pad * 2), cx = (int)*caretX;
        if (cx - *scroll > avail) *scroll = cx - avail;
        if (cx - *scroll < 0)     *scroll = cx;
        if (*scroll < 0) *scroll = 0;

        dl->AddRectFilled(P(x, y), P(x + w, y + h), INPUT_BG);
        dl->AddRect(P(x, y), P(x + w, y + h), focused ? ACCENT : INPUT_BRD);

        dl->PushClipRect(P(x + pad - 1, y), P(x + w - pad + 1, y + h), true);
        float ty = y + (h - ImGui::GetTextLineHeight()) * 0.5f;
        if (len == 0 && !focused) dl->AddText(P(x + pad, ty), GREY70, ph);
        else                      dl->AddText(P(x + pad - *scroll, ty), TYPED, d.c_str());
        if (focused && fmodf(s.blink, 1.f) < 0.5f)
        {
            float cxp = x + pad + *caretX - *scroll;
            dl->AddLine(P(cxp, y + 7), P(cxp, y + h - 7), ACCENT, 1.4f);
        }
        dl->PopClipRect();
    }

    bool Button(ImVec2 o, ImDrawList* dl, float x, float y, float w, float h, const char* label, const char* id)
    {
        auto P = [&](float a, float b) { return ImVec2(o.x + a, o.y + b); };
        ImGui::SetCursorPos(ImVec2(x, y));
        bool clicked = ImGui::InvisibleButton(id, ImVec2(w, h));
        bool hov = ImGui::IsItemHovered();
        dl->AddRectFilled(P(x, y), P(x + w, y + h), hov ? IM_COL32(33, 33, 33, 255) : BTN_BG);
        dl->AddRect(P(x, y), P(x + w, y + h), BTN_BRD);
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(P(x + (w - ts.x) * 0.5f, y + (h - ts.y) * 0.5f), hov ? WHITE : DIMTEXT, label);
        return clicked;
    }
}

// ---------------------------------------------------------------- LOGIN (Form1)
void DrawLogin(GameSdkState& s)
{
    BeginForm("gamesdk_login", 239, 257, BG_LOGIN, s, 26);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = ImGui::GetWindowPos();
    auto P = [&](float a, float b) { return ImVec2(o.x + a, o.y + b); };
    float dt = ImGui::GetIO().DeltaTime > 0 ? ImGui::GetIO().DeltaTime : 1.f / 60.f;

    dl->AddRectFilled(P(0, 0), P(239, 26), BAR);
    dl->AddRectFilled(P(0, 24), P(239, 26), IM_COL32(25, 25, 25, 255));
    dl->AddText(P(12, 7), DIMTEXT, "game.sdk");

    dl->AddText(P(35, 53), WHITE, "enter your crede");
    dl->AddText(P(35 + ImGui::CalcTextSize("enter your crede").x, 53), ACCENT, "ntials to continue");

    bool changed = false;
    SmoothField(o, dl, 17, 88, 200, 1, s.username, sizeof(s.username), "username", false, &s.user_caret, &s.user_scroll, &s.user_caretx, s, &changed);
    SmoothField(o, dl, 17, 126, 200, 2, s.password, sizeof(s.password), "password", true, &s.pass_caret, &s.pass_scroll, &s.pass_caretx, s, &changed);
    if (changed) s.login_error = false;

    // remember me + animated checkbox
    dl->AddText(P(19, 182), WHITE, "remember");
    dl->AddText(P(19 + ImGui::CalcTextSize("remember").x, 182), ACCENT, " me");
    {
        float bx = 196, by = 182, bs = 16;
        ImGui::SetCursorPos(ImVec2(bx, by));
        if (ImGui::InvisibleButton("##remember", ImVec2(bs, bs))) s.remember = !s.remember;
        s.remember_anim = Approach(s.remember_anim, s.remember ? 1.f : 0.f, 17.f, dt);
        dl->AddRectFilled(P(bx, by), P(bx + bs, by + bs), LerpC(BTN_BG, CB_ACCENT, s.remember_anim));
        dl->AddRect(P(bx, by), P(bx + bs, by + bs), BORDER);
        if (s.remember_anim > 0.05f)
        {
            ImU32 wc = IM_COL32(255, 255, 255, (int)(255 * s.remember_anim));
            dl->AddLine(P(bx + 3, by + 8), P(bx + 6, by + 11), wc, 2.f);
            dl->AddLine(P(bx + 6, by + 11), P(bx + 12, by + 4), wc, 2.f);
        }
    }

    if (Button(o, dl, 19, 206, 198, 19, "continue", "##continue"))
    {
        if (std::strcmp(s.username, "slurricane") == 0 && std::strcmp(s.password, "slurricane") == 0)
            s.login_ok = true;
        else
            s.login_error = true;
    }

    if (s.login_error)
        dl->AddText(P(19, 230), IM_COL32(220, 70, 70, 255), "invalid credentials");

    // click empty space unfocuses the text fields
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
        s.focused_field = 0;

    EndForm();
}

// --------------------------------------------------------------- LOADER (Form2)
void DrawLoader(GameSdkState& s)
{
    BeginForm("gamesdk_loader", 303, 236, BG_LOADER, s, 26);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 o = ImGui::GetWindowPos();
    auto P = [&](float a, float b) { return ImVec2(o.x + a, o.y + b); };

    dl->AddRectFilled(P(0, 0), P(303, 26), BAR);
    dl->AddRectFilled(P(0, 24), P(303, 26), IM_COL32(25, 25, 25, 255));
    dl->AddText(P(12, 7), DIMTEXT, "game.sdk");

    Section(dl, o, 7, 32, 91, 192, "Games");
    {
        float tx = 14, ty = 58, tw = 77, th = 28;
        ImGui::SetCursorPos(ImVec2(tx, ty));
        if (ImGui::InvisibleButton("##rust", ImVec2(tw, th))) s.game_tab = 0;
        bool sel = s.game_tab == 0;
        if (sel) dl->AddRectFilled(P(tx, ty), P(tx + 3, ty + th), ACCENT);
        dl->AddText(P(tx + 12, ty + (th - ImGui::CalcTextSize("Rust").y) * 0.5f), sel ? WHITE : GREY70, "Rust");
    }

    Section(dl, o, 102, 32, 194, 192, "Info");
    dl->AddText(P(117, 58),  DIMTEXT, "Status");         dl->AddText(P(214, 58),  GREEN,  "Undetected");
    dl->AddText(P(117, 77),  DIMTEXT, "Last Update");    dl->AddText(P(233, 77),  GREY70, "6/29/26");
    dl->AddText(P(117, 95),  DIMTEXT, "Version");        dl->AddText(P(214, 95),  GREY70, "v1.0 release");
    dl->AddText(P(117, 113), DIMTEXT, "Last Detection"); dl->AddText(P(251, 113), GREEN,  "never");

    if (Button(o, dl, 132, 195, 129, 19, "launch", "##launch"))
        { /* TODO: launch / inject */ }

    EndForm();
}
