#include "FanChannel.h"

#include "Fan.h"
#include "OpenKNX.h"

FanChannel::FanChannel(uint8_t iChannelNumber, Fan& fan):
_fan(fan)
{
    _channelIndex = iChannelNumber;
}

const std::string FanChannel::name()
{
    return "FanChannel" + _channelIndex;
}

void FanChannel::setup(bool configured)
{
    if (!configured)
        return;
    
    setOpMode(ParamFAN_CH_OpMode);
    setVentilationMode(ParamFAN_CH_VentMode);
    setVentilationMode(ParamFAN_CH_VentModeAutomatic, Fan::VentilationModeTarget_Automatic);
    setControlMode(ParamFAN_CH_ControlMode);
    setHumiditySensorMode(ParamFAN_CH_HumSensMode);
    _fan.thresholdHumidityOn = ParamFAN_CH_ThresholdHumidityOn;
    _fan.thresholdHumidityOff = ParamFAN_CH_ThresholdHumidityOff;
    _fan.thresholdSpeed = ParamFAN_CH_ThresholdSpeed;
    
    // Set up callback to update KO feedback when fan speed changes
    _fan.setSpeedChangeCallback([this](int16_t newSpeed) {
        KoFAN_CH_LevelFeedback.value(newSpeed, DPT_Value_1_Ucount);
    });

}

void FanChannel::resetFan()
{
    _fan.setFanSpeed(0);
}

int16_t FanChannel::getFanSpeed()
{
    return _fan.getFanSpeed();
}


void FanChannel::setOpMode(uint8_t opModeIdx)
{   
    switch (opModeIdx)
    {
    case 0:
        _fan.setOperatingMode(Fan::OperatingMode::Off);
        break;
    case 1:
        _fan.setOperatingMode(Fan::OperatingMode::Manual);
        break;
    case 2:
        _fan.setOperatingMode(Fan::OperatingMode::Automatic);
        break;
    case 4:
        _fan.setOperatingMode(Fan::OperatingMode::FullControl);
        break;
    default:
        break;
    }
}

void FanChannel::setVentilationMode(uint8_t ventilationModeIdx, Fan::VentilationModeTarget target)
{
    switch (ventilationModeIdx)
    {
    case 0: 
        _fan.setVentilationMode(Fan::VentilationMode::HeatRecovery, target);
        break;
    case 1:
        _fan.setVentilationMode(Fan::VentilationMode::SupplyAir, target);
        break;
    case 2:
        _fan.setVentilationMode(Fan::VentilationMode::ExhaustAir, target);
        break;
    default:
        break;
    }
}

void FanChannel::setControlMode(uint8_t controlModeIdx)
{
    switch (controlModeIdx)
    {
    case 0:
        _fan.setControlMode(Fan::ControlMode::Threshold);
        break;
    case 1:
        _fan.setControlMode(Fan::ControlMode::Adaptive);
        break;
    default:
        break;
    }
}

void FanChannel::setHumiditySensorMode(uint8_t humiditySensorModeIdx)
{
    switch (humiditySensorModeIdx)
    {
    case 0:
        _fan.humiditySensorMode = Fan::HumiditySensorMode::Relative;
        break;
    case 1:
        _fan.humiditySensorMode = Fan::HumiditySensorMode::Absolute;
        break;
    default:
        break;
    }
}


void FanChannel::processInputKo(GroupObject& ko) 
{
    if (!ko.initialized())
        return;
    uint16_t kobj = ko.asap();
    switch (FAN_KoCalcIndex(kobj))
    {
        case FAN_KoCH_Level:
        {
            int8_t speed = ko.value(DPT_Value_1_Ucount);
            _fan.setFanSpeed(speed);
            break;
        }
        case FAN_KoCH_LevelUpDown:
        {
            int8_t updown = ko.value(DPT_Step);
            if(updown == 1)
                _fan.setFanSpeed(_fan.getFanSpeed() + 1);
            else
                _fan.setFanSpeed(_fan.getFanSpeed() - 1);
            break;
        }
        case FAN_KoCH_OpMode:
        {
            if(ParamFAN_CH_OpMode == 3)
            {
                uint8_t opModeIdx = ko.value(DPT_Value_1_Ucount);
                if(opModeIdx == 0)
                    _fan.setOperatingMode(Fan::OperatingMode::Manual);
                else if(opModeIdx == 1)
                    _fan.setOperatingMode(Fan::OperatingMode::Automatic);
                else if(opModeIdx == 2)
                    _fan.setOperatingMode(Fan::OperatingMode::FullControl);
                KoFAN_CH_OpModeFeedback.value(opModeIdx, DPT_Value_1_Ucount);
            }
            break;
        }
        case FAN_KoCH_VentMode:
        {
            if(ParamFAN_CH_VentMode == 3)
            {
                uint8_t ventilationModeIdx = ko.value(DPT_Value_1_Ucount);
                setVentilationMode(ventilationModeIdx);
                KoFAN_CH_VentModeFeedback.value(ventilationModeIdx, DPT_Value_1_Ucount);
            }
            break;
        }
        case FAN_KoCH_VentModeAutomatic:
        {
            if(ParamFAN_CH_VentModeAutomatic == 3)
            {
                uint8_t ventilationModeIdx = ko.value(DPT_Value_1_Ucount);
                setVentilationMode(ventilationModeIdx, Fan::VentilationModeTarget_Automatic);
                KoFAN_CH_VentModeFeedbackAutomatic.value(ventilationModeIdx, DPT_Value_1_Ucount);
            }
            break;
        }
        case FAN_KoCH_TemperatureInside:
        {
            _fan.setInsideTemperature(ko.value(DPT_Value_Temp));
            break;
        }
        case FAN_KoCH_HumidityInside:
        {
            float humidity = ko.value(DPT_Value_Humidity);
            _fan.setInsideHumdity(humidity);
            break;
        }
        case FAN_KoCH_TemperatureOutside:
        {
            _fan.setOutsideTemperature(ko.value(DPT_Value_Temp));
            break;
        }
        case FAN_KoCH_HumidityOutside:
        {
            _fan.setOutsideHumidity(ko.value(DPT_Value_Humidity));
            break;
        }
        case FAN_KoCH_TimerActivation:
        {
            uint8_t timerenable = ko.value(DPT_Enable);
            if(timerenable){
                int32_t runtime;
                if(ParamFAN_CH_TimerSelection == 0) // Manual
                    runtime = ParamFAN_CH_TimerValue;
                else
                    runtime = ParamFAN_CH_TimerSelection;

                _fan.setTimer(runtime, std::bind(&FanChannel::timerCallback, this));
                int16_t timeractive = 1;
                KoFAN_CH_TimerFeedback.value(timeractive, DPT_State);
            }
            else{
                _fan.stopTimer();
                int16_t timeractive = 0;
                KoFAN_CH_TimerFeedback.value(timeractive, DPT_State);
            }
            break;
        }
        case FAN_KoCH_FullControlPower:
        {
            bool on = ko.value(DPT_Switch);
            _fan.setFullControlPower(on);
            break;
        }
        case FAN_KoCH_FullControlSpeed:
        {
            uint8_t percent = ko.value(DPT_Scaling);
            _fan.setFullControlSpeed(percent);
            KoFAN_CH_FullControlSpeedFeedback.value(percent, DPT_Scaling);
            break;
        }
        case FAN_KoCH_FullControlDirection:
        {
            uint8_t dir = ko.value(DPT_Switch);
            _fan.setFullControlDirection(dir);
            break;
        }
    }
}

void FanChannel::timerCallback()
{
    int16_t timeractive = 0;
    KoFAN_CH_TimerFeedback.value(timeractive, DPT_State);
}