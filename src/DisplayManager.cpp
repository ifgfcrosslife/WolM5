#include "DisplayManager.h"

#include <M5Unified.h>

namespace {
constexpr uint16_t BG = 0x0610;
constexpr uint16_t PANEL = 0x0E16;
constexpr uint16_t PANEL_EDGE = 0x2432;
constexpr uint16_t TEXT = 0xF3F6;
constexpr uint16_t MUTED = 0xAAB7;
constexpr uint16_t CHIP_TEXT = 0xF8FA;
constexpr uint16_t CHIPS_BG = 0x111A;
constexpr uint16_t CHIPS_ALT = 0x1827;
constexpr uint16_t CHIPS_SOFT = 0x1E2B;
constexpr uint16_t GREEN_565 = 0x07E0;
constexpr uint16_t AMBER_565 = 0xFD20;
constexpr uint16_t RED_565 = 0xF800;

constexpr int SCREEN_W = 128;
constexpr int SCREEN_H = 128;
constexpr int DRAW_BUF_HEIGHT = 16;

lv_disp_draw_buf_t drawBuf;
lv_color_t drawBuf1[SCREEN_W * DRAW_BUF_HEIGHT];
lv_color_t drawBuf2[SCREEN_W * DRAW_BUF_HEIGHT];
lv_disp_drv_t dispDrv;
bool lvglReady = false;

lv_color_t colorFrom565(uint16_t color) {
  return lv_color_hex(color);
}

String labelText(const String &text) {
  return text.isEmpty() ? String(" ") : text;
}

String chipSymbol(const String &token) {
  if (token == "READY") {
    return LV_SYMBOL_OK;
  }
  if (token == "AP") {
    return LV_SYMBOL_HOME;
  }
  if (token == "LAN") {
    return LV_SYMBOL_WIFI;
  }
  if (token == "WIFI") {
    return LV_SYMBOL_WIFI;
  }
  if (token == "SCAN") {
    return LV_SYMBOL_REFRESH;
  }
  if (token == "TRY") {
    return LV_SYMBOL_REFRESH;
  }
  if (token == "SETUP") {
    return LV_SYMBOL_EDIT;
  }
  if (token == "WAIT") {
    return LV_SYMBOL_REFRESH;
  }
  if (token == "ERR") {
    return LV_SYMBOL_CLOSE;
  }
  if (token == "STOP") {
    return LV_SYMBOL_CLOSE;
  }
  if (token == "M5") {
    return LV_SYMBOL_HOME;
  }
  return token;
}

void flushCb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  const int32_t width = area->x2 - area->x1 + 1;
  const int32_t height = area->y2 - area->y1 + 1;

  M5.Display.startWrite();
  M5.Display.pushImage(area->x1, area->y1, width, height,
                       reinterpret_cast<const lgfx::rgb565_t *>(color_p));
  M5.Display.endWrite();

  lv_disp_flush_ready(disp);
}
}  // namespace

void DisplayManager::begin() {
  auto cfg = M5.config();
  cfg.output_power = true;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  M5.Display.setBrightness(190);
  M5.Display.fillScreen(BG);

  if (!lvglReady) {
    lv_init();
    lv_disp_draw_buf_init(&drawBuf, drawBuf1, drawBuf2, SCREEN_W * DRAW_BUF_HEIGHT);
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = SCREEN_W;
    dispDrv.ver_res = SCREEN_H;
    dispDrv.flush_cb = flushCb;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);
    lvglReady = true;
  }

  initUi();
  dirty = true;
  lastTickMs = millis();
}

void DisplayManager::showBoot(const AppConfig &config) {
  deviceTitle = config.deviceName.isEmpty() ? String("ATOMS3") : config.deviceName;
  renderScreen(Mode::Boot, "BOOT", "Starting", "Preparing bridge", "WiFi / AP / Supabase",
               "BOOT", "WAIT", "M5", AMBER_565);
}

void DisplayManager::showConnecting(const String &ssid, bool hidden) {
  const String detail = hidden ? "Hidden SSID enabled" : "Broadcast SSID";
  renderScreen(Mode::Connecting, "CONNECT", "Router", ssid, detail,
               "WIFI", "SCAN", "TRY", AMBER_565);
}

void DisplayManager::showConnected(const String &localIp, const String &apIp) {
  renderScreen(Mode::Connected, "ONLINE", "LAN", localIp, "AP " + apIp,
               "AP", "LAN", "READY", GREEN_565);
}

void DisplayManager::showSetupMode(const String &apIp) {
  renderScreen(Mode::Setup, "SETUP", "No router link", "Connect via WOLM5 AP", "AP " + apIp,
               "AP", "SETUP", "WAIT", AMBER_565);
}

void DisplayManager::showError(const String &message) {
  renderScreen(Mode::Error, "ERROR", "Bridge fault", message, "Check serial log",
               "AP", "ERR", "STOP", RED_565);
}

void DisplayManager::tick() {
  if (!lvglReady) {
    return;
  }

  const uint32_t now = millis();
  if (lastTickMs == 0) {
    lastTickMs = now;
  } else if (now > lastTickMs) {
    lv_tick_inc(now - lastTickMs);
    lastTickMs = now;
  }

  lv_timer_handler();

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

void DisplayManager::initUi() {
  screen = lv_scr_act();
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(screen, colorFrom565(BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

  titlePill = lv_obj_create(screen);
  lv_obj_set_size(titlePill, 114, 18);
  lv_obj_align(titlePill, LV_ALIGN_TOP_MID, 0, 5);
  lv_obj_clear_flag(titlePill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(titlePill, 9, 0);
  lv_obj_set_style_bg_color(titlePill, colorFrom565(PANEL), 0);
  lv_obj_set_style_border_color(titlePill, colorFrom565(PANEL_EDGE), 0);
  lv_obj_set_style_border_width(titlePill, 1, 0);
  lv_obj_set_style_pad_all(titlePill, 0, 0);
  lv_obj_set_style_shadow_width(titlePill, 0, 0);

  titleLabel = lv_label_create(titlePill);
  lv_label_set_text(titleLabel, "ATOMS3");
  lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(titleLabel, 106);
  lv_obj_center(titleLabel);
  lv_obj_set_style_text_color(titleLabel, colorFrom565(MUTED), 0);
  lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_12, 0);

  lv_obj_t *card = lv_obj_create(screen);
  lv_obj_set_size(card, 112, 68);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 28);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(card, 16, 0);
  lv_obj_set_style_bg_color(card, colorFrom565(PANEL), 0);
  lv_obj_set_style_border_color(card, colorFrom565(PANEL_EDGE), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 0, 0);

  accentStrip = lv_obj_create(card);
  lv_obj_set_size(accentStrip, 100, 3);
  lv_obj_align(accentStrip, LV_ALIGN_TOP_MID, 0, 5);
  lv_obj_clear_flag(accentStrip, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(accentStrip, 2, 0);
  lv_obj_set_style_border_width(accentStrip, 0, 0);
  lv_obj_set_style_bg_color(accentStrip, colorFrom565(AMBER_565), 0);

  stateLabel = lv_label_create(card);
  lv_label_set_long_mode(stateLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(stateLabel, 96);
  lv_obj_align(stateLabel, LV_ALIGN_TOP_MID, 0, 19);
  lv_obj_set_style_text_color(stateLabel, colorFrom565(TEXT), 0);
  lv_obj_set_style_text_font(stateLabel, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(stateLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(stateLabel, "BOOTING");

  detailLabel = lv_label_create(card);
  lv_label_set_long_mode(detailLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(detailLabel, 96);
  lv_obj_align(detailLabel, LV_ALIGN_TOP_MID, 0, 46);
  lv_obj_set_style_text_color(detailLabel, colorFrom565(MUTED), 0);
  lv_obj_set_style_text_font(detailLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_align(detailLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(detailLabel, "Preparing bridge");

  footerLabel = lv_label_create(screen);
  lv_label_set_long_mode(footerLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_width(footerLabel, 104);
  lv_obj_align(footerLabel, LV_ALIGN_TOP_MID, 0, 98);
  lv_obj_set_style_text_color(footerLabel, colorFrom565(MUTED), 0);
  lv_obj_set_style_text_font(footerLabel, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_align(footerLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(footerLabel, "WiFi / AP / Supabase");

  auto createChip = [](lv_obj_t *parent, int16_t x, int16_t y, int16_t w) -> ChipUi {
    ChipUi chip;
    chip.container = lv_obj_create(parent);
    lv_obj_set_size(chip.container, w, 18);
    lv_obj_align(chip.container, LV_ALIGN_TOP_LEFT, x, y);
    lv_obj_clear_flag(chip.container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(chip.container, 9, 0);
    lv_obj_set_style_border_width(chip.container, 0, 0);
    lv_obj_set_style_pad_all(chip.container, 0, 0);
    lv_obj_set_style_shadow_width(chip.container, 0, 0);
    chip.label = lv_label_create(chip.container);
    lv_label_set_long_mode(chip.label, LV_LABEL_LONG_DOT);
    lv_obj_set_width(chip.label, w - 8);
    lv_obj_center(chip.label);
    lv_obj_set_style_text_font(chip.label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_align(chip.label, LV_TEXT_ALIGN_CENTER, 0);
    return chip;
  };

  leftChip = createChip(screen, 7, 107, 32);
  middleChip = createChip(screen, 48, 107, 32);
  rightChip = createChip(screen, 89, 107, 32);

  updateChip(leftChip, LV_SYMBOL_HOME, CHIP_TEXT, CHIPS_BG);
  updateChip(middleChip, LV_SYMBOL_WIFI, CHIP_TEXT, CHIPS_ALT);
  updateChip(rightChip, LV_SYMBOL_OK, CHIP_TEXT, CHIPS_SOFT);

  uiReady = true;
}

void DisplayManager::updateLabel(lv_obj_t *label, const String &text) {
  if (!label) {
    return;
  }
  lv_label_set_text(label, labelText(text).c_str());
}

void DisplayManager::updateChip(ChipUi &chip, const String &text, uint16_t fg, uint16_t bg) {
  if (!chip.container || !chip.label) {
    return;
  }
  lv_obj_set_style_bg_color(chip.container, colorFrom565(bg), 0);
  lv_label_set_text(chip.label, labelText(chipSymbol(text)).c_str());
  lv_obj_set_style_text_color(chip.label, colorFrom565(fg), 0);
}

void DisplayManager::renderScreen(Mode mode, const String &title, const String &message, const String &detail,
                                  const String &footer, const String &leftText, const String &middleText,
                                  const String &rightText, uint16_t accentColor) {
  if (!uiReady) {
    initUi();
  }

  if (sameState(mode, title, message, detail, footer, leftText, middleText, rightText, accentColor)) {
    return;
  }

  currentMode = mode;
  currentTitle = title;
  currentMessage = message;
  currentDetail = detail;
  currentFooter = footer;
  currentLeftChip = leftText;
  currentMiddleChip = middleText;
  currentRightChip = rightText;
  currentAccent = accentColor;

  if (titleLabel) {
    lv_label_set_text(titleLabel, labelText(deviceTitle.isEmpty() ? String("ATOMS3") : deviceTitle).c_str());
  }

  lv_obj_set_style_bg_color(screen, colorFrom565(BG), 0);
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(accentStrip, colorFrom565(accentColor), 0);

  if (mode == Mode::Connected) {
    updateLabel(stateLabel, message);
  } else if (mode == Mode::Setup) {
    updateLabel(stateLabel, LV_SYMBOL_WIFI);
  } else if (mode == Mode::Connecting) {
    updateLabel(stateLabel, LV_SYMBOL_REFRESH);
  } else if (mode == Mode::Error) {
    updateLabel(stateLabel, LV_SYMBOL_CLOSE);
  } else {
    updateLabel(stateLabel, message);
  }
  if (mode == Mode::Connected) {
    updateLabel(detailLabel, "IP " + detail);
  } else {
    updateLabel(detailLabel, detail);
  }
  updateLabel(footerLabel, footer);

  if (mode == Mode::Connected) {
    updateChip(this->leftChip, "AP", CHIP_TEXT, CHIPS_BG);
    updateChip(this->middleChip, "LAN", CHIP_TEXT, CHIPS_ALT);
    updateChip(this->rightChip, "READY", CHIP_TEXT, accentColor);
  } else if (mode == Mode::Setup) {
    updateChip(this->leftChip, "AP", CHIP_TEXT, CHIPS_BG);
    updateChip(this->middleChip, "SETUP", CHIP_TEXT, accentColor);
    updateChip(this->rightChip, "WAIT", CHIP_TEXT, CHIPS_ALT);
  } else if (mode == Mode::Error) {
    updateChip(this->leftChip, "ERR", CHIP_TEXT, CHIPS_BG);
    updateChip(this->middleChip, "STOP", CHIP_TEXT, accentColor);
    updateChip(this->rightChip, "ERR", CHIP_TEXT, CHIPS_ALT);
  } else {
    updateChip(this->leftChip, "BOOT", CHIP_TEXT, CHIPS_BG);
    updateChip(this->middleChip, "WAIT", CHIP_TEXT, CHIPS_ALT);
    updateChip(this->rightChip, "M5", CHIP_TEXT, CHIPS_SOFT);
  }
}
