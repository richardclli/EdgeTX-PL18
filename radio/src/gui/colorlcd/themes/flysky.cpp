/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "tabsgroup.h"

const ZoneOption OPTIONS_THEME_DEFAULT[] = {
  { STR_BACKGROUND_COLOR, ZoneOption::Color, OPTION_VALUE_UNSIGNED(COLOR_THEME_PRIMARY2) },
  { STR_MAIN_COLOR, ZoneOption::Color, OPTION_VALUE_UNSIGNED(COLOR_THEME_WARNING) },
  { nullptr, ZoneOption::Bool }
};

class FlyskyTheme: public OpenTxTheme
{
  public:
    FlyskyTheme():
      OpenTxTheme("FlySky", OPTIONS_THEME_DEFAULT)
    {
      loadColors();
    }

    void loadColors() const
    {
      TRACE("Load FlySky theme colors");

      lcdColorTable[DEFAULT_COLOR_INDEX] = RGB(18, 94, 153);

      lcdColorTable[COLOR_THEME_PRIMARY1_INDEX] = RGB(0, 0, 0);
      lcdColorTable[COLOR_THEME_PRIMARY2_INDEX] = RGB(255, 255, 255);
      lcdColorTable[COLOR_THEME_PRIMARY3_INDEX] = RGB(12, 63, 102);
      lcdColorTable[COLOR_THEME_SECONDARY1_INDEX] = RGB(18, 94, 153);
      lcdColorTable[COLOR_THEME_SECONDARY2_INDEX] = RGB(182, 224, 242);
      lcdColorTable[COLOR_THEME_SECONDARY3_INDEX] = RGB(228, 238, 242);
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = RGB(20, 161, 229);
      lcdColorTable[COLOR_THEME_EDIT_INDEX] = RGB(0, 153, 9);
      lcdColorTable[COLOR_THEME_ACTIVE_INDEX] = RGB(255, 222, 0);
      lcdColorTable[COLOR_THEME_WARNING_INDEX] = RGB(224, 0, 0);
      lcdColorTable[COLOR_THEME_DISABLED_INDEX] = RGB(140, 140, 140);
      lcdColorTable[CUSTOM_COLOR_INDEX] = RGB(170, 85, 0);
    }

    void loadMenuIcon(uint8_t index, const char * filename, uint32_t color=COLOR_THEME_SECONDARY1) const
    {
      TRACE("loadMenuIcon %s", getFilePath(filename));

      BitmapBuffer * mask = BitmapBuffer::loadMask(getFilePath(filename));
      if (mask) {
        delete iconMask[index];
        iconMask[index] = mask;

        uint8_t width = mask->width();
        uint8_t height = mask->height();

        if (0) {
          delete menuIconNormal[index];
          menuIconNormal[index] = new BitmapBuffer(BMP_RGB565, width, height);
          if (menuIconNormal[index]) {
            menuIconNormal[index]->clear(COLOR_THEME_FOCUS);
            menuIconNormal[index]->drawMask(0, 0, mask, color);
          }

          delete menuIconSelected[index];
          menuIconSelected[index] = new BitmapBuffer(BMP_RGB565, width, height);
          if (menuIconSelected[index]) {
            menuIconSelected[index]->clear(COLOR_THEME_FOCUS);
            menuIconSelected[index]->drawMask(0, 0, mask, color);
          }
        }
        else {
          delete menuIconNormal[index];
          menuIconNormal[index] = new BitmapBuffer(BMP_RGB565, MENU_HEADER_BUTTON_WIDTH, MENU_HEADER_BUTTON_WIDTH);
          if (menuIconNormal[index]) {
            menuIconNormal[index]->clear(COLOR_THEME_FOCUS);
            menuIconNormal[index]->drawMask((MENU_HEADER_BUTTON_WIDTH-width)/2, (MENU_HEADER_BUTTON_WIDTH-width)/2, mask, color);
          }

          delete menuIconSelected[index];
          menuIconSelected[index] = new BitmapBuffer(BMP_RGB565, MENU_HEADER_BUTTON_WIDTH, MENU_HEADER_BUTTON_WIDTH);
          if (menuIconSelected[index]) {
            if (index < ICON_RADIO)
              menuIconSelected[index]->clear(COLOR_THEME_FOCUS /*HEADER_LOGO_BGCOLOR*/);
            else
              menuIconSelected[index]->clear(COLOR_THEME_FOCUS);
            menuIconSelected[index]->drawMask((MENU_HEADER_BUTTON_WIDTH-width)/2, (MENU_HEADER_BUTTON_WIDTH-height)/2, mask, color);
          }
        }
      }
    }

    void loadIcons() const
    {
#if defined(LOG_TELEMETRY) || defined(WATCHDOG_DISABLED)
      loadMenuIcon(ICON_OPENTX, "mask_opentx_testmode.png", COLOR_THEME_SECONDARY1);
#else
      loadMenuIcon(ICON_OPENTX, "mask_edgetx.png");
#endif
#if defined(HARDWARE_TOUCH)
      loadMenuIcon(ICON_NEXT, "mask_next.png");
      loadMenuIcon(ICON_BACK, "mask_back.png");
#endif
      loadMenuIcon(ICON_RADIO, "mask_menu_radio.png");
      loadMenuIcon(ICON_RADIO_SETUP, "mask_radio_setup.png");
      loadMenuIcon(ICON_RADIO_SD_MANAGER, "mask_radio_sd_browser.png");
      loadMenuIcon(ICON_RADIO_TOOLS, "mask_radio_tools.png");
      loadMenuIcon(ICON_RADIO_GLOBAL_FUNCTIONS, "mask_radio_global_functions.png");
      loadMenuIcon(ICON_RADIO_TRAINER, "mask_radio_trainer.png");
      loadMenuIcon(ICON_RADIO_HARDWARE, "mask_radio_hardware.png");
      loadMenuIcon(ICON_RADIO_CALIBRATION, "mask_radio_calibration.png");
      loadMenuIcon(ICON_RADIO_VERSION, "mask_radio_version.png");
      loadMenuIcon(ICON_MODEL, "mask_menu_model.png");
      loadMenuIcon(ICON_MODEL_SETUP, "mask_model_setup.png");
      loadMenuIcon(ICON_MODEL_HELI, "mask_model_heli.png");
      loadMenuIcon(ICON_MODEL_FLIGHT_MODES, "mask_model_flight_modes.png");
      loadMenuIcon(ICON_MODEL_INPUTS, "mask_model_inputs.png");
      loadMenuIcon(ICON_MODEL_MIXER, "mask_model_mixer.png");
      loadMenuIcon(ICON_MODEL_OUTPUTS, "mask_model_outputs.png");
      loadMenuIcon(ICON_MODEL_CURVES, "mask_model_curves.png");
      loadMenuIcon(ICON_MODEL_GVARS, "mask_model_gvars.png");
      loadMenuIcon(ICON_MODEL_LOGICAL_SWITCHES, "mask_model_logical_switches.png");
      loadMenuIcon(ICON_MODEL_SPECIAL_FUNCTIONS, "mask_model_special_functions.png");
      loadMenuIcon(ICON_MODEL_LUA_SCRIPTS, "mask_model_lua_scripts.png");
      loadMenuIcon(ICON_MODEL_TELEMETRY, "mask_model_telemetry.png");
      loadMenuIcon(ICON_STATS, "mask_menu_stats.png");
      loadMenuIcon(ICON_STATS_THROTTLE_GRAPH, "mask_stats_throttle_graph.png");
      loadMenuIcon(ICON_STATS_TIMERS, "mask_stats_timers.png");
      loadMenuIcon(ICON_STATS_ANALOGS, "mask_stats_analogs.png");
      loadMenuIcon(ICON_STATS_DEBUG, "mask_stats_debug.png");
      loadMenuIcon(ICON_THEME, "mask_menu_theme.png");
      loadMenuIcon(ICON_THEME_SETUP, "mask_theme_setup.png");
      loadMenuIcon(ICON_THEME_VIEW1, "mask_theme_view1.png");
      loadMenuIcon(ICON_THEME_VIEW2, "mask_theme_view2.png");
      loadMenuIcon(ICON_THEME_VIEW3, "mask_theme_view3.png");
      loadMenuIcon(ICON_THEME_VIEW4, "mask_theme_view4.png");
      loadMenuIcon(ICON_THEME_VIEW5, "mask_theme_view5.png");
      loadMenuIcon(ICON_THEME_ADD_VIEW, "mask_theme_add_view.png");
      loadMenuIcon(ICON_MONITOR, "mask_monitor.png");
      loadMenuIcon(ICON_MONITOR_CHANNELS1, "mask_monitor_channels1.png");
      loadMenuIcon(ICON_MONITOR_CHANNELS2, "mask_monitor_channels2.png");
      loadMenuIcon(ICON_MONITOR_CHANNELS3, "mask_monitor_channels3.png");
      loadMenuIcon(ICON_MONITOR_CHANNELS4, "mask_monitor_channels4.png");
      loadMenuIcon(ICON_MONITOR_LOGICAL_SWITCHES, "mask_monitor_logsw.png");

      BitmapBuffer * background = BitmapBuffer::loadMask(getFilePath("mask_currentmenu_bg.png"));
      BitmapBuffer * shadow = BitmapBuffer::loadMask(getFilePath("mask_currentmenu_shadow.png"));
      BitmapBuffer * dot = BitmapBuffer::loadMask(getFilePath("mask_currentmenu_dot.png"));

      if (!currentMenuBackground) {
        currentMenuBackground = new BitmapBuffer(BMP_RGB565, 36, 53);
      }
      if (currentMenuBackground) {
        currentMenuBackground->drawSolidFilledRect(0, 0, currentMenuBackground->width(), MENU_HEADER_HEIGHT, COLOR_THEME_FOCUS);
        currentMenuBackground->drawSolidFilledRect(0, MENU_HEADER_HEIGHT, currentMenuBackground->width(), MENU_TITLE_TOP - MENU_HEADER_HEIGHT, COLOR_THEME_SECONDARY3);
        currentMenuBackground->drawSolidFilledRect(0, MENU_TITLE_TOP, currentMenuBackground->width(), currentMenuBackground->height() - MENU_TITLE_TOP, COLOR_THEME_SECONDARY1);
        currentMenuBackground->drawMask(0, 0, background, COLOR_THEME_FOCUS);
        currentMenuBackground->drawMask(0, 0, shadow, COLOR_THEME_PRIMARY1);
        currentMenuBackground->drawMask(10, 39, dot, COLOR_THEME_SECONDARY1);
      }

      delete topleftBitmap;
      topleftBitmap = BitmapBuffer::loadMaskOnBackground("topleft.png", COLOR_THEME_SECONDARY1, COLOR_THEME_FOCUS);

      delete background;
      delete shadow;
      delete dot;
    }

    void loadThemeBitmaps() const
    {
      // Calibration screen
      delete calibStick;
      calibStick = BitmapBuffer::loadBitmap(getFilePath("stick_pointer.png"));

      delete calibStickBackground;
      calibStickBackground = BitmapBuffer::loadBitmap(getFilePath("stick_background.png"));

      delete calibTrackpBackground;
      calibTrackpBackground = BitmapBuffer::loadBitmap(getFilePath("trackp_background.png"));

      // Model Selection screen
      // delete modelselIconBitmap;
      // modelselIconBitmap = BitmapBuffer::loadMaskOnBackground("modelsel/mask_iconback.png", COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);
      // if (modelselIconBitmap) {
      //   BitmapBuffer * bitmap = BitmapBuffer::loadBitmap(getFilePath("modelsel/icon_default.png"));
      //   modelselIconBitmap->drawBitmap(20, 8, bitmap);
      //   delete bitmap;
      // }

      delete modelselSdFreeBitmap;
      modelselSdFreeBitmap = BitmapBuffer::loadMask(getFilePath("modelsel/mask_sdfree.png"));

      delete modelselModelQtyBitmap;
      modelselModelQtyBitmap = BitmapBuffer::loadMask(getFilePath("modelsel/mask_modelqty.png"));

      delete modelselModelNameBitmap;
      modelselModelNameBitmap = BitmapBuffer::loadMask(getFilePath("modelsel/mask_modelname.png"));

      delete modelselModelMoveBackground;
      modelselModelMoveBackground = BitmapBuffer::loadMask(getFilePath("modelsel/mask_moveback.png"));

      delete modelselModelMoveIcon;
      modelselModelMoveIcon = BitmapBuffer::loadMask(getFilePath("modelsel/mask_moveico.png"));

      delete modelselWizardBackground;
      modelselWizardBackground = BitmapBuffer::loadBitmap(getFilePath("wizard/background.png"));

      // Channels monitor screen
      delete chanMonLockedBitmap;
      chanMonLockedBitmap = BitmapBuffer::loadMaskOnBackground("mask_monitor_lockch.png", COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);

      delete chanMonInvertedBitmap;
      chanMonInvertedBitmap = BitmapBuffer::loadMaskOnBackground("mask_monitor_inver.png", COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);

      // Mixer setup screen
      delete mixerSetupMixerBitmap;
      mixerSetupMixerBitmap = BitmapBuffer::loadMaskOnBackground("mask_sbar_mixer.png", COLOR_THEME_SECONDARY1, COLOR_THEME_FOCUS);

      delete mixerSetupToBitmap;
      mixerSetupToBitmap = BitmapBuffer::loadMaskOnBackground("mask_sbar_to.png", COLOR_THEME_SECONDARY1, COLOR_THEME_FOCUS);

      delete mixerSetupOutputBitmap;
      mixerSetupOutputBitmap = BitmapBuffer::loadMaskOnBackground("mask_sbar_output.png", COLOR_THEME_SECONDARY1, COLOR_THEME_FOCUS);

      delete mixerSetupAddBitmap;
      mixerSetupAddBitmap = BitmapBuffer::loadMaskOnBackground(getFilePath("mask_mplex_add.png"), COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);

      delete mixerSetupMultiBitmap;
      mixerSetupMultiBitmap = BitmapBuffer::loadMaskOnBackground(getFilePath("mask_mplex_multi.png"), COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);

      delete mixerSetupReplaceBitmap;
      mixerSetupReplaceBitmap = BitmapBuffer::loadMaskOnBackground(getFilePath("mask_mplex_replace.png"), COLOR_THEME_SECONDARY1, COLOR_THEME_SECONDARY3);

      delete mixerSetupLabelIcon;
      mixerSetupLabelIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_label.png"));

      // delete mixerSetupCurveIcon;
      // mixerSetupCurveIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_curve.png"));

      delete mixerSetupSwitchIcon;
      mixerSetupSwitchIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_switch.png"));

      // delete mixerSetupFlightmodeIcon;
      // mixerSetupFlightmodeIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_fm.png"));

//      delete mixerSetupSlowIcon;
//      mixerSetupSlowIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_slow.png"));
//
//      delete mixerSetupDelayIcon;
//      mixerSetupDelayIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_delay.png"));
//
//      delete mixerSetupDelaySlowIcon;
//      mixerSetupDelaySlowIcon = BitmapBuffer::loadMask(getFilePath("mask_textline_delayslow.png"));
    }

    void load() const override
    {
      loadColors();
      OpenTxTheme::load();
      if (!backgroundBitmap) {
        backgroundBitmap = BitmapBuffer::loadBitmap(getFilePath("background.png"));
      }
      update();
    }

    virtual void update(bool reload = true) const override
    {
      TRACE("TODO THEME::UPDATE()");

#if 0
      uint32_t color = g_eeGeneral.themeData.options[1].unsignedValue;
      uint32_t bg_color = UNEXPECTED_SHUTDOWN() ? WHITE : g_eeGeneral.themeData.options[0].unsignedValue;

      lcdColorTable[COLOR_THEME_SECONDARY3_INDEX] = bg_color;
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = color;
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = color;
      lcdColorTable[COLOR_THEME_PRIMARY3_INDEX] = color;
      lcdColorTable[COLOR_THEME_SECONDARY1_INDEX] = color;
      lcdColorTable[COLOR_THEME_WARNING_INDEX] = color;
      lcdColorTable[COLOR_THEME_SECONDARY1_INDEX] = color;
      lcdColorTable[COLOR_THEME_PRIMARY3_INDEX] =
          RGB(GET_RED(color)>>1, GET_GREEN(color)>>1, GET_BLUE(color)>>1);
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = color;
      lcdColorTable[COLOR_THEME_SECONDARY1_INDEX] = color;
      #define DARKER(x)     ((x * 70) / 100)
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = RGB(DARKER(GET_RED(color)), DARKER(GET_GREEN(color)), DARKER(GET_BLUE(color)));
      lcdColorTable[COLOR_THEME_SECONDARY1_INDEX] = color;
      lcdColorTable[COLOR_THEME_FOCUS_INDEX] = color;
#endif
      loadIcons();
      loadThemeBitmaps();
    }

    void drawBackground(BitmapBuffer * dc) const override
    {
      if (backgroundBitmap) {
        dc->drawBitmap(0, 0, backgroundBitmap);
      }
      else {
        lcdSetColor(g_eeGeneral.themeData.options[0].value.unsignedValue);
        dc->drawSolidFilledRect(0, 0, LCD_W, LCD_H, CUSTOM_COLOR);
      }
    }

    void drawTopLeftBitmap(BitmapBuffer * dc) const override
    {
      if (topleftBitmap) {
        dc->drawBitmap(0, 0, topleftBitmap);
        uint16_t width = topleftBitmap->width();
        dc->drawSolidFilledRect(width, 0, LCD_W - width, MENU_HEADER_HEIGHT, COLOR_THEME_PRIMARY2);
      }
    }

    void drawPageHeaderBackground(BitmapBuffer *dc, uint8_t icon,
                                  const char *title) const override
    {
      //      if (topleftBitmap) {
      //        dc->drawBitmap(0, 0, topleftBitmap);
      //        uint16_t width = topleftBitmap->width();
      //        dc->drawSolidFilledRect(width, 0, LCD_W-width,
      //        MENU_HEADER_HEIGHT, COLOR_THEME_FOCUS);
      //      }
      //      else {
      dc->drawSolidFilledRect(0, 0, LCD_W, MENU_HEADER_HEIGHT, COLOR_THEME_FOCUS);
      //      }
      //
      //      if (icon == ICON_OPENTX)
      //        dc->drawBitmap(4, 10, menuIconSelected[ICON_OPENTX]);
      //      else
      //        dc->drawBitmap(5, 7, menuIconSelected[icon]);
      //
      dc->drawSolidFilledRect(0, MENU_HEADER_HEIGHT, LCD_W,
                              MENU_TITLE_TOP - MENU_HEADER_HEIGHT,
                              COLOR_THEME_SECONDARY3);  // the white separation line
      dc->drawSolidFilledRect(0, MENU_TITLE_TOP, LCD_W, MENU_TITLE_HEIGHT,
                              COLOR_THEME_SECONDARY1);  // the title line background
                                               //
      if (title) {
        dc->drawText(MENUS_MARGIN_LEFT, MENU_TITLE_TOP + 1, title, COLOR_THEME_SECONDARY1);
      }
    }

    const BitmapBuffer * getIconMask(uint8_t index) const override
    {
      return iconMask[index];
    }

    const BitmapBuffer * getIcon(uint8_t index, IconState state) const override
    {
      return state == STATE_DEFAULT ? menuIconNormal[index] : menuIconSelected[index];
    }

    void drawCurrentMenuBackground(BitmapBuffer *dc) const override
    {
//      uint8_t padding_left = 4;
//
//      dc->drawSolidFilledRect(0, 0, 4, MENU_HEADER_BUTTON_WIDTH,
//                              COLOR_THEME_FOCUS);
//      for (unsigned i = 0; i < tabs.size(); i++) {
//        dc->drawBitmap(
//            padding_left + i * MENU_HEADER_BUTTON_WIDTH, 0,
//            theme->getIcon(tabs[i]->getIcon(),
//                           currentIndex == i ? STATE_PRESSED : STATE_DEFAULT));
//      }
    }

    void drawMenuDatetime(BitmapBuffer * dc) const
    {
      dc->drawSolidVerticalLine(DATETIME_SEPARATOR_X, 7, 31, COLOR_THEME_PRIMARY2);

      const TimerOptions timerOptions = {.options = SHOW_TIME};
      struct gtm t;
      gettime(&t);
      char str[10];

#if defined(TRANSLATIONS_CN) || defined(TRANSLATIONS_TW)
      sprintf(str, "%d" TR_MONTH "%d", t.tm_mon + 1, t.tm_mday);
#else
      const char * const STR_MONTHS[] = TR_MONTHS;
      sprintf(str, "%d %s", t.tm_mday, STR_MONTHS[t.tm_mon]);
#endif
      dc->drawText(DATETIME_MIDDLE, DATETIME_LINE1, str, FONT(XS)|COLOR_THEME_PRIMARY2|CENTERED);

      getTimerString(str, getValue(MIXSRC_TX_TIME), timerOptions);
      dc->drawText(DATETIME_MIDDLE, DATETIME_LINE2, str, FONT(XS)|COLOR_THEME_PRIMARY2|CENTERED);
    }

    void drawProgressBar(BitmapBuffer *dc, coord_t x, coord_t y, coord_t w,
                         coord_t h, int value, int total) const override
    {
      dc->drawSolidRect(x, y, w, h, 1, COLOR_THEME_SECONDARY1);
      if (value > 0) {
        int width = (w * value) / total;
        dc->drawSolidFilledRect(x + 2, y + 2, width - 4, h - 4, COLOR_THEME_FOCUS);
      }
    }

  protected:
    static const BitmapBuffer * backgroundBitmap;
    static BitmapBuffer * topleftBitmap;
    static BitmapBuffer * menuIconNormal[MENUS_ICONS_COUNT];
    static BitmapBuffer * menuIconSelected[MENUS_ICONS_COUNT];
    static BitmapBuffer * iconMask[MENUS_ICONS_COUNT];
    static BitmapBuffer * currentMenuBackground;
};

const BitmapBuffer * FlyskyTheme::backgroundBitmap = nullptr;
BitmapBuffer * FlyskyTheme::topleftBitmap = nullptr;
BitmapBuffer * FlyskyTheme::iconMask[MENUS_ICONS_COUNT] = { nullptr };
BitmapBuffer * FlyskyTheme::menuIconNormal[MENUS_ICONS_COUNT] = { nullptr };
BitmapBuffer * FlyskyTheme::menuIconSelected[MENUS_ICONS_COUNT] = { nullptr };
BitmapBuffer * FlyskyTheme::currentMenuBackground = nullptr;

/*FlyskyTheme flyskyTheme;

#if defined(PCBFLYSKY)
OpenTxTheme * defaultTheme = &flyskyTheme;
Theme * theme = &flyskyTheme;
#endif*/
