/**
 * @file boards/zxbr/board_configuration.cpp
 *
 * @brief Configuration defaults for the 48way STM32 board
 *
 * @author Matthew Howard,  2022
 */

#include "pch.h"

static void setInjectorPins() {
	engineConfiguration->injectionPinMode = OM_DEFAULT;

	engineConfiguration->injectionPins[0] = Gpio::E14;
	engineConfiguration->injectionPins[1] = Gpio::E13;
}

static void setIgnitionPins() {
	engineConfiguration->ignitionPinMode = OM_DEFAULT;

	engineConfiguration->ignitionPins[0] = Gpio::D12;
	engineConfiguration->ignitionPins[1] = Gpio::D13;
}


void setSdCardConfigurationOverrides(void) {
}

static void setEtbConfig() {

}

static void setupVbatt() {
	// 5.6k high side/10k low side = 1.56 ratio divider
	engineConfiguration->analogInputDividerCoefficient = 1.56f;
	
	// 6.34k high side/ 1k low side
	engineConfiguration->vbattDividerCoeff = (6.34 + 1) / 1; 

	// Battery sense on PA7
	engineConfiguration->vbattAdcChannel = EFI_ADC_0;

	engineConfiguration->adcVcc = 3.3f;
}

static void setStepperConfig() {
	engineConfiguration->idle.stepperDirectionPin = Gpio::Unassigned;
	engineConfiguration->idle.stepperStepPin = Gpio::Unassigned;
	engineConfiguration->stepperEnablePin = Gpio::Unassigned;
}

void setBoardConfigOverrides() {
	setupVbatt();
	//setEtbConfig();
	setStepperConfig();

	// PE3 is error LED, configured in board.mk
	engineConfiguration->communicationLedPin = Gpio::E2;
	engineConfiguration->runningLedPin = Gpio::E4;
	engineConfiguration->warningLedPin = Gpio::E1;

	engineConfiguration->clt.config.bias_resistor = 2490;
	engineConfiguration->iat.config.bias_resistor = 2490;

	//CAN 1 bus overwrites
	engineConfiguration->canRxPin = Gpio::D0;
	engineConfiguration->canTxPin = Gpio::D1;

	//CAN 2 bus overwrites
	engineConfiguration->can2RxPin = Gpio::B12;
	engineConfiguration->can2TxPin = Gpio::B13;
}

static void setupDefaultSensorInputs() {

	engineConfiguration->afr.hwChannel = EFI_ADC_4;
	setEgoSensor(ES_14Point7_Free);
	
	engineConfiguration->baroSensor.hwChannel = EFI_ADC_9;

}

void setBoardDefaultConfiguration(void) {
	setInjectorPins();
	setIgnitionPins();
	engineConfiguration->isSdCardEnabled = false;
	engineConfiguration->canBaudRate = B500KBPS;
	engineConfiguration->can2BaudRate = B500KBPS;
}
