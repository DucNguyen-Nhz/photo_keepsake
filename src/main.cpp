#include "anim.h"
#include "content.h"
#include "drive.h"
#include "gothic.h"
#include "layout.h"
#include "photo.h"
#include "render.h"
#include "wallet.h"
#include "window.h"
#include "raylib.h"
#include "rlgl.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <set>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Sleeve {
    SleeveArt art;
    Spring drop{110.0f, 8.0f};    // underdamped: flutters when it drops
    Spring flip{70.0f, 11.0f};
};

enum class HitKind { None, Menu, Wallet, Sleeve };
struct Hit {
    HitKind kind = HitKind::None;
    int index = -1;
};

constexpr int kMenuItems = 4;   // keep on top, open/close, refresh, quit
constexpr float kMenuRow = 26.0f;

// Background Drive sync; shared so a sync still running at quit can't touch a dead App.
struct SyncJob {
    std::atomic<bool> done{false};
    SyncResult result;
};

constexpr float kPeek = 26.0f;        // leather left visible at the screen edge when tucked away
constexpr float kDockSnap = 80.0f;    // release this close to an edge to dock
constexpr float kUndockPull = 60.0f;  // drag this far from the edge to undock

Rectangle MonitorAt(Vector2 g) {
    Rectangle best = {0, 0, (float)GetMonitorWidth(0), (float)GetMonitorHeight(0)};
    float bestD = 1e30f;
    for (int i = 0; i < GetMonitorCount(); i++) {
        Vector2 p = GetMonitorPosition(i);
        Rectangle r = {p.x, p.y, (float)GetMonitorWidth(i), (float)GetMonitorHeight(i)};
        if (CheckCollisionPointRec(g, r)) return r;
        float dx = std::max({r.x - g.x, 0.0f, g.x - (r.x + r.width)});
        float dy = std::max({r.y - g.y, 0.0f, g.y - (r.y + r.height)});
        if (dx * dx + dy * dy < bestD) {
            bestD = dx * dx + dy * dy;
            best = r;
        }
    }
    return best;
}

class App {
public:
    bool Init(int argc, char** argv);
    void Run();
    void Shutdown();

private:
    void Update(float dt);
    void UpdateInput(float dt);
    void UpdateSequence();
    void Draw();
    void DrawMenu();
    Hit HitTest(Vector2 m) const;
    Rectangle MenuRect() const;
    std::string MenuLabel(int i) const;
    void Toggle();
    void Step(int dir);
    void LoadAssets();
    void UnloadAssets();
    void FitWindow();
    void Reload();        // re-read the photo folder and rebake everything
    void Refresh();       // sync from Drive (if configured), then reload
    float WalletGlobalX() const;     // screen x of the inside panel's left edge
    float HiddenOffset() const;      // slide offset that tucks the wallet behind the dock edge
    void DockTo(int edge, Vector2 winPos);
    void TryDock();
    void Undock();

    std::vector<std::string> args_;
    std::shared_ptr<SyncJob> sync_;
    Content content_;
    Theme theme_ = Theme::Dracula;
    Font font_{};        // notes
    Font titleFont_{};   // cover lettering
    bool ownFont_ = false, ownTitle_ = false;
    WalletArt wallet_;
    RenderTexture2D gloss_{};
    std::vector<Sleeve> sleeves_;
    std::vector<SleeveGeom> geoms_;
    WalletHits walletHits_{};

    Spring cover_{55.0f, 10.5f};
    // Docking: the window stays flush with the monitor edge and its content slides in/out.
    int dock_ = 1;                   // 0 floating, -1 left edge, +1 right edge
    int side_ = 1;                   // wallet layout, see WalletPose::side
    Spring slide_{90.0f, 17.0f};     // content x offset inside the window
    bool hoverPeek_ = false;
    bool wantOpen_ = false;
    int openCount_ = 1;
    int maxOpen_ = 1;     // number of photos
    Spring scroll_{80.0f, 17.0f};   // content shifted up so the lowest hanging photo stays on screen
    float time_ = 0.0f;
    Dust dust_;
    Bats bats_;
    Petals petals_;
    float petalTimer_ = 0.0f;
    float moteTimer_ = 0.0f;
    bool dustDone_ = false;

    bool pressing_ = false, dragging_ = false;
    Vector2 grabGlobal_{}, grabWin_{};
    float wheelAcc_ = 0.0f, wheelCooldown_ = 0.0f;
    bool menuOpen_ = false;
    Vector2 menuPos_{};
    int cursor_ = MOUSE_CURSOR_DEFAULT;
    bool quit_ = false;
};

Font LoadFontFor(const Content& c, const std::string& path, bool& owned) {
    std::string all;
    for (int ch = 32; ch < 127; ch++) all += (char)ch;
    for (const auto& it : c.items) all += it.note;
    all += c.cfg.initials;
    int count = 0;
    int* cps = LoadCodepoints(all.c_str(), &count);
    std::set<int> uniq(cps, cps + count);
    UnloadCodepoints(cps);
    std::vector<int> list(uniq.begin(), uniq.end());

    Font f{};
    if (FileExists(path.c_str())) f = LoadFontEx(path.c_str(), 64, list.data(), (int)list.size());
    owned = f.texture.id != 0;
    if (!owned) {
        TraceLog(LOG_WARNING, "Font not found (%s), using default", path.c_str());
        return GetFontDefault();
    }
    GenTextureMipmaps(&f.texture);
    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
    return f;
}

bool App::Init(int argc, char** argv) {
    args_.assign(argv, argv + argc);
    content_ = LoadContent(argc, argv);
    SetTraceLogLevel(LOG_WARNING);
    OpenWidgetWindow((int)L::kWindowW, 400, "Wallet");
    if (!IsWindowReady()) return false;
    SetTargetFPS(60);
    FitWindow();

    int mon = GetCurrentMonitor();
    Vector2 monPos = GetMonitorPosition(mon);
    SavedState st;
    st.x = (int)(monPos.x + GetMonitorWidth(mon) - L::kWindowW);
    st.y = (int)monPos.y + 40;
    LoadSavedState(st);
    if (content_.cfg.x >= 0 && content_.cfg.y >= 0) {
        st = {content_.cfg.x, content_.cfg.y, 0};
    }
    SetWindowPosition(st.x, st.y);
    dock_ = 0;
    if (st.dock != 0) {
        DockTo(st.dock, {(float)st.x, (float)st.y});
        slide_.x = slide_.target = HiddenOffset();
    }

    InitRender();
    LoadAssets();

    if (content_.startOpen > 0) {
        wantOpen_ = true;
        slide_.x = slide_.target = 0.0f;
        openCount_ = std::min(content_.startOpen, maxOpen_);
        cover_.x = cover_.target = 1.0f;
        dustDone_ = true;
        for (int i = 0; i < openCount_; i++) sleeves_[i].drop.x = sleeves_[i].drop.target = 1.0f;
        if (content_.startFlip >= 0 && content_.startFlip < openCount_)
            sleeves_[content_.startFlip].flip.target = 1.0f;
    }
    if (!content_.driveId.empty()) Refresh();   // show the cache now, sync in the background
    return true;
}

// Size the window to fit as many hanging sleeves as the monitor allows.
void App::FitWindow() {
    Vector2 wp = GetWindowPosition();
    Rectangle mon = MonitorAt({wp.x + L::kWindowW * 0.5f, wp.y + 100.0f});
    auto needH = [](int k) {
        return L::kTop + L::kWalletH - L::kPocketDepth + k * L::kSleeveStep + L::kMargin + 12.0f;
    };
    maxOpen_ = std::max(1, (int)content_.items.size());
    int fit = maxOpen_;   // more photos than this scroll the wallet up instead
    while (fit > 1 && needH(fit) > mon.height - 60) fit--;
    SetWindowSize((int)L::kWindowW, (int)needH(fit));
    openCount_ = std::clamp(openCount_, 1, maxOpen_);
}

void App::LoadAssets() {
    theme_ = content_.cfg.theme;
    bool gothic = theme_ == Theme::Dracula;
    std::string fonts = AssetDir() + "fonts/";
    std::string notePath = !content_.cfg.fontPath.empty() ? content_.cfg.fontPath
                           : gothic ? fonts + "CormorantGaramond-Italic.ttf"
                                    : fonts + "PatrickHand-Regular.ttf";
    font_ = LoadFontFor(content_, notePath, ownFont_);
    titleFont_ = gothic ? LoadFontFor(content_, fonts + "GrenzeGotisch.ttf", ownTitle_) : font_;
    if (!gothic) ownTitle_ = false;
    wallet_ = BakeWallet(content_.cfg, titleFont_, font_);
    if (!gothic) gloss_ = BakeGloss();
    int n = (int)content_.items.size();
    sleeves_.resize(n);   // keeps the pose of sleeves that still exist
    for (int i = 0; i < n; i++)
        sleeves_[i].art = BakeSleeve(content_.items[i], font_, content_.cfg.aging, i + 1, theme_);
}

void App::UnloadAssets() {
    for (auto& s : sleeves_) UnloadSleeve(s.art);
    if (gloss_.id != 0) UnloadRenderTexture(gloss_);
    gloss_ = {};
    UnloadWallet(wallet_);
    if (ownFont_) UnloadFont(font_);
    if (ownTitle_) UnloadFont(titleFont_);
    ownFont_ = ownTitle_ = false;
}

void App::Reload() {
    std::vector<char*> argv;
    for (auto& a : args_) argv.push_back(a.data());
    int startOpen = content_.startOpen, startFlip = content_.startFlip;
    UnloadAssets();
    geoms_.clear();   // last frame's sleeve geometry may index photos that are gone
    content_ = LoadContent((int)argv.size(), argv.data());
    content_.startOpen = startOpen;
    content_.startFlip = startFlip;
    LoadAssets();
    FitWindow();
}

void App::Refresh() {
    if (content_.driveId.empty()) {
        Reload();
        return;
    }
    if (sync_) return;   // already syncing
    sync_ = std::make_shared<SyncJob>();
    std::thread([job = sync_, id = content_.driveId] {
        job->result = SyncDriveFolder(id, DriveCacheDir(id));
        job->done = true;
    }).detach();
}

void App::Shutdown() {
    Vector2 p = GetWindowPosition();
    SaveState({(int)p.x, (int)p.y, dock_});
    UnloadAssets();
    ShutdownRender();
    CloseWindow();
}

void App::Toggle() {
    wantOpen_ = !wantOpen_;
    if (wantOpen_) openCount_ = 1;
}

void App::Step(int dir) {
    if (!wantOpen_) {
        if (dir > 0) Toggle();
        return;
    }
    openCount_ = std::clamp(openCount_ + dir, 1, maxOpen_);
}

float App::WalletGlobalX() const {
    return GetWindowPosition().x + slide_.x + PanelX0(side_);
}

float App::HiddenOffset() const {
    if (dock_ > 0) return (L::kWindowW - kPeek) - PanelX0(side_);
    if (dock_ < 0) return kPeek - (PanelX0(side_) + L::kWalletW);
    return 0.0f;
}

// Snap the window flush to a monitor edge without moving the wallet on screen.
void App::DockTo(int edge, Vector2 wp) {
    float gx = wp.x + slide_.x + PanelX0(side_);
    Rectangle mon = MonitorAt({gx + L::kWalletW * 0.5f, wp.y + L::kTop + L::kWalletH * 0.5f});
    dock_ = edge;
    side_ = edge;   // cover always opens toward the middle of the screen
    float x = edge > 0 ? mon.x + mon.width - L::kWindowW : mon.x;
    float y = std::clamp(wp.y, mon.y, std::max(mon.y, mon.y + mon.height - L::kTop - L::kWalletH - 20));
    slide_.x = gx - x - PanelX0(side_);
    slide_.v = 0.0f;
    SetWindowPosition((int)x, (int)y);
}

void App::TryDock() {
    float gx = WalletGlobalX();
    Vector2 wp = GetWindowPosition();
    Rectangle mon = MonitorAt({gx + L::kWalletW * 0.5f, wp.y + L::kTop + L::kWalletH * 0.5f});
    if (mon.x + mon.width - (gx + L::kWalletW) < kDockSnap) DockTo(1, wp);
    else if (gx - mon.x < kDockSnap) DockTo(-1, wp);
}

// Float freely again; the window jumps so the wallet stays under the cursor.
void App::Undock() {
    Vector2 wp = GetWindowPosition();
    SetWindowPosition((int)(wp.x + slide_.x), (int)wp.y);
    slide_.x = slide_.target = slide_.v = 0.0f;
    dock_ = 0;
}

Rectangle App::MenuRect() const {
    return {menuPos_.x, menuPos_.y, 170, kMenuItems * kMenuRow + 10};
}

std::string App::MenuLabel(int i) const {
    if (i == 0) return IsTopmost() ? "Keep on top: yes" : "Keep on top: no";
    if (i == 1) return wantOpen_ ? "Close wallet" : "Open wallet";
    if (i == 2) return sync_ ? "Refreshing..." : "Refresh photos";
    return "Put away (quit)";
}

Hit App::HitTest(Vector2 m) const {
    if (menuOpen_ && CheckCollisionPointRec(m, MenuRect())) {
        int idx = (int)((m.y - menuPos_.y - 5) / kMenuRow);
        return {HitKind::Menu, std::clamp(idx, 0, kMenuItems - 1)};
    }
    m.x -= slide_.x;   // wallet and sleeves are drawn shifted by the slide and scroll
    m.y += scroll_.x;
    if (PointInQuad(m, walletHits_.cover) || PointInQuad(m, walletHits_.panel))
        return {HitKind::Wallet, -1};
    for (int i = 0; i < (int)geoms_.size(); i++)
        if (PointInQuad(m, geoms_[i].quad)) return {HitKind::Sleeve, i};
    return {};
}

void App::UpdateInput(float dt) {
    Vector2 m = CursorLocal();
    Hit hit = HitTest(m);

    if (menuOpen_) {
        Rectangle r = MenuRect();
        Rectangle near = {r.x - 30, r.y - 30, r.width + 60, r.height + 60};
        if (!CheckCollisionPointRec(m, near)) menuOpen_ = false;
    }
    SetPassthrough(hit.kind == HitKind::None && !pressing_ && !menuOpen_);

    int want = hit.kind == HitKind::None ? MOUSE_CURSOR_DEFAULT : MOUSE_CURSOR_POINTING_HAND;
    if (dragging_) want = MOUSE_CURSOR_RESIZE_ALL;
    if (want != cursor_) {
        SetMouseCursor(want);
        cursor_ = want;
    }

    hoverPeek_ = dock_ != 0 && !wantOpen_ && hit.kind == HitKind::Wallet;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hit.kind == HitKind::Menu) {
            if (hit.index == 0) SetTopmost(!IsTopmost());
            else if (hit.index == 1) Toggle();
            else if (hit.index == 2) Refresh();
            else quit_ = true;
        } else if (hit.kind == HitKind::Wallet) {
            pressing_ = true;
            dragging_ = false;
            grabWin_ = GetWindowPosition();
            grabGlobal_ = {grabWin_.x + m.x, grabWin_.y + m.y};
        } else if (hit.kind == HitKind::Sleeve && sleeves_[hit.index].drop.x > 0.85f) {
            Spring& f = sleeves_[hit.index].flip;
            f.target = f.target > 0.5f ? 0.0f : 1.0f;
        }
        menuOpen_ = false;
    }
    if (pressing_) {
        Vector2 wp = GetWindowPosition();
        Vector2 g = {wp.x + m.x, wp.y + m.y};
        float dx = g.x - grabGlobal_.x, dy = g.y - grabGlobal_.y;
        if (!dragging_ && dx * dx + dy * dy > 16.0f) dragging_ = true;
        if (dragging_ && dock_ != 0 && std::fabs(dx) > kUndockPull) {
            Undock();
            grabWin_ = GetWindowPosition();
            grabGlobal_ = g;
            dx = dy = 0.0f;
        }
        if (dragging_) {
            if (dock_ != 0) SetWindowPosition((int)grabWin_.x, (int)(grabWin_.y + dy));   // slide along the edge
            else SetWindowPosition((int)(grabWin_.x + dx), (int)(grabWin_.y + dy));
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (!dragging_) Toggle();
            else if (dock_ == 0) TryDock();
            pressing_ = dragging_ = false;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hit.kind != HitKind::None &&
        hit.kind != HitKind::Menu) {
        menuOpen_ = true;
        menuPos_ = {std::min(m.x, L::kWindowW - 175.0f),
                    std::min(m.y, GetScreenHeight() - (kMenuItems * kMenuRow + 15.0f))};
    }

    // Wheel: down drops the next sleeve, up folds the last one back.
    wheelAcc_ += GetMouseWheelMove();
    wheelCooldown_ -= dt;
    if (wheelCooldown_ <= 0.0f && std::fabs(wheelAcc_) >= 0.5f) {
        Step(wheelAcc_ < 0 ? 1 : -1);
        wheelAcc_ = 0.0f;
        wheelCooldown_ = 0.14f;
    }
    wheelAcc_ *= std::pow(0.02f, dt);

    if (IsKeyPressed(KEY_DOWN)) Step(1);
    if (IsKeyPressed(KEY_UP)) Step(-1);
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) Toggle();
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (menuOpen_) menuOpen_ = false;
        else wantOpen_ = false;
    }
    bool mod = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
               IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);   // Cmd on macOS
    if (IsKeyPressed(KEY_Q) && mod)
        quit_ = true;
}

// Slide in, cover opens, then each sleeve drops once its parent hangs; closing runs in reverse.
void App::UpdateSequence() {
    int n = (int)sleeves_.size();
    if (wantOpen_) {
        slide_.target = 0.0f;
        if (std::fabs(slide_.x) < 30.0f) cover_.target = 1.0f;
    }
    for (int i = 0; i < n; i++) {
        Sleeve& s = sleeves_[i];
        bool want = wantOpen_ && i < openCount_;
        float parent = i == 0 ? cover_.x : sleeves_[i - 1].drop.x;
        bool childDown = i + 1 < n && sleeves_[i + 1].drop.x > 0.12f;
        if (want) {
            if (parent > 0.8f) s.drop.target = 1.0f;
        } else {
            s.flip.target = 0.0f;
            if (!childDown) s.drop.target = 0.0f;
        }
    }
    if (!wantOpen_) {
        bool anyDown = std::any_of(sleeves_.begin(), sleeves_.end(),
                                   [](const Sleeve& s) { return s.drop.x > 0.06f; });
        cover_.target = anyDown ? 1.0f : 0.0f;
        if (!anyDown && cover_.x < 0.04f) {
            float nudge = hoverPeek_ ? -dock_ * 14.0f : 0.0f;   // peek out a little more on hover
            slide_.target = HiddenOffset() + nudge;
        }
    }
}

void App::Update(float dt) {
    time_ += dt;
    if (sync_ && sync_->done) {
        SyncResult r = sync_->result;
        sync_.reset();
        if (!r.ok) TraceLog(LOG_WARNING, "Drive sync: %s", r.error.c_str());
        if (r.changed) Reload();
    }
    UpdateInput(dt);
    UpdateSequence();
    // Scroll so the lowest photo that is (or is about to be) hanging stays in view.
    int hanging = 0;
    for (const auto& sl : sleeves_) hanging += sl.drop.target > 0.5f ? 1 : 0;
    float bottom = L::kTop + L::kWalletH - L::kPocketDepth + hanging * L::kSleeveStep + 24.0f;
    scroll_.target = std::max(0.0f, bottom - (float)GetScreenHeight());
    cover_.Update(dt);
    slide_.Update(dt);
    scroll_.Update(dt);
    for (auto& s : sleeves_) {
        s.drop.Update(dt);
        s.flip.Update(dt);
    }
    if (wantOpen_ && !dustDone_ && cover_.x > 0.3f && theme_ == Theme::Dracula) {
        bats_.Burst({PanelX0(side_) + L::kWalletW * 0.5f, L::kTop + L::kWalletH * 0.4f}, 3);
        petals_.Burst({L::kHingeX - 60, L::kTop + 20}, {L::kHingeX + 60, L::kTop + L::kWalletH - 20}, 22);
        dust_.Burst({L::kHingeX, L::kTop + 6}, {L::kHingeX, L::kTop + L::kWalletH - 6}, 40);
        dustDone_ = true;
    }
    if (wantOpen_ && !dustDone_ && cover_.x > 0.3f) {
        dust_.Burst({L::kHingeX, L::kTop + 6}, {L::kHingeX, L::kTop + L::kWalletH - 6}, 34);
        float px = PanelX0(side_);
        dust_.Burst({px + 20, L::kTop + L::kWalletH * 0.46f},
                    {px + L::kWalletW - 20, L::kTop + L::kWalletH * 0.46f}, 16);
        dustDone_ = true;
    }
    if (!wantOpen_ && cover_.x < 0.05f) dustDone_ = false;
    dust_.Update(dt);
    bats_.Update(dt);
    petals_.Update(dt);
    // while open, a petal now and then drifts down past the frames
    if (theme_ == Theme::Dracula && wantOpen_ && cover_.x > 0.9f && (petalTimer_ -= dt) <= 0.0f) {
        float x0 = PanelX0(side_);
        petals_.Burst({x0 + 10, L::kTop + L::kWalletH}, {x0 + L::kWalletW - 10, L::kTop + L::kWalletH}, 1);
        petalTimer_ = Frand(0.8f, 2.0f);
    }
    if (theme_ == Theme::Dracula && cover_.x > 0.9f && (moteTimer_ -= dt) <= 0.0f) {
        float hang = (float)geoms_.size() * L::kSleeveStep;
        dust_.Motes({PanelX0(side_) - 10, L::kTop, L::kWalletW + 20, L::kWalletH + hang}, 1);
        moteTimer_ = 0.12f;
    }
}

void App::Draw() {
    std::vector<SleevePose> poses(sleeves_.size());
    for (size_t i = 0; i < sleeves_.size(); i++) {
        const Sleeve& s = sleeves_[i];
        float settled = std::clamp((s.drop.x - 0.9f) * 10.0f, 0.0f, 1.0f);
        poses[i] = {s.drop.x, s.flip.x,
                    0.014f * settled * std::sin(time_ * 1.25f + i * 1.9f)};
    }
    geoms_ = LayoutSleeves(poses, SleeveAnchor(side_));
    WalletPose wp{cover_.x, time_, side_};

    rlPushMatrix();
    rlTranslatef(slide_.x, -scroll_.x, 0.0f);
    BeginStraightBlend();
    for (const auto& g : geoms_) DrawSleeveShadow(g, 0.28f);
    DrawWalletShadow(wp);

    BeginPremulBlend();
    bool gothic = theme_ == Theme::Dracula;
    for (int i = (int)geoms_.size() - 1; i >= 0; i--) {
        const SleeveGeom& g = geoms_[i];
        if (!gothic) {
            DrawSleeve(g, sleeves_[i].art, gloss_.texture, ShadeTint(g.shade));
            continue;
        }
        // lit by the page candle: warm, flickering
        float v = g.shade * (0.94f + 0.06f * CandleFlicker(time_, i * 2.7f));
        auto c = [](float x) { return (unsigned char)std::clamp(x * 255.0f, 0.0f, 255.0f); };
        DrawSleeve(g, sleeves_[i].art, Texture2D{}, Color{c(v), c(v * 0.93f), c(v * 0.84f), 255});
    }
    walletHits_ = DrawWallet(wallet_, wp);
    if (gothic) {
        BeginStraightBlend();
        DrawSpider({PanelX0(side_) + 6, L::kTop + L::kWalletH - 2}, time_,
                   std::clamp((cover_.x - 0.6f) * 3.0f, 0.0f, 1.0f));
        BeginPremulBlend();
    }
    if (gothic && cover_.x > 0.5f) {
        DrawCandleFlame(PageWick(side_), 1.0f, time_, 0.9f, std::clamp((cover_.x - 0.5f) * 2.5f, 0.0f, 1.0f));
    }

    BeginStraightBlend();
    dust_.Draw();
    petals_.Draw();
    bats_.Draw();
    rlDrawRenderBatchActive();
    rlPopMatrix();
    if (menuOpen_) DrawMenu();
}

void App::DrawMenu() {
    Rectangle r = MenuRect();
    DrawRectangleRounded({r.x + 3, r.y + 5, r.width, r.height}, 0.12f, 6, Color{0, 0, 0, 60});
    DrawRectangleRounded(r, 0.12f, 6, Color{238, 228, 204, 250});
    DrawRectangleRoundedLinesEx(r, 0.12f, 6, 1.0f, Color{120, 88, 56, 170});
    Hit h = HitTest(CursorLocal());
    for (int i = 0; i < kMenuItems; i++) {
        Rectangle row = {r.x + 4, r.y + 5 + i * kMenuRow, r.width - 8, kMenuRow};
        if (h.kind == HitKind::Menu && h.index == i)
            DrawRectangleRounded(row, 0.3f, 4, Color{150, 108, 66, 55});
        DrawTextEx(font_, MenuLabel(i).c_str(), {row.x + 8, row.y + 3}, 20, 0, Color{40, 34, 60, 230});
    }
}

void App::Run() {
    while (!quit_ && !WindowShouldClose()) {
        Update(GetFrameTime());
        BeginDrawing();
        ClearBackground(BLANK);
        Draw();
        EndDrawing();
    }
}

}  // namespace

int main(int argc, char** argv) {
    App app;
    if (!app.Init(argc, argv)) return 1;
    app.Run();
    app.Shutdown();
    return 0;
}
