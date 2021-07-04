#include "ui.h"
#include "utils.h"

namespace {
const RGB defaultDisplayColor = RGB{188,0,166};
const RGB defaultKnobColor = RGB{40,40,40};
}

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

  // Handle drawing
  if (_context._requestedDraw) {
    _context._requestedDraw = false;
    beginDraw();
    topScene().draw(_context, _canvas);
    endDraw();
  }
}

void Stage::beginDraw() {
  _canvas.gfx().setFont(u8g2_font_miranda_nbp_tr);
  _canvas.gfx().clearBuffer();
  _canvas.gfx().home();
  _canvas.setDisplayColor(defaultDisplayColor);
  _canvas.setKnobColor(defaultKnobColor);
}

void Stage::endDraw() {
  _canvas.gfx().sendBuffer();
  _canvas.applyColors();
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

void SelfTest::draw(Context& context, Canvas& canvas) {
  canvas.gfx().drawStr(0, 10, "Hello world!");
  canvas.gfx().drawStr(0, 30, _message.c_str());

  canvas.setDisplayColor(colorWheel(_values[0])* 0.8f);
  canvas.setKnobColor(colorWheel(_values[1]) * 0.4f);
}

bool SelfTest::input(Context& context, const InputEvent& event) {
  switch (event.type) {
    case InputType::LONG_PRESS:
      _message = "LONG_PRESS";
      break;
    case InputType::SINGLE_CLICK:
      _message = "SINGLE_CLICK";
      _index = _index ? 0 : 1;
      break;
    case InputType::DOUBLE_CLICK:
      _message = "DOUBLE_CLICK";
      break;
    case InputType::ROTATE:
      _message = "ROTATE: " + String(event.value);
      _values[_index] += uint8_t(event.value * 5);
      break;
    case InputType::BACK:
      _message = "BACK";
      break;
    case InputType::HOME:
      _message = "HOME";
      break;
    default:
      break;
  }

  context.requestDraw();
  Serial.println(_message);
  return false;
}
