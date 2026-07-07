#pragma once

#include <Arduino.h>
#include "AppConfig.h"

class DisplayManager {
 public:
  void begin();
  void showBoot(const AppConfig &config);
  void showConnecting(const String &ssid, bool hidden);
  void showConnected(const String &localIp, const String &apIp);
  void showSetupMode(const String &apIp);
  void showError(const String &message);
  void tick();

 private:
  enum class Mode {
    Boot,
    Connecting,
    Connected,
    Setup,
    Error,
  };

  Mode currentMode = Mode::Boot;
  String currentTitle;
  String currentMessage;
  String currentDetail;
  String currentFooter;
  String currentLeftChip;
  String currentMiddleChip;
  String currentRightChip;
  uint16_t currentAccent = 0;
  bool dirty = true;

  void renderScreen(Mode mode, const String &title, const String &message, const String &detail,
                    const String &footer, const String &leftChip, const String &middleChip,
                    const String &rightChip, uint16_t accentColor);
  bool sameState(Mode mode, const String &title, const String &message, const String &detail,
                 const String &footer, const String &leftChip, const String &middleChip,
                 const String &rightChip, uint16_t accentColor) const;
  void drawChip(int16_t x, int16_t y, int16_t w, const String &text, uint16_t fg, uint16_t bg);
};
