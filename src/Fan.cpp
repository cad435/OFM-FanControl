#include "Fan.h"
#include <sstream>
#include <algorithm>

using namespace std;

Fan::Fan(IFanHardware& hw)
    : _hw(hw) {
}

void Fan::onTimeoutTimer() {
  changeFanSpeed(0, true); // force stop fan
  if(_timerCallback) {
      _timerCallback();
  }
}

void Fan::setOperatingMode(OperatingMode operatingMode) {

  // stop fan when leaving automatic mode
  if(operatingMode != OperatingMode::Automatic
    && _operatingMode == OperatingMode::Automatic
    && !_manualOverrideActive) {
    changeFanSpeed(0);
  }

  // stop fan when leaving full control mode
  if(operatingMode != OperatingMode::FullControl
    && _operatingMode == OperatingMode::FullControl) {
    setFullControlPower(false);
    setFullControlSpeed(0);
  }

  _operatingMode = operatingMode;
  _manualOverrideActive = false; // reset manual override on mode change
  updateMode();
  updateEnvironment();
}

void Fan::setVentilationMode(VentilationMode ventilationMode, VentilationModeTarget target) {
  if(target == VentilationModeTarget_Manual) {
    _ventilationModeManual = ventilationMode;
  } else {
    _ventilationModeAutomatic = ventilationMode;
  }

  if (_autoModeActive) {
    _ventilationMode = _ventilationModeAutomatic;
  } else {
    _ventilationMode = _ventilationModeManual;
  }

  updateMode();
}

Fan::VentilationMode Fan::getVentilationMode() {
  return _ventilationMode;
}

void Fan::setControlMode(ControlMode controlMode) {
  controlMode = controlMode;
  updateEnvironment();
}

void Fan::setFanSpeed(int16_t fanSpeed) {
    _manualOverrideActive = true;
  
  changeFanSpeed(fanSpeed, true); // force speed change
  _previousState = saveState(); // update previous state to current state after manual override
}

void Fan::changeFanSpeed(int16_t fanSpeed, bool force) {
  if (!force && _manualOverrideActive) {
    // manual override active, ignore speed changes except if forced
    return;
  }
  // Delegate to derived class implementation
  changeFanSpeedDelegate(fanSpeed);

  // Notify listener of speed change
  if (_speedChangeCallback) {
    _speedChangeCallback(getFanSpeed());
  }
}

void Fan::setTimer(uint64_t secondsRemaining,
                   std::function<void()> timerCallback) {
  _timerCallback = timerCallback;
  _hw.startOneShotTimer(secondsRemaining * 1000, [this]() {
      this->onTimeoutTimer();
  });
}

void Fan::stopTimer() {
  changeFanSpeed(0, true); // force stop fan
  _hw.stopOneShotTimer();
  _timerCallback = nullptr;
}

void Fan::setSpeedChangeCallback(std::function<void(int16_t)> callback) {
  _speedChangeCallback = callback;
}

bool Fan::setInsideHumdity(float insideRelHumidity) {
  bool thresholdCrossed = false;
  if ((_insideRelHumidity < thresholdHumidityOn &&
      insideRelHumidity >= thresholdHumidityOn) ||
      (_insideRelHumidity >= thresholdHumidityOff &&
      insideRelHumidity < thresholdHumidityOff)) {
    thresholdCrossed = true;
    _manualOverrideActive = false; // reset manual override on threshold crossing
  }
  else
    thresholdCrossed = false;

  _insideRelHumidity = insideRelHumidity;
  updateEnvironment();
  return thresholdCrossed;
}

void Fan::setInsideTemperature(float insideTemperature) {
  _insideTemperature = insideTemperature;
  updateEnvironment();
}

void Fan::setOutsideHumidity(float outsideRelHumidity) {
  _outsideRelHumidity = outsideRelHumidity;
  updateEnvironment();
}

void Fan::setOutsideTemperature(float outsideTemperature) {
  _outsideTemperature = outsideTemperature;
  updateEnvironment();
}

void Fan::updateEnvironment() {
  if (_operatingMode != OperatingMode::Automatic)
    return;

  if (humiditySensorMode == HumiditySensorMode::Absolute &&
      !outsideAbsHumidityLower()){
    // absolute humidity mode and outside humidity not lower -> switch off fan
    changeFanSpeed(0);
    return;
  }

  if(thresholdHumidityOff >= thresholdHumidityOn) {
    // negative hysteresis
    if (_insideRelHumidity >= thresholdHumidityOn) {
      // humidity above threshold -> switch to auto mode
      activateAutoMode();
      return;
    }
    if (_insideRelHumidity < thresholdHumidityOff) {
      // humidity below threshold -> switch to manual mode
      deactivateAutoMode();
      return;
    }
  } else {
    // positive hysteresis
    if (_insideRelHumidity < thresholdHumidityOff) {
      // humidity below threshold -> switch to manual mode
      deactivateAutoMode();
      return;
    }

    if (_insideRelHumidity >= thresholdHumidityOn) {
      // humidity above threshold -> switch to auto mode
      activateAutoMode();
      return;
    }
  }

}

void Fan::activateAutoMode() {
  if(!_autoModeActive) {
    _previousState = saveState();
  }
  _autoModeActive = true;
  if (_controlMode == ControlMode::Threshold) {
    changeFanSpeed(thresholdSpeed);
  } else if (_controlMode == ControlMode::Adaptive) {
    float delta = 0;

    if (humiditySensorMode == HumiditySensorMode::Relative) {
      delta = max(_insideRelHumidity - thresholdHumidityOn, 0.0f);
    } else // humiditySensorMode == HumiditySensorMode::Absolute
    {
      delta = getDewPoint(_insideRelHumidity, _insideTemperature) -
              getDewPoint(_outsideRelHumidity, _outsideTemperature);
      // no hysteresis here yet
    }
    changeFanSpeed(static_cast<int16_t>(floor(_controlGain * delta)));
  }
  setVentilationMode(_ventilationModeAutomatic, VentilationModeTarget_Automatic);
}

void Fan::deactivateAutoMode() {
  _autoModeActive = false;
  restoreState(_previousState);
}

Fan::FanState Fan::saveState() {
  FanState state;
  state.speed = getFanSpeed();
  state.ventilationMode = _ventilationMode;
  return state;
}

void Fan::restoreState(FanState state) {
  changeFanSpeed(state.speed, true);
  setVentilationMode(state.ventilationMode);
}

float Fan::getDewPoint(float relHumidity, float temperature) {
  float a = 17.625;
  float b = 243.04;
  float temp = (a * temperature) / (b + temperature) + log(relHumidity / 100);
  float Td = (b * temp) / (a - temp);
  return Td;
}

bool Fan::outsideAbsHumidityLower() {
  float insideDewPoint = getDewPoint(_insideRelHumidity, _insideTemperature);
  float outsideDewPoint = getDewPoint(_outsideRelHumidity, _outsideTemperature);
  return insideDewPoint > outsideDewPoint;
}