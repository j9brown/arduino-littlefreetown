#include <algorithm>
#include <utility>

#include "ui.h"
#include "utils.h"

namespace {
constexpr RGB DEFAULT_DISPLAY_COLOR = RGB{188,0,166};
constexpr RGB DEFAULT_KNOB_COLOR = RGB{40,40,40};

constexpr millis_t POLL_INTERVAL = 20;
constexpr millis_t DRAW_INTERVAL = 20;
} // namespace

InputEvent Binding::readInputEvent() {
  int32_t rotations = _panel->readKnobRotations();
  if (rotations) {
    return InputEvent{ InputType::ROTATE, rotations };
  }

  switch (_panel->readKnobButton()) {
    case Panel::ButtonEvent::LONG_PRESS:
      return InputEvent{ InputType::LONG_PRESS };
    case Panel::ButtonEvent::SINGLE_CLICK:
      return InputEvent{ InputType::SINGLE_CLICK };
    case Panel::ButtonEvent::DOUBLE_CLICK:
      return InputEvent{ InputType::DOUBLE_CLICK };
    default:
      break;
  }

  switch (_panel->readKillButton()) {
    case Panel::ButtonEvent::LONG_PRESS:
      return InputEvent{ InputType::HOME };
    case Panel::ButtonEvent::SINGLE_CLICK:
      return InputEvent{ InputType::BACK };
    default:
      break;
  }
  return InputEvent{ InputType::NONE };
}

Stage::Stage(Binding* binding, GetActivityTimeoutCallback getActivityTimeoutCallback) :
    _binding(binding), _canvas(binding), _getActivityTimeoutCallback(std::move(getActivityTimeoutCallback)) {}

void Stage::begin(std::unique_ptr<Scene> scene) {
  assert(_stateIndex == -1);
  _context._requestedPush = std::move(scene);
  activity();
}

void Stage::update() {
  while (updateOnce());
}

bool Stage::updateOnce() {
  // Handle pop and home
  if (_stateIndex > 0 && (_context._requestedPop || _context._requestedHome)) {
    topScene().exit(_context);
    popState();
    _context._requestedPop = false;
    _context._requestedPush = nullptr; // don't honor push from exited scene
    _context.requestDraw();
    _needPoll = true;
    return true;
  } else {
    _context._requestedHome = false;
    if (_stateIndex == 0 && _context._requestedPop) {
      _context.requestSleep();
    }
    _context._requestedPop = false;
  }

  // Handle push
  if (_context._requestedPush) {
    pushState(std::move(_context._requestedPush));
    _context._requestedPush = nullptr;
    _context.requestDraw();
    topScene().enter(_context);
    _needPoll = true;
    return true;
  }

  // Handle one input event
  InputEvent event = _binding->readInputEvent();
  if (event.type != InputType::NONE) {
    if (_asleep) { // eat input events used to wake
      _context.requestWake();
      return true;
    }

    activity();
    if (!topScene().input(_context, event)) {
      switch (event.type) {
        case InputType::BACK:
          _context.requestPop();
          break;
        case InputType::HOME:
          _context.requestHome();
          break;
        default:
          break;
      }
    }
    return true;
  }

  // Handle sleeping
  if (_context._requestedSleep) {
    _context._requestedSleep = false;
    if (!_asleep) {
      //Serial.println("Going to sleep");
      _asleep = true;
      _binding->gfx().setPowerSave(true);
      _binding->setColors(RGB{}, RGB{});
      return true;
    }
  }

  // Handle waking
  if (_context._requestedWake) {
    _context._requestedWake = false;
    if (_asleep) {
      //Serial.println("Waking up");
      _asleep = false;
      _binding->gfx().setPowerSave(false);
      _context.requestDraw();
      activity();
      return true;
    }
  }

  // Handle polling for changes (may wake)
  _context._frameTime = millis();
  if (_needPoll || _context._frameTime - _lastPollTime >= POLL_INTERVAL) {
    _needPoll = false;
    _lastPollTime = _context._frameTime;
    topScene().poll(_context);
    return true;
  }

  // Stop here if asleep.
  if (_asleep) {
    return false; // nothing left to do while sleeping
  }

  // Handle activity timeouts
  millis_t timeout = _getActivityTimeoutCallback();
  if (timeout > 0 && _context._frameTime - _lastActivityTime >= timeout) {
    _context.requestSleep();
    return true;
  }

  // Handle drawing
  if (_context._requestedDraw && _context._frameTime - _lastDrawTime >= DRAW_INTERVAL) {
    _context._requestedDraw = false;
    beginDraw();
    topScene().draw(_context, _canvas);
    endDraw();
    return true;
  }

  // All done
  return false;
}

bool Stage::canSleep() const {
  return _asleep && _context.canSleep();
}

void Stage::beginDraw() {
  _canvas.gfx().setFont(DEFAULT_FONT);
  _canvas.gfx().clearBuffer();
  _canvas.gfx().home();
  _canvas.gfx().setFontPosTop();
  _canvas.gfx().setFontMode(1);
  _canvas.gfx().setDrawColor(2);
  _canvas.setDisplayColor(DEFAULT_DISPLAY_COLOR);
  _canvas.setKnobColor(DEFAULT_KNOB_COLOR);
}

void Stage::endDraw() {
  _canvas.gfx().sendBuffer();
  _canvas.applyColors();
}

void Stage::activity() {
  _lastActivityTime = millis();
}

void Stage::pushState(std::unique_ptr<Scene> scene) {
  assert(_stateIndex < MAX_STATE_STACK_DEPTH);
  _stateStack[++_stateIndex].scene = std::move(scene);
}

void Stage::popState() {
  assert(_stateIndex > 0);
  --_stateIndex;
}

void Menu::poll(Context& context) {
  for (const auto& item : _items) {
    item->poll(context);
  }
}

bool Menu::input(Context& context, const InputEvent& event) {
  if (_items.empty()) return false;
  
  switch (event.type) {
    case InputType::SINGLE_CLICK:
      if (_editing) {
        _editing = false;
      } else {
        _editing = _items[_activeIndex]->click(context);
      }
      context.requestDraw();
      return true;
    case InputType::ROTATE:
      if (_editing) {
        _items[_activeIndex]->edit(context, event.value);
        context.requestDraw();
      } else {
        size_t newIndex = std::min(_items.size() - 1,
            size_t(std::max(0L, int32_t(_activeIndex) + event.value)));
        if (_activeIndex != newIndex) {
          _activeIndex = newIndex;
          context.requestDraw();
        }
      }
      return true;
    case InputType::BACK:
    case InputType::LONG_PRESS:
      if (_editing) {
        _editing = false;
        context.requestDraw();
        return true;
      }
      return false;
    default:
      return false;
  }
}

void Menu::draw(Context& context, Canvas& canvas) {
  // Scroll into view
  const uint32_t displayHeight = canvas.gfx().getDisplayHeight();
  const uint32_t displayWidth = canvas.gfx().getDisplayWidth();
  const uint32_t lineHeight = canvas.gfx().getMaxCharHeight();
  const uint32_t maxFullyVisibleLines = displayHeight / lineHeight;
  if (_activeIndex < _scrollTop) {
    _scrollTop = _activeIndex;
  } else {
    size_t scrollBottom = _scrollTop + maxFullyVisibleLines - 1;
    if (_activeIndex > scrollBottom) {
      _scrollTop = _activeIndex + 1 - maxFullyVisibleLines;
    }
  }

  // Draw items
  size_t index = _scrollTop;
  uint32_t y = 0;
  while (index < _items.size() && y < displayHeight) {
    const bool active = index == _activeIndex;
    _items[index]->draw(context, canvas, active, active && _editing,
        y, displayWidth, lineHeight);
    index++;
    y += lineHeight;
  }
}

Item::Item(String label) : _label(std::move(label)) {}

void Item::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  if (editing) {
    canvas.gfx().drawBox(LAYOUT_VALUE_LEFT, y, width - LAYOUT_VALUE_LEFT, height);
  } else if (active) {
    canvas.gfx().drawBox(0, y, width, height);
  }
  canvas.gfx().setCursor(LAYOUT_LABEL_LEFT + LAYOUT_LABEL_MARGIN, y);
  canvas.gfx().print(_label);
}

TitleItem::TitleItem(String label) :
    Item(std::move(label)) {}

bool TitleItem::click(Context& context) {
  context.requestPop();
  return false;
}

void TitleItem::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  canvas.gfx().setFont(TITLE_FONT);
  Item::draw(context, canvas, active, editing, y, width, height);
  canvas.gfx().setFont(u8g2_font_open_iconic_gui_1x_t);
  canvas.gfx().drawStr(118, y + 1, "A");
  canvas.gfx().setFont(DEFAULT_FONT);
}

BackItem::BackItem(String label) :
    Item(std::move(label)) {}

bool BackItem::click(Context& context) {
  context.requestPop();
  return false;
}

NavigateItem::NavigateItem(String label, SelectCallback selectCallback) :
    Item(std::move(label)),
    _selectCallback(std::move(selectCallback)) {}

bool NavigateItem::click(Context& context) {
  context.requestPush(_selectCallback());
  return false;
}

TintItem::TintItem(String label, Editable<tint_t> editable) :
    NumericItem(std::move(label), std::move(editable), TINT_MIN, TINT_MAX, 1) {}

void TintItem::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  NumericItem::draw(context, canvas, active, editing, y, width, height);
  if (active) {
    canvas.setKnobColor(makeKnobColor(getValue(), TONE_MAX, 6));
  }
}

void TintItem::printValue(Print& printer, tint_t value) {
  printTint(printer, value);
}

ToneItem::ToneItem(String label, Editable<tint_t> tint, Editable<tone_t> tone) :
    NumericItem(std::move(label), std::move(tone), TONE_MIN, TONE_MAX, 1),
    _tint(std::move(tint)) {}

bool ToneItem::click(Context& context) {
  return tintHasTone(_tint.get()) && NumericItem::click(context);
}

void ToneItem::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  NumericItem::draw(context, canvas, active, editing, y, width, height);
  tint_t tint = _tint.get();
  if (active && tintHasTone(tint)) {
    canvas.setKnobColor(makeKnobColor(tint, getValue(), 6));
  }
}

void ToneItem::printValue(Print& printer, tone_t value) {
  printTone(printer, _tint.get(), value);
}

BrightnessItem::BrightnessItem(String label, Editable<brightness_t> editable) :
    NumericItem(std::move(label), std::move(editable), BRIGHTNESS_MIN, BRIGHTNESS_MAX, 1) {}

void BrightnessItem::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  NumericItem::draw(context, canvas, active, editing, y, width, height);
  if (active) {
    canvas.setKnobColor(makeKnobColor(TINT_WHITE, TONE_DEFAULT, getValue()));
  }
}

void BrightnessItem::printValue(Print& printer, brightness_t value) {
  printBrightness(printer, value);
}
