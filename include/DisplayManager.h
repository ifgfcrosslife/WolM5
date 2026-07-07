#pragma once

#include <Arduino.h>
#include <lvgl.h>
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

  struct ChipUi {
    lv_obj_t *container = nullptr;
    lv_obj_t *label = nullptr;
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
  bool uiReady = false;
  uint32_t lastTickMs = 0;

  lv_obj_t *screen = nullptr;
  lv_obj_t *titlePill = nullptr;
  lv_obj_t *stateLabel = nullptr;
  lv_obj_t *detailLabel = nullptr;
  lv_obj_t *footerLabel = nullptr;
  lv_obj_t *accentStrip = nullptr;
  ChipUi leftChip;
  ChipUi middleChip;
  ChipUi rightChip;

  void renderScreen(Mode mode, const String &title, const String &message, const String &detail,
                    const String &footer, const String &leftChip, const String &middleChip,
                    const String &rightChip, uint16_t accentColor);
  bool sameState(Mode mode, const String &title, const String &message, const String &detail,
                 const String &footer, const String &leftChip, const String &middleChip,
                 const String &rightChip, uint16_t accentColor) const;
  void initUi();
  void updateChip(ChipUi &chip, const String &text, uint16_t fg, uint16_t bg);
  void updateLabel(lv_obj_t *label, const String &text);
};
