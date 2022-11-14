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

#include "switch_warn_dialog.h"
#include "switches.h"

SwitchWarnDialog::SwitchWarnDialog() :
    FullScreenDialog(WARNING_TYPE_ALERT, STR_SWITCHWARN)
{
  last_bad_switches = 0xff;
  bad_pots = 0;
  last_bad_pots = 0x0;
  setCloseCondition(std::bind(&SwitchWarnDialog::warningInactive, this));

  warn_label = new StaticText(this, rect_t{}, "", 0, COLOR_THEME_PRIMARY1 | FONT(BOLD));

  lv_obj_t* obj = warn_label->getLvObj();
  lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);

  warn_label->setLeft(ALERT_MESSAGE_LEFT);
  warn_label->setWidth(LCD_W - ALERT_MESSAGE_LEFT - PAGE_PADDING);
  warn_label->setTop(ALERT_MESSAGE_TOP);
  warn_label->setHeight(LCD_H - ALERT_MESSAGE_TOP - PAGE_PADDING);
}

bool SwitchWarnDialog::warningInactive()
{
  if (!isSwitchWarningRequired(bad_pots))
    return true;

  if (last_bad_switches != switches_states || last_bad_pots != bad_pots) {
    invalidate();
    if (last_bad_switches == 0xff || last_bad_pots & 0x7ff) {
      AUDIO_ERROR_MESSAGE(AU_SWITCH_ALERT);
    }
  }

  last_bad_pots = bad_pots;
  last_bad_switches = switches_states;

  return false;
}

void SwitchWarnDialog::paint(BitmapBuffer * dc)
{
  if (!running) return;
  FullScreenDialog::paint(dc);
}

void SwitchWarnDialog::checkEvents()
{
  if (!running) return;

  FullScreenDialog::checkEvents();
  if (deleted()) return;

  std::string warn_txt;
  swarnstate_t states = g_model.switchWarningState;
  for (int i = 0; i < NUM_SWITCHES; ++i) {
    if (SWITCH_WARNING_ALLOWED(i)) {
      swarnstate_t mask = ((swarnstate_t)0x07 << (i*3));
      if (states & mask) {
        if ((switches_states & mask) != (states & mask)) {
          swarnstate_t state = (states >> (i*3)) & 0x07;
          warn_txt += getSwitchPositionName(SWSRC_FIRST_SWITCH + i * 3 + state - 1);
        }
      }
    }
  }

  if (g_model.potsWarnMode) {
    if (!warn_txt.empty()) { warn_txt += '\n'; }
    for (int i = 0; i < NUM_POTS + NUM_SLIDERS; i++) {
      if (!IS_POT_SLIDER_AVAILABLE(POT1 + i)) { continue; }
      if ( (g_model.potsWarnEnabled & (1 << i))) {
        if (abs(g_model.potsWarnPosition[i] - GET_LOWRES_POT_POSITION(i)) > 1) {
          warn_txt += STR_VSRCRAW[POT1 + i + 1];
        }
      }
    }
  }

  warn_label->setText(warn_txt);
}