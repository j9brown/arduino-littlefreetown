/*
 * Simple menu-based user interface framework.
 */

#pragma once

#include <memory>

#include <Arduino.h>

#include "panel.h"

class Scene;
class Context;

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

  inline void requestPush(std::unique_ptr<Scene> scene) { _requestedPush = std::move(scene); }
  inline void requestPop() { _requestedPop = true; }
  inline void requestHome() { _requestedHome = true; }
  inline void requestDraw() { _requestedDraw = true; }
  inline void requestSleep() { _requestedSleep = true; _requestedWake = false; }
  inline void requestWake() { _requestedWake = true; _requestedSleep = false; }

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
};

// Drawing interface.
// The canvas is cleared and LED colors are set to defaults prior to each draw call.
class Canvas {
public:
  Canvas(Binding* binding) : _binding(binding) {}

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
  ~Stage();

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
  uint32_t _lastActivityTime = 0;
};

class Scene {
public:
  Scene() {}
  virtual ~Scene() {}

  virtual void enter(Context& context) {}
  virtual void exit(Context& context) {}
  virtual void draw(Context& context, Canvas& canvas) {}
  virtual bool input(Context& context, const InputEvent& event) { return false; }

private:
  Scene(const Scene&) = delete;
  Scene(Scene&&) = delete;  
  Scene& operator=(const Scene&) = delete;
  Scene& operator=(Scene&&) = delete;
};

class Menu : public Scene {
public:
  Menu() {}
  virtual ~Menu() override {}

  void draw(Context& context, Canvas& canvas) override;
  bool input(Context& context, const InputEvent& event) override;

private:
  
};
