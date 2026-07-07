#include "DisplayManager.h"
#include <M5Unified.h>

namespace {
constexpr uint16_t BG = 0x0000;
constexpr uint16_t CARD = 0x1115;
constexpr uint16_t CARD_EDGE = 0x2A2F;
constexpr uint16_t WHITE_565 = 0xFFFF;
constexpr uint16_t TEXT = 0xF7DE;
constexpr uint16_t MUTED = 0xBDF7;
constexpr uint16_t GREEN_565 = 0x07E0;
constexpr uint16_t AMBER_565 = 0xFD20;
constexpr uint16_t RED_565 = 0xF800;
constexpr uint16_t CHIP_BG = 0x1B20;
constexpr uint16_t CHIP_SOFT = 0x3A40;

String fitText(const String &text, int32_t maxWidth, const lgfx::IFont *font) {
  if (text.isEmpty()) {
    return text;
  }

  if (M5.Display.textWidth(text.c_str(), font) <= maxWidth) {
    return text;
  }

  String cut = text;
  const String ellipsis = "...";
  while (cut.length() > 0) {
    cut.remove(cut.length() - 1);
    String candidate = cut + ellipsis;
    if (M5.Display.textWidth(candidate.c_str(), font) <= maxWidth) {
      return candidate;
    }
  }

  return ellipsis;
}
}

void DisplayManager::begin() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(170);
  M5.Display.fillScreen(BG);
  dirty = true;
}

void DisplayManager::showBoot(const AppConfig &) {
  renderScreen(Mode::Boot, "WOL M5", "Starting", "Preparing bridge", "WiFi / AP / Supabase",
               "BOOT", "WAIT", "M5", AMBER_565);
}

void DisplayManager::showConnecting(const String &ssid, bool hidden) {
  const String detail = hidden ? "Hidden SSID enabled" : "Broadcast SSID";
  renderScreen(Mode::Connecting, "CONNECTING", "Router", ssid, detail,
               "WIFI", "SCAN", "TRY", AMBER_565);
}

void DisplayManager::showConnected(const String &localIp, const String &apIp) {
  renderScreen(Mode::Connected, "ONLINE", "Bridge ready", "LAN " + localIp, "AP " + apIp,
               "AP", "LAN", "READY", GREEN_565);
}

void DisplayManager::showSetupMode(const String &apIp) {
  renderScreen(Mode::Setup, "SETUP MODE", "No router link", "Connect via WOLM5 AP", "AP " + apIp,
               "AP", "SETUP", "WAIT", AMBER_565);
}

void DisplayManager::showError(const String &message) {
  renderScreen(Mode::Error, "ERROR", "Bridge fault", message, "Check serial log",
               "AP", "ERR", "STOP", RED_565);
}

void DisplayManager::tick() {
  if (!dirty) {
    return;
  }
  renderScreen(currentMode, currentTitle, currentMessage, currentDetail, currentFooter,
               currentLeftChip, currentMiddleChip, currentRightChip, currentAccent);
  dirty = false;
}

bool DisplayManager::sameState(Mode mode, const String &title, const String &message, const String &detail,
                               const String &footer, const String &leftChip, const String &middleChip,
                               const String &rightChip, uint16_t accentColor) const {
  return currentMode == mode && currentTitle == title && currentMessage == message &&
         currentDetail == detail && currentFooter == footer && currentLeftChip == leftChip &&
         currentMiddleChip == middleChip && currentRightChip == rightChip && currentAccent == accentColor;
}

void DisplayManager::renderScreen(Mode mode, const String &title, const String &message, const String &detail,
                                  const String &footer, const String &leftChip, const String &middleChip,
                                  const String &rightChip, uint16_t accentColor) {
  if (sameState(mode, title, message, detail, footer, leftChip, middleChip, rightChip, accentColor)) {
    return;
  }

  currentMode = mode;
  currentTitle = title;
  currentMessage = message;
  currentDetail = detail;
  currentFooter = footer;
  currentLeftChip = leftChip;
  currentMiddleChip = middleChip;
  currentRightChip = rightChip;
  currentAccent = accentColor;

  M5.Display.fillScreen(BG);
  M5.Display.setTextDatum(top_left);

  M5.Display.fillRoundRect(8, 8, 112, 20, 10, CARD);
  M5.Display.drawRoundRect(8, 8, 112, 20, 10, CARD_EDGE);
  M5.Display.fillRoundRect(12, 12, 8, 8, 4, accentColor);
  M5.Display.setTextColor(MUTED, CARD);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(24, 14);
  M5.Display.print(fitText(title, 88, &fonts::Font0));

  M5.Display.fillRoundRect(8, 36, 112, 54, 14, CARD);
  M5.Display.drawRoundRect(8, 36, 112, 54, 14, CARD_EDGE);
  M5.Display.fillRoundRect(12, 40, 104, 6, 3, accentColor);

  M5.Display.setTextColor(WHITE_565, CARD);
  M5.Display.setTextSize(1);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setCursor(14, 50);
  M5.Display.print(fitText(message, 92, &fonts::Font2));

  M5.Display.setTextColor(TEXT, CARD);
  M5.Display.setFont(&fonts::Font0);
  M5.Display.setCursor(14, 72);
  M5.Display.print(fitText(detail, 96, &fonts::Font0));

  M5.Display.setTextColor(MUTED, CARD);
  M5.Display.setCursor(14, 84);
  M5.Display.print(fitText(footer, 96, &fonts::Font0));

  drawChip(10, 98, 34, leftChip, WHITE_565, CHIP_BG);
  drawChip(47, 98, 36, middleChip, WHITE_565, CHIP_SOFT);
  drawChip(87, 98, 31, rightChip, WHITE_565, accentColor);
}

void DisplayManager::drawChip(int16_t x, int16_t y, int16_t w, const String &text, uint16_t fg, uint16_t bg) {
  M5.Display.fillRoundRect(x, y, w, 16, 8, bg);
  M5.Display.setTextColor(fg, bg);
  M5.Display.setTextSize(1);
  M5.Display.setCursor(x + 5, y + 4);
  M5.Display.print(fitText(text, w - 10, &fonts::Font0));
}
