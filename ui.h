/*
 * Simple menu-based user interface framework.
 */

#pragma once

#include <memory>
#include <vector>

#include <Arduino.h>

#include "panel.h"

class Scene;
class Item;
using millis_t = uint32_t;

enum class InputType {
  NONE = 0,
  LONG_PRESS,
  SINGLE_CLICK,
  DOUBLE_CLICK,
  ROTATE, // rotation direction and amount in |value|
  BACK,
  HOME
};

struct InputEvent {
  InputType type;
  int32_t value = 0;
};

// Binds the UI to the available hardware.
// Could be abstracted further if needed.
class Binding {
public:
  Binding(Panel* panel) : _panel(panel) {}
  ~Binding() = default;

  // Reads the next input event.
  InputEvent readInputEvent();

  // Gets the display's drawing interface.
  inline U8G2& gfx() { return _panel->gfx(); }

  // Sets the panel's colors.
  inline void setColors(RGB display, RGB knob) {
    _panel->setColors(display, knob);
  }

  // Plays a tone for a given duration.
  inline void playTone(uint32_t freq, uint32_t millis) {
    _panel->playTone(freq, millis);
  }

private:
  Binding(const Binding&) = delete;
  Binding(Binding&&) = delete;  
  Binding& operator=(const Binding&) = delete;
  Binding& operator=(Binding&&) = delete;

  Panel* const _panel;
};

// Context for scene callbacks.
class Context {
public:
  Context() {}
  ~Context() = default;

  inline void requestPush(std::unique_ptr<Scene> scene) { _requestedPush = std::move(scene); }
  inline void requestPop() { _requestedPop = true; }
  inline void requestHome() { _requestedHome = true; }
  inline void requestDraw() { _requestedDraw = true; }
  inline void requestSleep() { _requestedSleep = true; _requestedWake = false; }
  inline void requestWake() { _requestedWake = true; _requestedSleep = false; }

  inline millis_t frameTime() const { return _frameTime; }

private:
  Context(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(const Context&) = delete;
  Context& operator=(Context&&) = delete;

  friend class Stage;

  std::unique_ptr<Scene> _requestedPush = nullptr;
  bool _requestedPop = false;
  bool _requestedHome = false;
  bool _requestedDraw = false;
  bool _requestedSleep = false;
  bool _requestedWake = false;
  millis_t _frameTime = 0;
};

// Drawing interface.
// The canvas is cleared and LED colors are set to defaults prior to each draw call.
class Canvas {
public:
  Canvas(Binding* binding) : _binding(binding) {}
  ~Canvas() = default;

  inline U8G2& gfx() { return _binding->gfx(); }

  inline void setDisplayColor(RGB color) {
    _displayColor = color;
  }

  inline void setKnobColor(RGB color) {
    _knobColor = color;
  }

private:
  Canvas(const Canvas&) = delete;
  Canvas(Canvas&&) = delete;
  Canvas& operator=(const Canvas&) = delete;
  Canvas& operator=(Canvas&&) = delete;

  void applyColors() {
    _binding->setColors(_displayColor, _knobColor);
  }

  friend class Stage;
  Binding* const _binding;
  RGB _displayColor;
  RGB _knobColor;
};

// External interface for managing the UI.
class Stage {
public:
  Stage(Binding* binding);
  ~Stage() = default;

  void begin(std::unique_ptr<Scene> scene);
  void update();

private:
  struct State {
    std::unique_ptr<Scene> scene;
  };

  inline State& topState() { return _stateStack[_stateIndex]; }
  inline Scene& topScene() { return *topState().scene; }

  void pushState(std::unique_ptr<Scene> scene);
  void popState();
  void beginDraw();
  void endDraw();
  void activity();

  static constexpr ssize_t MAX_STATE_STACK_DEPTH = 10;

  Binding* const _binding;
  State _stateStack[MAX_STATE_STACK_DEPTH];
  ssize_t _stateIndex = -1;
  Context _context;
  Canvas _canvas;
  bool _asleep = false;
  millis_t _lastActivityTime = 0;
  millis_t _lastPollTime = 0;
  bool _needPoll = false;
};

class Scene {
public:
  Scene() {}
  virtual ~Scene() = default;

  // Called after the scene is pushed on the stack.
  virtual void enter(Context& context) {}

  // Called before the scene is popped off the stack.
  virtual void exit(Context& context) {}

  // Called to handle an input event.
  // Returns true if handled.
  virtual bool input(Context& context, const InputEvent& event) { return false; }

  // Called periodically to check for changes and make requests on the context.
  virtual void poll(Context& context) {}

  // Called to draw the contents of the scene when not asleep.
  virtual void draw(Context& context, Canvas& canvas) {}

private:
  Scene(const Scene&) = delete;
  Scene(Scene&&) = delete;  
  Scene& operator=(const Scene&) = delete;
  Scene& operator=(Scene&&) = delete;
};

class Menu : public Scene {
public:
  Menu() {}
  virtual ~Menu() override = default;

  inline void addItem(std::unique_ptr<Item> item) {
    _items.push_back(std::move(item));
  }

  void poll(Context& context) override;
  bool input(Context& context, const InputEvent& event) override;
  void draw(Context& context, Canvas& canvas) override;

private:
  std::vector<std::unique_ptr<Item>> _items;
  size_t _scrollTop = 0;
  size_t _activeIndex = 0;
  bool _editing = false;
};

class Item {
public:
  Item(String label);
  virtual ~Item() = default;

  virtual void poll(Context& context) {}
  virtual void drawLabel(Context& context, Canvas& canvas, uint32_t x, uint32_t y);
  virtual void drawValue(Context& context, Canvas& canvas, uint32_t x, uint32_t y) {}
  virtual bool select(Context& context) { return false; }
  virtual void edit(Context& context, int32_t delta) {}

private:
  String const _label;
};

class NavigateItem : public Item {
public:
  using SelectCallback = std::unique_ptr<Scene> (*)();

  NavigateItem(String label, SelectCallback selectCallback);
  ~NavigateItem() override = default;

  bool select(Context& context) override;

private:
  SelectCallback _selectCallback;
};

template <typename T>
class NumberItem : public Item {
public:
  using GetCallback = T (*)();
  using SetCallback = void (*)(T);

  NumberItem(String label, GetCallback getCallback, SetCallback setCallback,
      T min, T max, T step);
  ~NumberItem() override = default;

  void poll(Context& context) override;
  void drawValue(Context& context, Canvas& canvas, uint32_t x, uint32_t y) override;
  bool select(Context& context) override;
  void edit(Context& context, int32_t delta) override;

private:
  const GetCallback _getCallback;
  const SetCallback _setCallback;
  const T _min, _max, _step;
  T _polledValue;
};

template <typename T>
NumberItem<T>::NumberItem(String label, GetCallback getCallback, SetCallback setCallback,
    T min, T max, T step) :
    Item(std::move(label)),
    _getCallback(std::move(getCallback)),
    _setCallback(std::move(setCallback)),
    _min(std::move(min)),
    _max(std::move(max)),
    _step(std::move(step)),
    _polledValue(_getCallback()) {}

template <typename T>
void NumberItem<T>::poll(Context& context) {
  T value = _getCallback();
  if (value != _polledValue) {
    std::swap(value, _polledValue);
    context.requestDraw();
  }
}

template <typename T>
void NumberItem<T>::drawValue(Context& context, Canvas& canvas, uint32_t x, uint32_t y) {
  canvas.gfx().setCursor(x, y);
  canvas.gfx().print(_polledValue);
}

template <typename T>
bool NumberItem<T>::select(Context& context) {
  return true;  
}

template <typename T>
void NumberItem<T>::edit(Context& context, int32_t delta) {
  T newValue;
  if (delta > 0) {
    T adj = _step * uint32_t(delta);
    if (_polledValue < _max - adj) {
      newValue = _polledValue + adj;
    } else {
      newValue = _max;
    }
  } else {
    T adj = _step * uint32_t(-delta);
    if (_polledValue > _min + adj) {
      newValue = _polledValue - adj;
    } else {
      newValue = _min;
    }
  }
  if (newValue != _polledValue) {
    _setCallback(newValue);
    poll(context);
  }
}
