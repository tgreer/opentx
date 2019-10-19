/*
 * Copyright (C) OpenTX
 *
 * Source:
 *  https://github.com/opentx/libopenui
 *
 * This file is a part of libopenui library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include "tabsgroup.h"
#include "mainwindow.h"
#include "view_main.h"

#if defined(HARDWARE_TOUCH)
#include "keyboard_base.h"
#endif

#include "opentx.h" // TODO for constants...

TabsGroupHeader::TabsGroupHeader(TabsGroup * parent, uint8_t icon):
  FormGroup(parent, { 0, 0, LCD_W, MENU_BODY_TOP }, OPAQUE),
#if defined(HARDWARE_TOUCH)
  back(this, { 0, 0, MENU_HEADER_BUTTON_WIDTH, MENU_HEADER_BUTTON_WIDTH }, ICON_BACK,
       [=]() -> uint8_t {
         parent->deleteLater();
         return 1;
       }, BUTTON_NOFOCUS),
#endif
  icon(icon),
  carousel(this, parent)
{
}

TabsGroupHeader::~TabsGroupHeader()
{
#if defined(HARDWARE_TOUCH)
  back.detach();
#endif
  carousel.detach();
}

void TabsGroupHeader::paint(BitmapBuffer * dc)
{
  static_cast<ThemeBase *>(theme)->drawMenuBackground(dc, icon, title);
}

TabsCarousel::TabsCarousel(Window * parent, TabsGroup * menu):
  Window(parent, { MENU_HEADER_BUTTONS_LEFT, 0, LCD_W - MENU_HEADER_BUTTONS_LEFT, MENU_HEADER_HEIGHT + 10 }, TRANSPARENT),
  menu(menu)
{
}

void TabsCarousel::updateInnerWidth()
{
  setInnerWidth(padding_left + MENU_HEADER_BUTTON_WIDTH * menu->tabs.size());
}

void TabsCarousel::paint(BitmapBuffer * dc)
{
  static_cast<ThemeBase *>(theme)->drawMenuHeader(dc, menu->tabs, currentIndex);
}

#if defined(HARDWARE_TOUCH)
bool TabsCarousel::onTouchEnd(coord_t x, coord_t y)
{
  unsigned index = (x - padding_left) / MENU_HEADER_BUTTON_WIDTH;
  menu->setCurrentTab(index);
  setCurrentIndex(index);
  return true;
}
#endif

TabsGroup::TabsGroup(uint8_t icon):
  Window(&mainWindow, { 0, 0, LCD_W, LCD_H }, OPAQUE),
  header(this, icon),
  body(this, { 0, MENU_BODY_TOP, LCD_W, MENU_BODY_HEIGHT })
{
}

TabsGroup::~TabsGroup()
{
#if defined(HARDWARE_TOUCH)
  Keyboard::hide();
#endif

  for (auto tab: tabs) {
    delete tab;
  }

  body.deleteChildren();
}

void TabsGroup::addTab(PageTab * page)
{
  tabs.push_back(page);
  if (!currentTab) {
    setCurrentTab(0);
  }
  header.carousel.updateInnerWidth();
}

void TabsGroup::removeTab(unsigned index)
{
  if (currentTab == tabs[index]) {
    setCurrentTab(max<unsigned>(0, index - 1));
  }
  tabs.erase(tabs.begin() + index);
  header.carousel.updateInnerWidth();
}

void TabsGroup::removeAllTabs()
{
  for (auto * tab: tabs) {
    delete tab;
  }
  tabs.clear();
  currentTab = nullptr;
  header.carousel.updateInnerWidth();
}

void TabsGroup::setVisibleTab(PageTab * tab)
{
  if (tab != currentTab) {
    clearFocus();
    body.clear();
#if defined(HARDWARE_TOUCH)
    Keyboard::hide();
#endif
    currentTab = tab;
    tab->build(&body);
    if (!focusWindow)
      setFocus();
    header.setTitle(tab->title.c_str());
    invalidate();
  }
}

void TabsGroup::checkEvents()
{
  Window::checkEvents();
  if (currentTab) {
    currentTab->checkEvents();
  }
}

#if defined(HARDWARE_KEYS)
void TabsGroup::onEvent(event_t event)
{
  TRACE_WINDOWS("%s received event 0x%X", getWindowDebugString().c_str(), event);

  if (event == EVT_KEY_BREAK(KEY_PGDN)) {
    uint8_t current = header.carousel.getCurrentIndex() + 1;
    setCurrentTab(current >= tabs.size() ? 0 : current);
  }
  else if (event == EVT_KEY_LONG(KEY_PGDN)) {
    killEvents(event);
    uint8_t current = header.carousel.getCurrentIndex();
    setCurrentTab(current == 0 ? tabs.size() - 1 : current - 1);
  }
  else if (event == EVT_KEY_LONG(KEY_EXIT) || event == EVT_KEY_BREAK(KEY_EXIT)) {
    killEvents(event);
    ViewMain::instance->setFocus();
    deleteLater();
  }
  else if (parent) {
    parent->onEvent(event);
  }
}
#endif

void TabsGroup::paint(BitmapBuffer * dc)
{
  dc->clear(DEFAULT_BGCOLOR);
}

#if defined(HARDWARE_TOUCH)
bool TabsGroup::onTouchEnd(coord_t x, coord_t y)
{
  if (Window::onTouchEnd(x, y))
    return true;

  Keyboard::hide();
  return true;
}
#endif