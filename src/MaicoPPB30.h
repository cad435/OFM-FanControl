#pragma once
#include "Fan.h"
#include "IFanHardware.h"

class MaicoPPB30 : public Fan {
public:
  MaicoPPB30(IFanHardware& hw, uint8_t S1_PIN, uint8_t S2_PIN, uint8_t SW_PIN);

  void changeFanSpeedDelegate(int16_t fanSpeed) override;
  int16_t getFanSpeed() override;

  void setFullControlPower(bool on) override;
  void setFullControlSpeed(uint8_t percent) override;
  void setFullControlDirection(uint8_t dir) override;
  uint8_t getFullControlSpeed() override;

protected:
  void updateMode() override;

private:
  void setPWM();
  void onDirectionTimer();
  static int16_t getPWMLevel(int16_t fraction, int16_t base = 24, int16_t resolution = 1024);

  const uint8_t _S1_PWM_PIN;
  const uint8_t _S2_PWM_PIN;
  const uint8_t _SW_PIN;

  const int8_t heatRecoveryPeriodSeconds = 60;
  static constexpr std::array<int16_t, 6> _FanSteps = {0, 4, 6, 8, 9, 10};

  int16_t _fanStep = 0;
  int16_t _directionS1 = 1;
  int16_t _directionS2 = 1;

  bool _directionTimerActive = false;

  // Full control state
  uint8_t _fullControlPercent = 0;
  int8_t _fullControlDirection = 1; // 1=Zuluft(A), -1=Abluft(B)
  void updateFullControl();
};
