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

#ifndef _SPECIAL_FUNCTIONS_H
#define _SPECIAL_FUNCTIONS_H

#include "tabsgroup.h"

struct CustomFunctionData;

class SpecialFunctionsPage: public PageTab {
  public:
    SpecialFunctionsPage(CustomFunctionData * functions);

    void build(FormWindow * window) override
    {
      build(window, 0);
    }

  protected:
    CustomFunctionData * functions;
    void build(FormWindow * window, int8_t focusSpecialFunctionIndex);
    void rebuild(FormWindow * window, int8_t focusSpecialFunctionIndex);
    void editSpecialFunction(FormWindow * window, uint8_t index);
};

#endif //_SPECIAL_FUNCTIONS_H
