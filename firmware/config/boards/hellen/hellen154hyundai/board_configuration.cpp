/**
 * @file boards/hellen/hellen121nissan/board_configuration.cpp
 *
 *
 * @brief Configuration defaults for the hellen121nissan board
 *
 * See https://rusefi.com/s/hellen121nissan
 *
 * @author andreika <prometheus.pcb@gmail.com>
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#include "pch.h"
#include "custom_engine.h"
#include "hellen_meta.h"

static void setInjectorPins() {
	engineConfiguration->injectionPins[0] = H144_LS_1;
	engineConfiguration->injectionPins[1] = H144_LS_2;
	engineConfiguration->injectionPins[2] = H144_LS_3;
	engineConfiguration->injectionPins[3] = H144_LS_4;


	// Disable remainder
	for (int i = 4; i < MAX_CYLINDER_COUNT;i++) {
		engineConfiguration->injectionPins[i] = Gpio::Unassigned;
	}

	engineConfiguration->injectionPinMode = OM_DEFAULT;
}

static void setIgnitionPins() {
	engineConfiguration->ignitionPins[0] = Gpio::C13;
	engineConfiguration->ignitionPins[1] = Gpio::E5;
	engineConfiguration->ignitionPins[2] = Gpio::E4;
	engineConfiguration->ignitionPins[3] = Gpio::E3;
	
	// disable remainder
	for (int i = 4; i < MAX_CYLINDER_COUNT; i++) {
		engineConfiguration->ignitionPins[i] = Gpio::Unassigned;
	}

	engineConfiguration->ignitionPinMode = OM_DEFAULT;
}

static void setupVbatt() {
	// 4.7k high side/4.7k low side = 2.0 ratio divider
	engineConfiguration->analogInputDividerCoefficient = 2.0f;

	// set vbatt_divider 5.835
	// 33k / 6.8k
	engineConfiguration->vbattDividerCoeff = (33 + 6.8) / 6.8; // 5.835

	// pin input +12 from Main Relay
	engineConfiguration->vbattAdcChannel = EFI_ADC_5; // 4T

	engineConfiguration->adcVcc = 3.29f;
}

static void setupDefaultSensorInputs() {
	// trigger inputs
	engineConfiguration->triggerInputPins[0] = H144_IN_CRANK;
	engineConfiguration->triggerInputPins[1] = Gpio::Unassigned;
	// Direct hall-only cam input
	engineConfiguration->camInputs[0] = H144_IN_CAM;
	engineConfiguration->camInputs[1 * CAMS_PER_BANK] = H144_IN_D_AUX4;

	engineConfiguration->vvtMode[0] = VVT_SECOND_HALF;
	engineConfiguration->vvtMode[1 * CAMS_PER_BANK] = VVT_SECOND_HALF;

    engineConfiguration->vehicleSpeedSensorInputPin = H144_IN_VSS;

	engineConfiguration->tps1_1AdcChannel = H144_IN_TPS;
	engineConfiguration->tps1_2AdcChannel = H144_IN_AUX1;
	engineConfiguration->useETBforIdleControl = true;

	engineConfiguration->throttlePedalUpVoltage = 0.73;
	engineConfiguration->throttlePedalWOTVoltage = 4.0;
	engineConfiguration->throttlePedalSecondaryUpVoltage = 0.34;
	engineConfiguration->throttlePedalSecondaryWOTVoltage = 1.86;

	engineConfiguration->throttlePedalPositionAdcChannel = EFI_ADC_3;
	engineConfiguration->throttlePedalPositionSecondAdcChannel = EFI_ADC_14;
	engineConfiguration->mafAdcChannel = EFI_ADC_NONE;
	engineConfiguration->map.sensor.hwChannel = H144_IN_MAP1;

	engineConfiguration->afr.hwChannel = EFI_ADC_NONE;

	engineConfiguration->clt.adcChannel = H144_IN_CLT;

	engineConfiguration->iat.adcChannel = H144_IN_IAT;

//	engineConfiguration->auxTempSensor1.adcChannel = H144_IN_O2S2;
	engineConfiguration->auxTempSensor2.adcChannel = EFI_ADC_NONE;
}

static bool isFirstInvocation = true;

void setBoardConfigOverrides() {
	setHellen144LedPins();
	setupVbatt();

	setHellenSdCardSpi2();

	engineConfiguration->clt.config.bias_resistor = 4700;
	engineConfiguration->iat.config.bias_resistor = 4700;

	if (engine->engineState.hellenBoardId == -1) {
		// first revision of did not have Hellen Board ID
		// https://github.com/rusefi/hellen154hyundai/issues/55
		engineConfiguration->etbIo[1].directionPin1 = Gpio::Unassigned;
		engineConfiguration->etbIo[1].directionPin2 = Gpio::Unassigned;
		engineConfiguration->etbIo[1].controlPin = Gpio::Unassigned;

		if (isFirstInvocation) {
			isFirstInvocation = false;
			efiSetPadMode("ETB FIX0", H144_OUT_PWM4, PAL_MODE_INPUT_ANALOG);
			efiSetPadMode("ETB FIX1", H144_OUT_PWM5, PAL_MODE_INPUT_ANALOG);
			efiSetPadMode("ETB FIX2", H144_OUT_IO13, PAL_MODE_INPUT_ANALOG);
		}
	}
}

/**
 * @brief   Board-specific configuration defaults.
 *
 * See also setDefaultEngineConfiguration
 *
 * @todo    Add your board-specific code, if any.
 */
void setBoardDefaultConfiguration() {
	setInjectorPins();
	setIgnitionPins();

	engineConfiguration->displayLogicLevelsInEngineSniffer = true;
	engineConfiguration->isSdCardEnabled = true;

	engineConfiguration->enableSoftwareKnock = true;
	engineConfiguration->canNbcType = CAN_BUS_GENESIS_COUPE;

	engineConfiguration->canTxPin = H176_CAN_TX;
	engineConfiguration->canRxPin = H176_CAN_RX;

	engineConfiguration->fuelPumpPin = H144_OUT_IO9;
//	engineConfiguration->idle.solenoidPin = Gpio::D14;	// OUT_PWM5
//	engineConfiguration->fanPin = Gpio::D12;	// OUT_PWM8
	engineConfiguration->mainRelayPin = Gpio::G14;	// pin: 111a, OUT_IO3
	engineConfiguration->malfunctionIndicatorPin = H144_OUT_PWM8;

	engineConfiguration->brakePedalPin = H144_IN_RES3;
	engineConfiguration->clutchUpPin = H144_IN_RES2;
	engineConfiguration->acSwitch = H144_IN_RES1;

	// "required" hardware is done - set some reasonable defaults
	setupDefaultSensorInputs();

	// control pins are inverted since overall ECU pinout seems to be inverted
	engineConfiguration->etbIo[0].directionPin1 = H144_OUT_PWM3;
	engineConfiguration->etbIo[0].directionPin2 = H144_OUT_PWM2;
	engineConfiguration->etbIo[0].controlPin = H144_OUT_IO12;
	engineConfiguration->etb_use_two_wires = true;

	// wastegate DC motor
	engineConfiguration->etbIo[1].directionPin1 = H144_OUT_PWM4;
	engineConfiguration->etbIo[1].directionPin2 = H144_OUT_PWM5;
	engineConfiguration->etbIo[1].controlPin = H144_OUT_IO13;
	engineConfiguration->etb_use_two_wires = true;
	engineConfiguration->etbFunctions[1] = ETB_Wastegate;

	// Some sensible defaults for other options
	setCrankOperationMode();

	engineConfiguration->vvtCamSensorUseRise = true;
	engineConfiguration->useOnlyRisingEdgeForTrigger = true;
	setAlgorithm(LM_SPEED_DENSITY);


	engineConfiguration->injectorCompensationMode = ICM_FixedRailPressure;

	engineConfiguration->specs.cylindersCount = 4;
	engineConfiguration->specs.firingOrder = FO_1_3_4_2;
	engineConfiguration->specs.displacement = 1.998;
	strcpy(engineConfiguration->engineMake, ENGINE_MAKE_Hyundai);
	strcpy(engineConfiguration->engineCode, "Theta II");
	engineConfiguration->globalTriggerAngleOffset = 90;

	engineConfiguration->ignitionMode = IM_INDIVIDUAL_COILS; // IM_WASTED_SPARK
	engineConfiguration->crankingInjectionMode = IM_SIMULTANEOUS;
	engineConfiguration->injectionMode = IM_SIMULTANEOUS;//IM_BATCH;// IM_SEQUENTIAL;

	// very similar to Nissan?
	engineConfiguration->tpsMin = 100;
	engineConfiguration->tpsMax = 914;

	engineConfiguration->tps1SecondaryMin = 880;
	engineConfiguration->tps1SecondaryMax = 68;
}
