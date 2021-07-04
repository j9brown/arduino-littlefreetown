#include "ui.h"
#include "utils.h"

namespace {
const RGB DEFAULT_DISPLAY_COLOR = RGB{188,0,166};
const RGB DEFAULT_KNOB_COLOR = RGB{40,40,40};

const uint32_t ACTIVITY_TIMEOUT_MS = 30 * 1000;
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

Stage::~Stage() {}

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
    }
    return;
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
    }
    return;
  } else if (_asleep) {
    return;
  }

  // Handle activity timeouts
  if (millis() - _lastActivityTime >= ACTIVITY_TIMEOUT_MS) {
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

void Menu::draw(Context& context, Canvas& canvas) {
  canvas.gfx().setCursor(0, 20);
  canvas.gfx().print("Menu!");
}

bool Menu::input(Context& context, const InputEvent& event) {
  return false;
}
