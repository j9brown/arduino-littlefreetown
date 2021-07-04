#include <algorithm>
#include <utility>

#include "ui.h"
#include "utils.h"

namespace {
const RGB DEFAULT_DISPLAY_COLOR = RGB{188,0,166};
const RGB DEFAULT_KNOB_COLOR = RGB{40,40,40};

const millis_t ACTIVITY_TIMEOUT = 30 * 1000;
const millis_t POLL_INTERVAL = 20;
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

Stage::Stage(Binding* binding) :
    _binding(binding), _canvas(binding) {}

void Stage::begin(std::unique_ptr<Scene> scene) {
  assert(_stateIndex == -1);
  _context._requestedPush = std::move(scene);
  activity();
}

void Stage::update() {
  // Handle pop and home
  if (_stateIndex > 0 && (_context._requestedPop || _context._requestedHome)) {
    topScene().exit(_context);
    popState();
    _context._requestedPop = false;
    _context._requestedPush = nullptr; // don't honor push from exited scene
    _context.requestDraw();
    _needPoll = true;
    return;
  } else {
    _context._requestedPop = false;
    _context._requestedHome = false;
  }

  // Handle push
  if (_context._requestedPush) {
    pushState(std::move(_context._requestedPush));
    _context._requestedPush = nullptr;
    _context.requestDraw();
    topScene().enter(_context);
    _needPoll = true;
    return;
  }

  // Handle one input event
  InputEvent event = _binding->readInputEvent();
  if (event.type != InputType::NONE) {
    if (_asleep) { // eat input events used to wake
      _context.requestWake();
      return;
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
    return;
  }

  // Handle sleeping
  if (_context._requestedSleep) {
    _context._requestedSleep = false;
    if (!_asleep) {
      Serial.println("Going to sleep");
      _asleep = true;
      _binding->gfx().setPowerSave(true);
      _binding->setColors(RGB{}, RGB{});
      return;
    }
  }

  // Handle waking
  if (_context._requestedWake) {
    _context._requestedWake = false;
    if (_asleep) {
      Serial.println("Waking up");
      _asleep = false;
      _binding->gfx().setPowerSave(false);
      _context.requestDraw();
      activity();
      return;
    }
  }
  
  // Handle polling for changes (may wake)
  _context._frameTime = millis();
  if (_needPoll || _context._frameTime - _lastPollTime >= POLL_INTERVAL) {
    _needPoll = false;
    _lastPollTime = _context._frameTime;
    topScene().poll(_context);
    return;
  }

  // Stop here if asleep.
  if (_asleep) {
    return;
  }

  // Handle activity timeouts
  if (_context._frameTime - _lastActivityTime >= ACTIVITY_TIMEOUT) {
    _context.requestSleep();
    return;
  }

  // Handle drawing
  if (_context._requestedDraw) {
    _context._requestedDraw = false;
    beginDraw();
    topScene().draw(_context, _canvas);
    endDraw();
    return;
  }
}

void Stage::beginDraw() {
  _canvas.gfx().setFont(u8g2_font_miranda_nbp_tr);
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
        _editing = _items[_activeIndex]->select(context);
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
    if (index == _activeIndex) {
      if (_editing) {
        canvas.gfx().drawBox(90, y, displayWidth - 90, lineHeight);
      } else {
        canvas.gfx().drawBox(0, y, displayWidth, lineHeight);
      }
    }
    _items[index]->drawLabel(context, canvas, 1, y);
    _items[index]->drawValue(context, canvas, 93, y);
    index++;
    y += lineHeight;
  }
}

Item::Item(String label) : _label(std::move(label)) {}

void Item::drawLabel(Context& context, Canvas& canvas, uint32_t x, uint32_t y) {
  canvas.gfx().drawStr(x, y, _label.c_str());
}

NavigateItem::NavigateItem(String label, SelectCallback selectCallback) :
    Item(std::move(label)),
    _selectCallback(std::move(selectCallback)) {}

bool NavigateItem::select(Context& context) {
  context.requestPush(_selectCallback());
  return false;
}
