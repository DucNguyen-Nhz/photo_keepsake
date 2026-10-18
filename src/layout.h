#pragma once

// All sizes are in window pixels (1x). Baked textures are rendered at kBakeScale.
namespace L {
constexpr float kBakeScale = 2.0f;
constexpr float kPad = 3.0f;          // transparent padding around baked art (keeps warped edges smooth)
constexpr float kMargin = 48.0f;      // room around content for shadows / perspective bulge

constexpr float kWalletW = 250.0f;
constexpr float kWalletH = 172.0f;
constexpr float kWalletR = 14.0f;

constexpr float kSleeveW = 224.0f;
constexpr float kSleeveH = 166.0f;
constexpr float kSleeveTab = 8.0f;    // plastic hinge strip on top of each sleeve
constexpr float kSleeveStep = kSleeveTab + kSleeveH;
constexpr float kPocketDepth = 16.0f; // how far the first hinge sits inside the wallet pocket

constexpr float kPerspective = 1400.0f;

constexpr float kWindowW = 2.0f * (kMargin + kWalletW);
constexpr float kHingeX = kMargin + kWalletW;   // spine of the wallet
constexpr float kTop = kMargin;
}
