/*
 * Simple menu-based user interface framework.
 */

#pragma once

#include <memory>
#include <vector>
#include <utility>

#include <Arduino.h>

#include "panel.h"
#include "settings.h"

class Scene;
class Item;
using millis_t = uint32_t;

namespace {
constexpr uint32_t LAYOUT_LABEL_LEFT = 0;
constexpr uint32_t LAYOUT_LABEL_MARGIN = 1;
constexpr uint32_t LAYOUT_VALUE_LEFT = 90;
constexpr uint32_t LAYOUT_VALUE_MARGIN = 3;

constexpr const uint8_t *DEFAULT_FONT = u8g2_font_miranda_nbp_tr;
//constexpr const uint8_t *DEFAULT_FONT = u8g2_font_prospero_nbp_tr;
constexpr const uint8_t *TITLE_FONT = u8g2_font_prospero_bold_nbp_tr;
//constexpr const uint8_t *DEFAULT_FONT = u8g2_font_6x13_tr;
//constexpr const uint8_t *TITLE_FONT = u8g2_font_6x13B_tr;
} // namespace

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

  bool isRequestPending() const {
    return _requestedPop || _requestedHome || _requestedDraw || _requestedSleep || _requestedWake;
  }

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
  using GetActivityTimeoutCallback = millis_t (*)();

  Stage(Binding* binding, GetActivityTimeoutCallback getActivityTimeoutCallback);
  ~Stage() = default;

  void begin(std::unique_ptr<Scene> scene);
  void update();

  bool canSleep() const;

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

  static constexpr ssize_t MAX_STATE_STACK_DEPTH = 5;

  Binding* const _binding;
  Canvas _canvas;
  Context _context;
  GetActivityTimeoutCallback const _getActivityTimeoutCallback;

  State _stateStack[MAX_STATE_STACK_DEPTH];
  ssize_t _stateIndex = -1;
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

  // Called periodically to check for changes and make requests on the context.
  virtual void poll(Context& context) {}

  // Called to draw the item's value at the given position.
  virtual void draw(Context& context, Canvas& canvas, bool active, bool editing,
      uint32_t y, uint32_t width, uint32_t height);

  // Called when the item is clicked while active.
  // Returns true if the item should be edited.
  virtual bool click(Context& context) { return false; }

  // Called to apply a delta to the item's value.
  virtual void edit(Context& context, int32_t delta) {}

private:
  String const _label;
};

class TitleItem : public Item {
public:
  TitleItem(String label);
  ~TitleItem() override = default;

  void draw(Context& context, Canvas& canvas, bool active, bool editing,
      uint32_t y, uint32_t width, uint32_t height) override;
};

class BackItem : public Item {
public:
  BackItem(String label);
  ~BackItem() override = default;

  bool click(Context& context) override;
};

class NavigateItem : public Item {
public:
  using SelectCallback = std::unique_ptr<Scene> (*)();

  NavigateItem(String label, SelectCallback selectCallback);
  ~NavigateItem() override = default;

  bool click(Context& context) override;

private:
  SelectCallback _selectCallback;
};

template <typename T>
class ValueItem : public Item {
public:
  ValueItem(String label);
  ~ValueItem() override = default;

  void poll(Context& context) override;
  void draw(Context& context, Canvas& canvas, bool active, bool editing,
      uint32_t y, uint32_t width, uint32_t height) override;
  bool click(Context& context) override;
  void edit(Context& context, int32_t delta) override;

protected:
  virtual T getValue() = 0;
  virtual void setValue(T value) = 0;
  virtual T addDeltaWithRollover(T value, int32_t delta) = 0;
  virtual void printValue(Print& printer, T value) = 0;

private:
  T _polledValue{};
};

template <typename T>
ValueItem<T>::ValueItem(String label) :
    Item(std::move(label)) {}

template <typename T>
void ValueItem<T>::poll(Context& context) {
  T value = getValue();
  if (value != _polledValue) {
    std::swap(value, _polledValue);
    context.requestDraw();
  }
}

template <typename T>
void ValueItem<T>::draw(Context& context, Canvas& canvas, bool active, bool editing,
    uint32_t y, uint32_t width, uint32_t height) {
  Item::draw(context, canvas, active, editing, y, width, height);     
  canvas.gfx().setCursor(LAYOUT_VALUE_LEFT + LAYOUT_VALUE_MARGIN, y);
  printValue(canvas.gfx(), getValue());
}

template <typename T>
bool ValueItem<T>::click(Context& context) {
  return true;
}

template <typename T>
void ValueItem<T>::edit(Context& context, int32_t delta) {
  T oldValue = getValue();
  T newValue = addDeltaWithRollover(oldValue, delta);
  if (newValue != oldValue) {
    setValue(newValue);
    poll(context);
  }
}

template <typename T>
class NumericItem : public ValueItem<T> {
public:
  using GetCallback = T (*)();
  using SetCallback = void (*)(T);

  NumericItem(String label, GetCallback getCallback, SetCallback setCallback,
      T min, T max, T step);

  template <unsigned addr>
  NumericItem(String label, Setting<T, addr>, T min, T max, T step);
      
  ~NumericItem() override = default;

protected:
  T getValue() override;
  void setValue(T value) override;
  T addDeltaWithRollover(T value, int32_t delta) override;
  void printValue(Print& printer, T value) override;

private:
  const GetCallback _getCallback;
  const SetCallback _setCallback;
  const T _min, _max, _step;
  T _polledValue;
};

template <typename T>
NumericItem<T>::NumericItem(String label, GetCallback getCallback, SetCallback setCallback,
    T min, T max, T step) :
    ValueItem<T>(std::move(label)),
    _getCallback(std::move(getCallback)),
    _setCallback(std::move(setCallback)),
    _min(std::move(min)),
    _max(std::move(max)),
    _step(std::move(step)) {}

template <typename T>
template <unsigned addr>
NumericItem<T>::NumericItem(String label, Setting<T, addr>, T min, T max, T step) :
    NumericItem(std::move(label), Setting<T, addr>::get, Setting<T, addr>::set,
        std::move(min), std::move(max), std::move(step)) {}

template <typename T>
T NumericItem<T>::getValue() {
  return _getCallback();
}

template <typename T>
void NumericItem<T>::setValue(T value) {
  _setCallback(value);
}

template <typename T>
T NumericItem<T>::addDeltaWithRollover(T value, int32_t delta) {
  return ::addDeltaWithRollover<T>(value, _min, _max, _step, delta);
}

template <typename T>
void NumericItem<T>::printValue(Print& printer, T value) {
  printer.print(value);
}

class TintItem : public NumericItem<tint_t> {
public:
  TintItem(String label, GetCallback getCallback, SetCallback setCallback);

  template <unsigned addr>
  TintItem(String label, Setting<tint_t, addr>);
  
  ~TintItem() override = default;

  void draw(Context& context, Canvas& canvas, bool active, bool editing,
      uint32_t y, uint32_t width, uint32_t height) override;

protected:
  void printValue(Print& printer, tint_t value) override;
};

template <unsigned addr>
TintItem::TintItem(String label, Setting<tint_t, addr>) :
    TintItem(label, Setting<tint_t, addr>::get, Setting<tint_t, addr>::set) {}

class BrightnessItem : public NumericItem<brightness_t> {
public:
  BrightnessItem(String label, GetCallback getCallback, SetCallback setCallback);

  template <unsigned addr>
  BrightnessItem(String label, Setting<brightness_t, addr>);

  ~BrightnessItem() override = default;

  void draw(Context& context, Canvas& canvas, bool active, bool editing,
      uint32_t y, uint32_t width, uint32_t height) override;

protected:
  void printValue(Print& printer, brightness_t value) override;
};

template <unsigned addr>
BrightnessItem::BrightnessItem(String label, Setting<brightness_t, addr>) :
    BrightnessItem(label, Setting<brightness_t, addr>::get, Setting<brightness_t, addr>::set) {}

// Traits for ChoiceItem value types.
// Must have the following members:
// - static constexpr T min = first value;
// - static constexpr T max = last value;
// - static const char* toString(T value);
template <typename T>
struct ChoiceTraits;

// A menu item for enumerated values.
// Must define a corresponding ChoiceTraits<T> specialization for each value type.
template <typename T>
class ChoiceItem : public ValueItem<T> {
public:
  using Traits = ChoiceTraits<T>;
  using GetCallback = T (*)();
  using SetCallback = void (*)(T);

  ChoiceItem(String label, GetCallback getCallback, SetCallback setCallback);

  template <unsigned addr>
  ChoiceItem(String label, Setting<T, addr>);
  
  ~ChoiceItem() override = default;

protected:
  T getValue() override;
  void setValue(T value) override;
  T addDeltaWithRollover(T value, int32_t delta) override;
  void printValue(Print& printer, T value) override;

private:
  const GetCallback _getCallback;
  const SetCallback _setCallback;
  T _polledValue;
};

template <typename T>
ChoiceItem<T>::ChoiceItem(String label, GetCallback getCallback, SetCallback setCallback) :
    ValueItem<T>(std::move(label)),
    _getCallback(std::move(getCallback)),
    _setCallback(std::move(setCallback)) {}

template <typename T>
template <unsigned addr>
ChoiceItem<T>::ChoiceItem(String label, Setting<T, addr>) :
    ChoiceItem(std::move(label), Setting<T, addr>::get, Setting<T, addr>::set) {}

template <typename T>
T ChoiceItem<T>::getValue() {
  return _getCallback();
}

template <typename T>
void ChoiceItem<T>::setValue(T value) {
  _setCallback(value);
}

template <typename T>
T ChoiceItem<T>::addDeltaWithRollover(T value, int32_t delta) {
  using U = std::underlying_type_t<T>;
  return T(::addDeltaWithRollover<U>(U(value), U(Traits::min), U(Traits::max), U(1), delta));
}

template <typename T>
void ChoiceItem<T>::printValue(Print& printer, T value) {
  printer.print(Traits::toString(value));
}
