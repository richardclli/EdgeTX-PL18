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

#include "hw_inputs.h"
#include "opentx.h"

#define SET_DIRTY() storageDirty(EE_GENERAL)

struct HWInputEdit : public RadioTextEdit {
  HWInputEdit(Window* parent, char* name, size_t len) :
      RadioTextEdit(parent, rect_t{}, name, len)
  {
    setWidth(LV_DPI_DEF / 2);
  }
};

static const lv_coord_t col_two_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(2),
                                         LV_GRID_TEMPLATE_LAST};

static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};

HWSticks::HWSticks(Window* parent) : FormGroup(parent, rect_t{})
{
  FlexGridLayout grid(col_two_dsc, row_dsc, 2);
  setFlexLayout();

  for (int i = 0; i < NUM_STICKS; i++) {
    auto line = newLine(&grid);
    new StaticText(line, rect_t{}, STR_VSRCRAW[i + 1], 0, COLOR_THEME_PRIMARY1);
    new HWInputEdit(line, g_eeGeneral.anaNames[i], LEN_ANA_NAME);
  }

#if defined(STICK_DEAD_ZONE)
  auto line = newLine(&grid);
  new StaticText(line, rect_t{}, STR_DEAD_ZONE, 0, COLOR_THEME_PRIMARY1);
  auto dz = new Choice(line, rect_t{}, 0, 7,
                       GET_SET_DEFAULT(g_eeGeneral.stickDeadZone));
  dz->setTextHandler([](uint8_t value) {
    return std::to_string(value ? 2 << (value - 1) : 0);
  });
#endif
}

HWPots::HWPots(Window* parent) : FormGroup(parent, rect_t{})
{
  FlexGridLayout grid(col_two_dsc, row_dsc, 2);
  setFlexLayout();

  for (int i = 0; i < NUM_POTS; i++) {
    // Display EX3 & EX4 (= last two pots) only when FlySky gimbals are present
#if !defined(SIMU) && defined(RADIO_FAMILY_T16)
    if (!globalData.flyskygimbals && (i >= (NUM_POTS - 2))) continue;
#endif
    auto line = newLine(&grid);
    new StaticText(line, rect_t{}, STR_VSRCRAW[i + NUM_STICKS + 1], 0,
                   COLOR_THEME_PRIMARY1);

    auto box = new FormGroup(line, rect_t{});
    box->setFlexLayout(LV_FLEX_FLOW_ROW, lv_dpx(4));

    auto box_obj = box->getLvObj();
    lv_obj_set_style_flex_cross_place(box_obj, LV_FLEX_ALIGN_CENTER, 0);

    new HWInputEdit(box, g_eeGeneral.anaNames[i + NUM_STICKS], LEN_ANA_NAME);
    new Choice(
        box, rect_t{}, STR_POTTYPES, POT_NONE, POT_WITHOUT_DETENT,
        [=]() -> int {
          return bfGet<uint32_t>(g_eeGeneral.potsConfig, 2 * i, 2);
        },
        [=](int newValue) {
          g_eeGeneral.potsConfig =
              bfSet<uint32_t>(g_eeGeneral.potsConfig, newValue, 2 * i, 2);
          SET_DIRTY();
        });
  }
}

HWSliders::HWSliders(Window* parent) : FormGroup(parent, rect_t{})
{
  FlexGridLayout grid(col_two_dsc, row_dsc, 2);
  setFlexLayout();

#if (NUM_SLIDERS > 0)
  for (int i = 0; i < NUM_SLIDERS; i++) {
    const int idx = i + NUM_STICKS + NUM_POTS;

    auto line = newLine(&grid);
    new StaticText(line, rect_t{}, STR_VSRCRAW[idx + 1], 0,
                   COLOR_THEME_PRIMARY1);

    auto box = new FormGroup(line, rect_t{});
    box->setFlexLayout(LV_FLEX_FLOW_ROW, lv_dpx(4));

    auto box_obj = box->getLvObj();
    lv_obj_set_style_flex_cross_place(box_obj, LV_FLEX_ALIGN_CENTER, 0);

    new HWInputEdit(box, g_eeGeneral.anaNames[idx], LEN_ANA_NAME);
    new Choice(
        box, rect_t{}, STR_SLIDERTYPES, SLIDER_NONE, SLIDER_WITH_DETENT,
        [=]() -> int {
          uint8_t mask = (0x01 << i);
          return (g_eeGeneral.slidersConfig & mask) >> i;
        },
        [=](int newValue) {
          uint8_t mask = (0x01 << i);
          g_eeGeneral.slidersConfig &= ~mask;
          g_eeGeneral.slidersConfig |= (newValue << i);
          SET_DIRTY();
        });
  }
#endif
}

class SwitchDynamicLabel : public StaticText
{
 public:
  SwitchDynamicLabel(Window* parent, uint8_t index) :
      StaticText(parent, rect_t{}, "", 0, COLOR_THEME_PRIMARY1),
      index(index)
  {
    checkEvents();
  }

  std::string label()
  {
    std::string str(STR_VSRCRAW[index + MIXSRC_FIRST_SWITCH - MIXSRC_Rud + 1]);
    return str + getSwitchPositionSymbol(lastpos);
  }

  uint8_t position()
  {
    auto value = getValue(MIXSRC_FIRST_SWITCH + index);
    if (value > 0)
      return 2;
    else if (value < 0)
      return 0;
    else
      return 1;
  }

  void checkEvents() override
  {
    uint8_t newpos = position();
    if (newpos != lastpos) {
      lastpos = newpos;
      setText(label());
    }
  }

 protected:
  uint8_t index;
  uint8_t lastpos = 0xff;
};

#if defined(PCBHORUS)
#define SWITCH_TYPE_MAX(sw)                  \
  ((MIXSRC_SF - MIXSRC_FIRST_SWITCH == sw || \
    MIXSRC_SH - MIXSRC_FIRST_SWITCH == sw)   \
       ? SWITCH_2POS                         \
       : SWITCH_3POS)
#else
#define SWITCH_TYPE_MAX(sw) (SWITCH_3POS)
#endif

HWSwitches::HWSwitches(Window* parent) : FormGroup(parent, rect_t{})
{
  FlexGridLayout grid(col_two_dsc, row_dsc, 2);
  setFlexLayout();

  for (int i = 0; i < NUM_SWITCHES; i++) {
    auto line = newLine(&grid);
    new SwitchDynamicLabel(line, i);

    auto box = new FormGroup(line, rect_t{});
    box->setFlexLayout(LV_FLEX_FLOW_ROW, lv_dpx(4));

    auto box_obj = box->getLvObj();
    lv_obj_set_style_flex_cross_place(box_obj, LV_FLEX_ALIGN_CENTER, 0);

    new HWInputEdit(box, g_eeGeneral.switchNames[i], LEN_SWITCH_NAME);
    new Choice(
        box, rect_t{}, STR_SWTYPES, SWITCH_NONE, SWITCH_TYPE_MAX(i),
        [=]() -> int { return SWITCH_CONFIG(i); },
        [=](int newValue) {
          swconfig_t mask = (swconfig_t)0x03 << (2 * i);
          g_eeGeneral.switchConfig = (g_eeGeneral.switchConfig & ~mask) |
                                     ((swconfig_t(newValue) & 0x03) << (2 * i));
          SET_DIRTY();
        });
  }
}

template <class T>
HWInputDialog<T>::HWInputDialog(const char* title) :
    Dialog(Layer::back(), std::string(), rect_t{})
{
  setCloseWhenClickOutside(true);
  if (title) content->setTitle(title);
  new T(&content->form);
  content->setWidth(LCD_W * 0.8);
  content->updateSize();
}

template struct HWInputDialog<HWSticks>;
template struct HWInputDialog<HWPots>;
template struct HWInputDialog<HWSliders>;
template struct HWInputDialog<HWSwitches>;
