#pragma once
#include <stdint.h>
#include <math.h>
#include <array>
#include <functional>
#include "IFanHardware.h"


class Fan {
public:
  enum VentilationMode {
    HeatRecovery = 0,
    SupplyAir = 1,
    ExhaustAir = 2,
  };

  enum OperatingMode {
    Off = 0,
    Manual = 1,
    Automatic = 2,
    FullControl = 3,
  };

  enum ControlMode {
    Threshold = 0,
    Adaptive = 1,
  };

  enum HumiditySensorMode {
    Relative = 0,
    Absolute = 1,
  };

  struct FanState {
    int16_t speed = 0;
    VentilationMode ventilationMode = VentilationMode::HeatRecovery;
  };

  enum VentilationModeTarget {
    VentilationModeTarget_Manual = 0,
    VentilationModeTarget_Automatic = 1,
  };


  virtual ~Fan() = default;

  Fan(IFanHardware& hw);
  virtual void setVentilationMode(VentilationMode ventilationMode, VentilationModeTarget target = VentilationModeTarget_Manual);
  virtual void setOperatingMode(OperatingMode operatingMode);
  virtual void setControlMode(ControlMode controlMode);
  void setFanSpeed(int16_t fanSpeed); // for speed changes from outside
  void setTimer(uint64_t secondsRemaining, std::function<void()> timerCallback);
  void stopTimer();
  void setSpeedChangeCallback(std::function<void(int16_t)> callback);
  FanState saveState();
  void restoreState(FanState state);
  
  bool setInsideHumdity(float insideRelHumidity);
  void setInsideTemperature(float insideTemperature);
  void setOutsideHumidity(float outsideRelHumidity);
  void setOutsideTemperature(float outsideTemperature);
  virtual int16_t getFanSpeed() = 0;

  // Full control mode — bypass step-based control
  virtual void setFullControlPower(bool on) = 0;
  virtual void setFullControlSpeed(uint8_t percent) = 0;
  virtual void setFullControlDirection(uint8_t dir) = 0;
  virtual uint8_t getFullControlSpeed() = 0;
  VentilationMode getVentilationMode();
  static float getDewPoint(float relHumidity, float temperature);

  HumiditySensorMode humiditySensorMode = HumiditySensorMode::Relative;
  float thresholdHumidityOn = 60;
  float thresholdHumidityOff = 60;
  int16_t thresholdSpeed = 4;

protected:
  void changeFanSpeed(int16_t fanSpeed, bool force = false); //used for changes from within base or derived classes
  virtual void changeFanSpeedDelegate(int16_t fanSpeed) = 0; //specific speed change implementation in derived classes
  virtual void updateMode() = 0;
  void updateEnvironment();
  bool outsideAbsHumidityLower();
  void activateAutoMode();
  void deactivateAutoMode();
  
  // Callbacks used by logic
  void onTimeoutTimer();

  IFanHardware& _hw;

  VentilationMode _ventilationMode = VentilationMode::HeatRecovery;
  VentilationMode _ventilationModeManual = VentilationMode::HeatRecovery;
  VentilationMode _ventilationModeAutomatic = VentilationMode::HeatRecovery;
  OperatingMode _operatingMode = OperatingMode::Manual;
  ControlMode _controlMode = ControlMode::Threshold;
  
  bool _autoModeActive = false;
  bool _manualOverrideActive = false;
  const float _controlGain = 0.18; // TODO: determine proper gain value

  float _outsideRelHumidity = 0;
  float _insideRelHumidity = 0;
  float _outsideTemperature = 0;
  float _insideTemperature = 0;

  std::function<void()> _timerCallback;
  std::function<void(int16_t)> _speedChangeCallback;

  FanState _previousState;
};
