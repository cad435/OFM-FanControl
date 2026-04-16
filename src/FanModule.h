#pragma once

#include "MaicoPPB30.h"
#include "FanChannel.h"
#include "OpenKNX.h"
#include "hardware.h"
#include "knxprod.h"
#include "RP2040FanHardware.h"
#include "TachoReader.h"

class FanModule : public OpenKNX::Module {
public:
  void loop() override;
  void setup(bool configured) override;

#ifdef OPENKNX_DUALCORE
  void setup1() override;
  void loop1() override;
#endif

  void processAfterStartupDelay() override;
  void processInputKo(GroupObject &ko) override;
  bool sendReadRequest(GroupObject &ko);

  const std::string name() override;
  const std::string version() override;

private:
  RP2040FanHardware _fan1Hw;
  MaicoPPB30 _fan1 = MaicoPPB30(_fan1Hw, FAN1_S1_PWM_PIN, FAN1_S2_PWM_PIN, FAN1_SW_PIN);

  RP2040FanHardware _fan2Hw;
  MaicoPPB30 _fan2 = MaicoPPB30(_fan2Hw, FAN2_S1_PWM_PIN, FAN2_S2_PWM_PIN, FAN2_SW_PIN);

  FanChannel *_channel[FAN_ChannelCount];
  uint32_t readRequestDelay = 0;

  TachoReader _tacho[FAN_ChannelCount];
  uint32_t _lastRpmUpdate = 0;
  volatile bool _setupComplete = false;
};

// Wir benutzen das, um in main besser auf das Modul zugreifen zu können
extern FanModule openknxFanModule;