#include "MaicoPPB30.h"

using namespace std;

constexpr std::array<int16_t, 6> MaicoPPB30::_FanSteps;

MaicoPPB30::MaicoPPB30(IFanHardware& hw, uint8_t S1_PIN, uint8_t S2_PIN, uint8_t SW_PIN)
    : Fan(hw), _S1_PWM_PIN(S1_PIN), _S2_PWM_PIN(S2_PIN), _SW_PIN(SW_PIN) {
  _hw.init(_S1_PWM_PIN, _S2_PWM_PIN, _SW_PIN);
  
  _fanStep = _FanSteps[0];
  setPWM();
}

void MaicoPPB30::changeFanSpeedDelegate(int16_t fanSpeed) {
  // Clamp speed (0-5)
  if (fanSpeed < 0) fanSpeed = 0;
  if (fanSpeed > 5) fanSpeed = 5;

  _fanStep = _FanSteps[fanSpeed];

  // We need to trigger updateMode, which might depend on base class state (ventilation mode etc)
  updateMode();
}

int16_t MaicoPPB30::getFanSpeed() {
    // Reverse lookup from step to speed index
  int count = 0;
  for (auto &fs : _FanSteps) {
    if (fs == _fanStep)
      return count;
    else
      count++;
  }
  return 0;
}

void MaicoPPB30::onDirectionTimer() {
  _directionS1 *= -1;
  _directionS2 *= -1;
  setPWM();
}

void MaicoPPB30::updateMode() {
  // Full control bypasses normal step-based logic
  if (_operatingMode == OperatingMode::FullControl) {
    return;
  }

  // Access base class protected members
  if (_operatingMode == OperatingMode::Off) {
    _fanStep = _FanSteps[0];
    _hw.setDigital(_SW_PIN, false); // LOW
  }
  else
    _hw.setDigital(_SW_PIN, true); // HIGH

  if (_ventilationMode == VentilationMode::SupplyAir) {
    _directionS1 = -1;
    _directionS2 = -1;
  } else { // HeatRecovery and ExhaustAir modes
    _directionS1 = 1;
    _directionS2 = 1;
  }

  if (_ventilationMode == VentilationMode::HeatRecovery &&
      !_directionTimerActive && _fanStep > _FanSteps[0]) {
    _directionTimerActive = true;
    _hw.startDirectionTimer(heatRecoveryPeriodSeconds * 1000, [this]() {
        this->onDirectionTimer();
    });
  }
  if ((_ventilationMode != VentilationMode::HeatRecovery &&
       _directionTimerActive) ||
      _fanStep == _FanSteps[0]) {
    _directionTimerActive = false;
    _hw.stopDirectionTimer();
  }

  setPWM();
}

int16_t MaicoPPB30::getPWMLevel(int16_t fraction, int16_t base, int16_t resolution) {
  return (fraction * resolution) / base;
}

void MaicoPPB30::setPWM() {
  _hw.setPWM(_S1_PWM_PIN, getPWMLevel(12 + _directionS1 * _fanStep));
  _hw.setPWM(_S2_PWM_PIN, getPWMLevel(12 + _directionS2 * _fanStep));
}

void MaicoPPB30::setFullControlPower(bool on) {
  _hw.setDigital(_SW_PIN, on);
}

void MaicoPPB30::setFullControlSpeed(uint8_t percent) {
  if (percent > 100) percent = 100;
  _fullControlPercent = percent;
  updateFullControl();
}

void MaicoPPB30::setFullControlDirection(uint8_t dir) {
  _fullControlDirection = (dir == 0) ? 1 : -1;
  updateFullControl();
}

uint8_t MaicoPPB30::getFullControlSpeed() {
  return _fullControlPercent;
}

void MaicoPPB30::updateFullControl() {
  int16_t offset = (int16_t)_fullControlPercent * 12 / 100;
  _hw.setPWM(_S1_PWM_PIN, getPWMLevel(12 + _fullControlDirection * offset));
  _hw.setPWM(_S2_PWM_PIN, getPWMLevel(12 + _fullControlDirection * offset));
}
