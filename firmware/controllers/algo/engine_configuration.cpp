/**
 * @file	engine_configuration.cpp
 * @brief	Utility method related to the engine configuration data structure.
 *
 * @date Nov 22, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 *
 * This file is part of rusEfi - see http://rusefi.com
 *
 * rusEfi is free software; you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * rusEfi is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "pch.h"

#include "os_access.h"
#include "speed_density.h"
#include "advance_map.h"
#include "flash_main.h"

#include "hip9011_logic.h"
#include "bench_test.h"

#if EFI_MEMS
#include "accelerometer.h"
#endif

#include "defaults.h"

#include "bmw_m73.h"
#include "bmw_n73.h"

#include "citroenBerlingoTU3JP.h"
#include "custom_engine.h"
#include "dodge_neon.h"
#include "dodge_ram.h"

#include "engine_template.h"

#include "ford_aspire.h"
#include "ford_1995_inline_6.h"

#include "honda_k_dbc.h"
#include "honda_600.h"
#include "hyundai.h"

#include "GY6_139QMB.h"

#include "nissan_primera.h"
#include "nissan_vq.h"

#include "mazda_miata.h"
#include "mazda_miata_1_6.h"
#include "mazda_miata_na8.h"
#include "mazda_miata_vvt.h"
#include "mazda_626.h"
#include "m111.h"
#include "mercedes.h"
#include "mitsubishi.h"

#include "subaru.h"
#include "test_engine.h"
#include "sachs.h"
#include "vw.h"
#include "vw_b6.h"
#include "chevrolet_camaro_4.h"
#include "toyota_jzs147.h"
#include "ford_festiva.h"
#include "boost_control.h"
#if EFI_IDLE_CONTROL
#include "idle_thread.h"
#endif /* EFI_IDLE_CONTROL */

#if EFI_ALTERNATOR_CONTROL
#include "alternator_controller.h"
#endif

#if EFI_ELECTRONIC_THROTTLE_BODY
#include "electronic_throttle.h"
#endif

#if EFI_HIP_9011
#include "hip9011.h"
#endif

#include "hardware.h"

#if EFI_PROD_CODE
#include "board.h"
#endif /* EFI_PROD_CODE */

#if EFI_EMULATE_POSITION_SENSORS
#include "trigger_emulator_algo.h"
#endif /* EFI_EMULATE_POSITION_SENSORS */

#if EFI_TUNER_STUDIO
#include "tunerstudio.h"
#endif

//#define TS_DEFAULT_SPEED 115200
#define TS_DEFAULT_SPEED 38400

/**
 * Current engine configuration. On firmware start we assign empty configuration, then
 * we copy actual configuration after reading settings.
 * This is useful to compare old and new configurations in order to apply new settings.
 *
 * todo: place this field next to 'engineConfiguration'?
 */
#if EFI_ACTIVE_CONFIGURATION_IN_FLASH
#include "flash_int.h"
engine_configuration_s & activeConfiguration = reinterpret_cast<persistent_config_container_s*>(getFlashAddrFirstCopy())->persistentConfiguration.engineConfiguration;
// we cannot use this activeConfiguration until we call rememberCurrentConfiguration()
bool isActiveConfigurationVoid = true;
#else
static engine_configuration_s activeConfigurationLocalStorage;
engine_configuration_s & activeConfiguration = activeConfigurationLocalStorage;
#endif /* EFI_ACTIVE_CONFIGURATION_IN_FLASH */

void rememberCurrentConfiguration() {
#if ! EFI_ACTIVE_CONFIGURATION_IN_FLASH
	memcpy(&activeConfiguration, engineConfiguration, sizeof(engine_configuration_s));
#else
	isActiveConfigurationVoid = false;
#endif /* EFI_ACTIVE_CONFIGURATION_IN_FLASH */
}

static void wipeString(char *string, int size) {
	// we have to reset bytes after \0 symbol in order to calculate correct tune CRC from MSQ file
	for (int i = strlen(string) + 1; i < size; i++) {
		string[i] = 0;
	}
}

static void wipeStrings() {
	wipeString(engineConfiguration->engineMake, sizeof(vehicle_info_t));
	wipeString(engineConfiguration->engineCode, sizeof(vehicle_info_t));
	wipeString(engineConfiguration->vehicleName, sizeof(vehicle_info_t));
}

void onBurnRequest() {
	wipeStrings();

	incrementGlobalConfigurationVersion();
}

// Weak link a stub so that every board doesn't have to implement this function
__attribute__((weak)) void boardOnConfigurationChange(engine_configuration_s* /*previousConfiguration*/) { }

/**
 * this is the top-level method which should be called in case of any changes to engine configuration
 * online tuning of most values in the maps does not count as configuration change, but 'Burn' command does
 *
 * this method is NOT currently invoked on ECU start - actual user input has to happen!
 * See preCalculate which is invoked BOTH on start and configuration change
 */
void incrementGlobalConfigurationVersion() {
	engine->globalConfigurationVersion++;
#if EFI_DEFAILED_LOGGING
	efiPrintf("set globalConfigurationVersion=%d", globalConfigurationVersion);
#endif /* EFI_DEFAILED_LOGGING */

	applyNewHardwareSettings();

	boardOnConfigurationChange(&activeConfiguration);

/**
 * All these callbacks could be implemented as listeners, but these days I am saving RAM
 */
	engine->preCalculate();
#if EFI_ALTERNATOR_CONTROL
	onConfigurationChangeAlternatorCallback(&activeConfiguration);
#endif /* EFI_ALTERNATOR_CONTROL */

#if EFI_BOOST_CONTROL
	onConfigurationChangeBoostCallback(&activeConfiguration);
#endif
#if EFI_ELECTRONIC_THROTTLE_BODY
	onConfigurationChangeElectronicThrottleCallback(&activeConfiguration);
#endif /* EFI_ELECTRONIC_THROTTLE_BODY */

#if EFI_ENGINE_CONTROL && EFI_PROD_CODE
	onConfigurationChangeBenchTest();
#endif

#if EFI_SHAFT_POSITION_INPUT
	onConfigurationChangeTriggerCallback();
#endif /* EFI_SHAFT_POSITION_INPUT */
#if EFI_EMULATE_POSITION_SENSORS && ! EFI_UNIT_TEST
	onConfigurationChangeRpmEmulatorCallback(&activeConfiguration);
#endif /* EFI_EMULATE_POSITION_SENSORS */

	engine->engineModules.apply_all([](auto & m) {
			m.onConfigurationChange(&activeConfiguration);
		});
	rememberCurrentConfiguration();
}

/**
 * @brief Sets the same dwell time across the whole getRpm() range
 * set dwell X
 */
void setConstantDwell(floatms_t dwellMs) {
	for (int i = 0; i < DWELL_CURVE_SIZE; i++) {
		config->sparkDwellRpmBins[i] = 1000 * i;
	}
	setArrayValues(config->sparkDwellValues, dwellMs);
}

void setWholeIgnitionIatCorr(float value) {
	setTable(config->ignitionIatCorrTable, value);
}

void setFuelTablesLoadBin(float minValue, float maxValue) {
	setLinearCurve(config->injPhaseLoadBins, minValue, maxValue, 1);
	setLinearCurve(config->veLoadBins, minValue, maxValue, 1);
	setLinearCurve(config->lambdaLoadBins, minValue, maxValue, 1);
}

void setWholeIatCorrTimingTable(float value) {
	setTable(config->ignitionIatCorrTable, value);
}

/**
 * See also crankingTimingAngle
 */
void setWholeTimingTable_d(angle_t value) {
	setTable(config->ignitionTable, value);
}

static void initTemperatureCurve(float *bins, float *values, int size, float defaultValue) {
	for (int i = 0; i < size; i++) {
		bins[i] = -40 + i * 10;
		values[i] = defaultValue; // this correction is a multiplier
	}
}

void prepareVoidConfiguration(engine_configuration_s *engineConfiguration) {
	efiAssertVoid(OBD_PCM_Processor_Fault, engineConfiguration != NULL, "ec NULL");
	efi::clear(engineConfiguration);

	engineConfiguration->clutchDownPinMode = PI_PULLUP;
	engineConfiguration->clutchUpPinMode = PI_PULLUP;
	engineConfiguration->brakePedalPinMode = PI_PULLUP;
}

void setDefaultBasePins() {
#ifdef EFI_WARNING_PIN
	engineConfiguration->warningLedPin = EFI_WARNING_PIN;
#else
	engineConfiguration->warningLedPin = Gpio::D13; // orange LED on discovery
#endif


#ifdef EFI_COMMUNICATION_PIN
	engineConfiguration->communicationLedPin = EFI_COMMUNICATION_PIN;
#else
	engineConfiguration->communicationLedPin = Gpio::D15; // blue LED on discovery
#endif
#ifdef EFI_RUNNING_PIN
	engineConfiguration->runningLedPin = EFI_RUNNING_PIN;
#else
	engineConfiguration->runningLedPin = Gpio::D12; // green LED on discovery
#endif

#if EFI_PROD_CODE
	// call overrided board-specific serial configuration setup, if needed (for custom boards only)
	// needed also by bootloader code
	setPinConfigurationOverrides();
#endif /* EFI_PROD_CODE */

	// set UART pads configuration based on the board
// needed also by bootloader code
	engineConfiguration->binarySerialTxPin = Gpio::C10;
	engineConfiguration->binarySerialRxPin = Gpio::C11;
	engineConfiguration->tunerStudioSerialSpeed = TS_DEFAULT_SPEED;
	engineConfiguration->uartConsoleSerialSpeed = 115200;
}

// needed also by bootloader code
// at the moment bootloader does NOT really need SD card, this is a step towards future bootloader SD card usage
void setDefaultSdCardParameters() {
	engineConfiguration->isSdCardEnabled = true;
}

static void setDefaultWarmupIdleCorrection() {
	initTemperatureCurve(CLT_MANUAL_IDLE_CORRECTION, 1.0);

	float baseIdle = 30;

	setCurveValue(CLT_MANUAL_IDLE_CORRECTION, -40, 1.5);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION, -30, 1.5);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION, -20, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION, -10, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,   0, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  10, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  20, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  30, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  40, 40.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  50, 37.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  60, 35.0 / baseIdle);
	setCurveValue(CLT_MANUAL_IDLE_CORRECTION,  70, 33.0 / baseIdle);
}

/**
 * see also setTargetRpmCurve()
 */
static void setDefaultIdleSpeedTarget() {
	copyArray(config->cltIdleRpmBins, {  -30, - 20,  -10,    0,   10,   20,   30,   40,   50,  60,  70,  80,  90, 100 , 110,  120 });
	copyArray(config->cltIdleRpm,     { 1350, 1350, 1300, 1200, 1150, 1100, 1050, 1000, 1000, 950, 950, 930, 900, 900, 1000, 1100 });
}

static void setDefaultFrankensoStepperIdleParameters() {
	engineConfiguration->idle.stepperDirectionPin = Gpio::E10;
	engineConfiguration->idle.stepperStepPin = Gpio::E12;
	engineConfiguration->stepperEnablePin = Gpio::E14;
	engineConfiguration->idleStepperReactionTime = 10;
	engineConfiguration->idleStepperTotalSteps = 150;
}

static void setCanFrankensoDefaults() {
	engineConfiguration->canTxPin = Gpio::B6;
	engineConfiguration->canRxPin = Gpio::B12;
}

/**
 * see also setDefaultIdleSpeedTarget()
 */
void setTargetRpmCurve(int rpm) {
	setLinearCurve(config->cltIdleRpmBins, CLT_CURVE_RANGE_FROM, 140, 10);
	setLinearCurve(config->cltIdleRpm, rpm, rpm, 10);
}

void setDefaultGppwmParameters() {
	// Same config for all channels
	for (size_t i = 0; i < efi::size(engineConfiguration->gppwm); i++) {
		auto& cfg = engineConfiguration->gppwm[i];

		cfg.pin = Gpio::Unassigned;
		cfg.dutyIfError = 0;
		cfg.onAboveDuty = 60;
		cfg.offBelowDuty = 50;
		cfg.pwmFrequency = 250;

		for (size_t j = 0; j < efi::size(cfg.loadBins); j++) {
			uint8_t z = j * 100 / (efi::size(cfg.loadBins) - 1);
			cfg.loadBins[j] = z;

			// Fill some values in the table
			for (size_t k = 0; k < efi::size(cfg.rpmBins); k++) {
				cfg.table[j][k] = z;
			}

		}

		for (size_t j = 0; j < efi::size(cfg.rpmBins); j++) {
			cfg.rpmBins[j] = 1000 * j;
		}
	}
}

static void setDefaultEngineNoiseTable() {
	setRpmTableBin(engineConfiguration->knockNoiseRpmBins, ENGINE_NOISE_CURVE_SIZE);

	engineConfiguration->knockSamplingDuration = 45;

	setArrayValues(engineConfiguration->knockBaseNoise, -20);
}

static void setHip9011FrankensoPinout() {
	/**
	 * SPI on PB13/14/15
	 */
	//	engineConfiguration->hip9011CsPin = Gpio::D0; // rev 0.1

	engineConfiguration->isHip9011Enabled = true;
	engineConfiguration->hip9011PrescalerAndSDO = HIP_8MHZ_PRESCALER; // 8MHz chip
	engineConfiguration->is_enabled_spi_2 = true;
	// todo: convert this to rusEfi, hardware-independent enum
#if EFI_PROD_CODE
#ifdef EFI_HIP_CS_PIN
	engineConfiguration->hip9011CsPin = EFI_HIP_CS_PIN;
#else
	engineConfiguration->hip9011CsPin = Gpio::B0; // rev 0.4
#endif
	engineConfiguration->hip9011CsPinMode = OM_OPENDRAIN;

	engineConfiguration->hip9011IntHoldPin = Gpio::B11;
	engineConfiguration->hip9011IntHoldPinMode = OM_OPENDRAIN;

	engineConfiguration->spi2SckMode = PO_OPENDRAIN; // 4
	engineConfiguration->spi2MosiMode = PO_OPENDRAIN; // 4
	engineConfiguration->spi2MisoMode = PO_PULLUP; // 32
#endif /* EFI_PROD_CODE */

	engineConfiguration->hip9011Gain = 1;

	if (!engineConfiguration->useTpicAdvancedMode) {
	    engineConfiguration->hipOutputChannel = EFI_ADC_10; // PC0
	}
}

/**
 * @brief	Global default engine configuration
 * This method sets the global engine configuration defaults. These default values are then
 * overridden by engine-specific defaults and the settings are saved in flash memory.
 *
 * This method is invoked only when new configuration is needed:
 *  * recently re-flashed chip
 *  * flash version of configuration failed CRC check or appears to be older then FLASH_DATA_VERSION
 *  * 'rewriteconfig' command
 *  * 'set engine_type X' command
 *
 * This method should only change the state of the configuration data structure but should NOT change the state of
 * anything else.
 *
 * This method should NOT be setting any default pinout
 */
static void setDefaultEngineConfiguration() {
#if (! EFI_UNIT_TEST)
	efi::clear(persistentState.persistentConfiguration);
#endif
	prepareVoidConfiguration(engineConfiguration);

	setDefaultBaseEngine();
	setDefaultFuel();
	setDefaultIgnition();
	setDefaultCranking();

	// VVT closed loop, totally random values!
	engineConfiguration->auxPid[0].pFactor = 2;
	engineConfiguration->auxPid[0].iFactor = 0.005;
	engineConfiguration->auxPid[0].dFactor = 0;
	engineConfiguration->auxPid[0].offset = 33;
	engineConfiguration->auxPid[0].minValue = 10;
	engineConfiguration->auxPid[0].maxValue = 90;

	engineConfiguration->vvtOutputFrequency[0] = 300; // VVT solenoid control

	engineConfiguration->isCylinderCleanupEnabled = true;

	engineConfiguration->auxPid[1].minValue = 10;
	engineConfiguration->auxPid[1].maxValue = 90;

	engineConfiguration->turboSpeedSensorMultiplier = 1;

#if EFI_IDLE_CONTROL
	setDefaultIdleParameters();
#endif /* EFI_IDLE_CONTROL */

#if EFI_ELECTRONIC_THROTTLE_BODY
	setDefaultEtbParameters();
	setDefaultEtbBiasCurve();
#endif /* EFI_ELECTRONIC_THROTTLE_BODY */
#if EFI_BOOST_CONTROL
    setDefaultBoostParameters();
#endif

    // OBD-II default rate is 500kbps
    engineConfiguration->canBaudRate = B500KBPS;
    engineConfiguration->can2BaudRate = B500KBPS;

	engineConfiguration->mafSensorType = Bosch0280218037;
	setBosch0280218037(config);

	engineConfiguration->canSleepPeriodMs = 50;
	engineConfiguration->canReadEnabled = true;
	engineConfiguration->canWriteEnabled = true;

	// Don't enable, but set default address
	engineConfiguration->verboseCanBaseAddress = CAN_DEFAULT_BASE;

	engineConfiguration->sdCardPeriodMs = 50;

	engineConfiguration->mapMinBufferLength = 1;
	engineConfiguration->vvtActivationDelayMs = 6000;
	
	engineConfiguration->startCrankingDuration = 3;

	engineConfiguration->maxAcRpm = 5000;
	engineConfiguration->maxAcClt = 100;
	engineConfiguration->maxAcTps = 75;

	initTemperatureCurve(IAT_FUEL_CORRECTION_CURVE, 1);

	engineConfiguration->auxPid[0].minValue = 10;
	engineConfiguration->auxPid[0].maxValue = 90;

	engineConfiguration->alternatorControl.minValue = 0;
	engineConfiguration->alternatorControl.maxValue = 90;

	setLinearCurve(config->scriptCurve1Bins, 0, 100, 1);
	setLinearCurve(config->scriptCurve1, 0, 100, 1);

	setLinearCurve(config->scriptCurve2Bins, 0, 100, 1);
	setLinearCurve(config->scriptCurve2, 30, 170, 1);

	setLinearCurve(config->scriptCurve3Bins, 0, 100, 1);
	setLinearCurve(config->scriptCurve4Bins, 0, 100, 1);
	setLinearCurve(config->scriptCurve5Bins, 0, 100, 1);
	setLinearCurve(config->scriptCurve6Bins, 0, 100, 1);

#if EFI_ENGINE_CONTROL
	setDefaultWarmupIdleCorrection();

	setLinearCurve(engineConfiguration->map.samplingAngleBins, 800, 7000, 1);
	setLinearCurve(engineConfiguration->map.samplingAngle, 100, 130, 1);
	setLinearCurve(engineConfiguration->map.samplingWindowBins, 800, 7000, 1);
	setLinearCurve(engineConfiguration->map.samplingWindow, 50, 50, 1);

	setLinearCurve(config->vvtTable1LoadBins, 20, 120, 10);
	setRpmTableBin(config->vvtTable1RpmBins, SCRIPT_TABLE_8);
	setLinearCurve(config->vvtTable2LoadBins, 20, 120, 10);
	setRpmTableBin(config->vvtTable2RpmBins, SCRIPT_TABLE_8);
	setLinearCurve(config->scriptTable1LoadBins, 20, 120, 10);
	setRpmTableBin(config->scriptTable1RpmBins, SCRIPT_TABLE_8);
	setLinearCurve(config->scriptTable2LoadBins, 20, 120, 10);
	setRpmTableBin(config->scriptTable2RpmBins, SCRIPT_TABLE_8);
	setLinearCurve(config->scriptTable3LoadBins, 20, 120, 10);
	setRpmTableBin(config->scriptTable3RpmBins, SCRIPT_TABLE_8);
	setLinearCurve(config->scriptTable4LoadBins, 20, 120, 10);
	setRpmTableBin(config->scriptTable4RpmBins, SCRIPT_TABLE_8);

	setDefaultEngineNoiseTable();

	engineConfiguration->clt.config = {0, 23.8889, 48.8889, 9500, 2100, 1000, 1500};

// todo: this value is way off! I am pretty sure temp coeffs are off also
	engineConfiguration->iat.config = {32, 75, 120, 9500, 2100, 1000, 2700};

	// wow unit tests have much cooler setDefaultLaunchParameters method
	engineConfiguration->launchRpm = 3000;
// 	engineConfiguration->launchTimingRetard = 10;
	engineConfiguration->launchTimingRpmRange = 500;
    engineConfiguration->launchSpeedThreshold = 30;
	engineConfiguration->hardCutRpmRange = 500;

	engineConfiguration->slowAdcAlpha = 0.33333;
	engineConfiguration->engineSnifferRpmThreshold = 2500;
	engineConfiguration->sensorSnifferRpmThreshold = 2500;

	engineConfiguration->noAccelAfterHardLimitPeriodSecs = 3;

	/**
	 * Idle control defaults
	 */
	setDefaultIdleSpeedTarget();
	//	setTargetRpmCurve(1200);

	engineConfiguration->idleRpmPid.pFactor = 0.05;
	engineConfiguration->idleRpmPid.iFactor = 0.002;

	engineConfiguration->idleRpmPid.minValue = 0;
	engineConfiguration->idleRpmPid.maxValue = 99;
	/**
	 * between variation between different sensor and weather and fabrication tolerance
	 * five percent looks like a safer default
	 */
	engineConfiguration->idlePidDeactivationTpsThreshold = 5;

	engineConfiguration->idle.solenoidFrequency = 200;
	// set idle_position 50
	engineConfiguration->manIdlePosition = 50;
//	engineConfiguration->idleMode = IM_AUTO;
	engineConfiguration->idleMode = IM_MANUAL;

	engineConfiguration->useStepperIdle = false;

	setDefaultGppwmParameters();

#if !EFI_UNIT_TEST
	engineConfiguration->analogInputDividerCoefficient = 2;
#endif

	// performance optimization
	engineConfiguration->sensorChartMode = SC_OFF;

	engineConfiguration->tpsMin = convertVoltageTo10bitADC(0);
	engineConfiguration->tpsMax = convertVoltageTo10bitADC(5);
	engineConfiguration->tps1SecondaryMin = convertVoltageTo10bitADC(0);
	engineConfiguration->tps1SecondaryMax = convertVoltageTo10bitADC(5);
	engineConfiguration->tps2Min = convertVoltageTo10bitADC(0);
	engineConfiguration->tps2Max = convertVoltageTo10bitADC(5);
	engineConfiguration->tps2SecondaryMin = convertVoltageTo10bitADC(0);
	engineConfiguration->tps2SecondaryMax = convertVoltageTo10bitADC(5);
	engineConfiguration->idlePositionMin = PACK_MULT_VOLTAGE * 0;
	engineConfiguration->idlePositionMax = PACK_MULT_VOLTAGE * 5;
	engineConfiguration->wastegatePositionMin = PACK_MULT_VOLTAGE * 0;
	engineConfiguration->wastegatePositionMax = PACK_MULT_VOLTAGE * 5;
	engineConfiguration->tpsErrorDetectionTooLow = -10; // -10% open
	engineConfiguration->tpsErrorDetectionTooHigh = 110; // 110% open

	engineConfiguration->oilPressure.v1 = 0.5f;
	engineConfiguration->oilPressure.v2 = 4.5f;
	engineConfiguration->oilPressure.value1 = 0;
	engineConfiguration->oilPressure.value2 = 689.476f;	// 100psi = 689.476kPa

	engineConfiguration->mapLowValueVoltage = 0;
	// todo: start using this for custom MAP
	engineConfiguration->mapHighValueVoltage = 5;

	engineConfiguration->HD44780width = 20;
	engineConfiguration->HD44780height = 4;

	engineConfiguration->cylinderBore = 87.5;

	setBoschHDEV_5_injectors();

	setEgoSensor(ES_14Point7_Free);

	engineConfiguration->globalFuelCorrection = 1;
	engineConfiguration->adcVcc = 3.0;

	engineConfiguration->map.sensor.type = MT_MPX4250;

	engineConfiguration->baroSensor.type = MT_CUSTOM;
	engineConfiguration->baroSensor.lowValue = 0;
	engineConfiguration->baroSensor.highValue = 500;

#if EFI_PROD_CODE
	engineConfiguration->engineChartSize = 300;
#else
	// need more events for automated test
	engineConfiguration->engineChartSize = 400;
#endif

	engineConfiguration->isMapAveragingEnabled = true;
	engineConfiguration->isWaveAnalyzerEnabled = true;

	engineConfiguration->acIdleRpmBump = 200;

	/* these two are used for HIP9011 only
	 * Currently this is offset from fire event, not TDC */
	/* TODO: convert to offset from TDC */
	engineConfiguration->knockDetectionWindowStart = 15.0 + 5.0;
	engineConfiguration->knockDetectionWindowEnd = 15.0 + 45.0;

	/**
	 * this is RPM. 10000 rpm is only 166Hz, 800 rpm is 13Hz
	 */
	engineConfiguration->triggerSimulatorFrequency = 1200;

	engineConfiguration->alternatorPwmFrequency = 300;

	engineConfiguration->cj125isUaDivided = true;

	engineConfiguration->isAlternatorControlEnabled = false;

	engineConfiguration->driveWheelRevPerKm = 500;
	engineConfiguration->vssGearRatio = 3.73;
	engineConfiguration->vssToothCount = 21;

	engineConfiguration->mapErrorDetectionTooLow = 5;
	// todo: default limits should be hard-coded for each sensor type
	// https://github.com/rusefi/rusefi/issues/4030
	engineConfiguration->mapErrorDetectionTooHigh = 410;

	engineConfiguration->useLcdScreen = true;

	engineConfiguration->hip9011Gain = 1;

	engineConfiguration->isEngineControlEnabled = true;
#endif // EFI_ENGINE_CONTROL
	strncpy(config->luaScript, "function onTick()\nend", efi::size(config->luaScript));
}

/**
 * @brief	Hardware board-specific default configuration (GPIO pins, ADC channels, SPI configs etc.)
 */
void setDefaultFrankensoConfiguration() {
	setDefaultFrankensoStepperIdleParameters();
	setCanFrankensoDefaults();

	engineConfiguration->map.sensor.hwChannel = EFI_ADC_4;
	engineConfiguration->clt.adcChannel = EFI_ADC_6;
	engineConfiguration->iat.adcChannel = EFI_ADC_7;
	engineConfiguration->afr.hwChannel = EFI_ADC_14;

	engineConfiguration->accelerometerSpiDevice = SPI_DEVICE_1;
	engineConfiguration->hip9011SpiDevice = SPI_DEVICE_2;
	engineConfiguration->cj125SpiDevice = SPI_DEVICE_2;

//	engineConfiguration->gps_rx_pin = Gpio::B7;
//	engineConfiguration->gps_tx_pin = Gpio::B6;

	engineConfiguration->triggerSimulatorPins[0] = Gpio::D1;
	engineConfiguration->triggerSimulatorPins[1] = Gpio::D2;

	engineConfiguration->triggerInputPins[0] = Gpio::C6;
	engineConfiguration->triggerInputPins[1] = Gpio::A5;

	// set this to SPI_DEVICE_3 to enable stimulation
	//engineConfiguration->digitalPotentiometerSpiDevice = SPI_DEVICE_3;
	engineConfiguration->digitalPotentiometerChipSelect[0] = Gpio::D7;
	engineConfiguration->digitalPotentiometerChipSelect[1] = Gpio::Unassigned;
	engineConfiguration->digitalPotentiometerChipSelect[2] = Gpio::D5;
	engineConfiguration->digitalPotentiometerChipSelect[3] = Gpio::Unassigned;

	engineConfiguration->spi1mosiPin = Gpio::B5;
	engineConfiguration->spi1misoPin = Gpio::B4;
	engineConfiguration->spi1sckPin = Gpio::B3; // please note that this pin is also SWO/SWD - Single Wire debug Output

	engineConfiguration->spi2mosiPin = Gpio::B15;
	engineConfiguration->spi2misoPin = Gpio::B14;
	engineConfiguration->spi2sckPin = Gpio::B13;

	engineConfiguration->spi3mosiPin = Gpio::B5;
	engineConfiguration->spi3misoPin = Gpio::B4;
	engineConfiguration->spi3sckPin = Gpio::B3;
	
	// set optional subsystem configs
#if EFI_MEMS
	// this would override some values from above
	configureAccelerometerPins();
#endif /* EFI_MEMS */

#if EFI_HIP_9011
	setHip9011FrankensoPinout();
#endif /* EFI_HIP_9011 */

#if EFI_FILE_LOGGING
	setDefaultSdCardParameters();
#endif /* EFI_FILE_LOGGING */

	engineConfiguration->is_enabled_spi_1 = false;
	engineConfiguration->is_enabled_spi_2 = false;
	engineConfiguration->is_enabled_spi_3 = true;
}

#ifdef CONFIG_RESET_SWITCH_PORT
// this pin is not configurable at runtime so that we have a reliable way to reset configuration
#define SHOULD_IGNORE_FLASH() (palReadPad(CONFIG_RESET_SWITCH_PORT, CONFIG_RESET_SWITCH_PIN) == 0)
#else
#define SHOULD_IGNORE_FLASH() (false)
#endif // CONFIG_RESET_SWITCH_PORT

// by default, do not ignore config from flash! use it!
#ifndef IGNORE_FLASH_CONFIGURATION
#define IGNORE_FLASH_CONFIGURATION false
#endif

void loadConfiguration() {
#ifdef CONFIG_RESET_SWITCH_PORT
	// initialize the reset pin if necessary
	palSetPadMode(CONFIG_RESET_SWITCH_PORT, CONFIG_RESET_SWITCH_PIN, PAL_MODE_INPUT_PULLUP);
#endif /* CONFIG_RESET_SWITCH_PORT */

#if ! EFI_ACTIVE_CONFIGURATION_IN_FLASH
	// Clear the active configuration so that registered output pins (etc) detect the change on startup and init properly
	prepareVoidConfiguration(&activeConfiguration);
#endif /* EFI_ACTIVE_CONFIGURATION_IN_FLASH */

#if EFI_INTERNAL_FLASH
	if (SHOULD_IGNORE_FLASH() || IGNORE_FLASH_CONFIGURATION) {
		engineConfiguration->engineType = DEFAULT_ENGINE_TYPE;
		resetConfigurationExt(engineConfiguration->engineType);
		writeToFlashNow();
	} else {
		// this call reads configuration from flash memory or sets default configuration
		// if flash state does not look right.
		readFromFlash();
	}
#else // not EFI_INTERNAL_FLASH
	// This board doesn't load configuration, initialize the default
	engineConfiguration->engineType = DEFAULT_ENGINE_TYPE;
	resetConfigurationExt(engineConfiguration->engineType);
#endif /* EFI_INTERNAL_FLASH */

	// Force any board configuration options that humans shouldn't be able to change
	setBoardConfigOverrides();
}

void resetConfigurationExt(configuration_callback_t boardCallback, engine_type_e engineType) {
	enginePins.reset(); // that's mostly important for functional tests
	/**
	 * Let's apply global defaults first
	 */
	setDefaultEngineConfiguration();

	// set initial pin groups
	setDefaultBasePins();

	if (boardCallback != nullptr) {
		boardCallback(engineConfiguration);
	}

#if EFI_PROD_CODE
	// call overrided board-specific configuration setup, if needed (for custom boards only)
	setBoardDefaultConfiguration();
	setBoardConfigOverrides();
#endif

	engineConfiguration->engineType = engineType;

	/**
	 * And override them with engine-specific defaults
	 */
	switch (engineType) {
	case HELLEN72_ETB:
	case MINIMAL_PINS:
		// all basic settings are already set in prepareVoidConfiguration(), no need to set anything here
		// nothing to do - we do it all in setBoardDefaultConfiguration
		break;
	case TEST_ENGINE:
		setTestCamEngineConfiguration();
		break;
	case TEST_CRANK_ENGINE:
		setTestCrankEngineConfiguration();
		break;
#if EFI_UNIT_TEST
	case TEST_ISSUE_366_BOTH:
		setTestEngineIssue366both();
		break;
	case TEST_ISSUE_366_RISE:
		setTestEngineIssue366rise();
		break;
	case TEST_ISSUE_898:
		setIssue898();
		break;
#endif // EFI_UNIT_TEST
#if HW_MICRO_RUSEFI
	case MRE_VW_B6:
		setMreVwPassatB6();
		break;
	case MRE_M111:
		setM111EngineConfiguration();
		break;
	case MRE_SECONDARY_CAN:
		mreSecondaryCan();
		break;
	case MRE_SUBARU_EJ18:
		setSubaruEJ18_MRE();
		break;
	case MRE_BOARD_NEW_TEST:
		mreBoardNewTest();
		break;
	case BMW_M73_MRE:
	case BMW_M73_MRE_SLAVE:
		setEngineBMW_M73_microRusEfi();
		break;
	case MRE_MIATA_NA6_VAF:
		setMiataNA6_VAF_MRE();
		break;
	case MRE_MIATA_94_MAP:
		setMiata94_MAP_MRE();
		break;
	case MRE_MIATA_NA6_MAP:
		setMiataNA6_MAP_MRE();
		break;
	case MRE_MIATA_NB2_MAP:
		setMiataNB2_MRE_MAP();
		break;
	case MRE_MIATA_NB2_MAF:
		setMiataNB2_MRE_MAF();
		break;
	case MRE_MIATA_NB2_ETB:
		setMiataNB2_MRE_ETB();
		break;
	case MRE_BODY_CONTROL:
		mreBCM();
		break;
#endif // HW_MICRO_RUSEFI
#if HW_PROTEUS
	case PROTEUS_VW_B6:
		setProteusVwPassatB6();
		break;
	case PROTEUS_QC_TEST_BOARD:
		proteusBoardTest();
		break;
	case PROTEUS_LUA_DEMO:
		proteusLuaDemo();
		break;
	case PROTEUS_HARLEY:
		proteusHarley();
		break;
	case PROTEUS_BMW_M73:
		setEngineBMW_M73_Proteus();
		break;
	case MIATA_PROTEUS_TCU:
		setMiataNB2_Proteus_TCU();
		break;
	case PROTEUS_HONDA_ELEMENT_2003:
		setProteusHondaElement2003();
		break;
	case PROTEUS_HONDA_OBD2A:
		setProteusHondaOBD2A();
		break;
	case PROTEUS_E65_6H_MAN_IN_THE_MIDDLE:
		setEngineProteusGearboxManInTheMiddle();
		break;
	case PROTEUS_VAG_80_18T:
	case PROTEUS_N73:
	case PROTEUS_MIATA_NB2:
		setMiataNB2_ProteusEngineConfiguration();
		break;
#ifdef HARDWARE_CI
	case PROTEUS_ANALOG_PWM_TEST:
		setProteusAnalogPwmTest();
		break;
#endif // HARDWARE_CI
#endif // HW_PROTEUS
#if HW_HELLEN
	case HELLEN_128_MERCEDES_4_CYL:
		setHellenMercedes128_4_cyl();
		break;
	case HELLEN_128_MERCEDES_6_CYL:
		setHellenMercedes128_6_cyl();
		break;
	case HELLEN_128_MERCEDES_8_CYL:
		setHellenMercedes128_8_cyl();
		break;
	case HELLEN_NB2:
		setMiataNB2_Hellen72();
		break;
	case HELLEN_NB2_36:
		setMiataNB2_Hellen72_36();
		break;
	case HELLEN_NA8_96:
		setHellenMiata96();
		break;
	case HELLEN_NB1:
		setHellenNB1();
		break;
	case HELLEN_121_NISSAN_4_CYL:
		setHellen121nissanQR();
		break;
	case HELLEN_121_NISSAN_6_CYL:
		setHellen121nissanVQ();
		break;
	case HELLEN_121_VAG_5_CYL:
	    setHellen121Vag_5_cyl();
        break;
	case HELLEN_121_VAG_V6_CYL:
	    setHellen121Vag_v6_cyl();
        break;
	case HELLEN_121_VAG_VR6_CYL:
	    setHellen121Vag_vr6_cyl();
        break;
	case HELLEN_121_VAG_8_CYL:
	    setHellen121Vag_8_cyl();
        break;
	case HELLEN_121_VAG_4_CYL:
	case HELLEN_55_BMW:
	case HELLEN_88_BMW:
	case HELLEN_134_BMW:
	case HELLEN_154_VAG:
		break;
	case HELLEN_154_HYUNDAI_COUPE_BK1:
		setGenesisCoupeBK1();
		break;
	case HELLEN_154_HYUNDAI_COUPE_BK2:
		setGenesisCoupeBK2();
		break;
	case HELLEN_NA6:
		setHellenNA6();
		break;
	case HELLEN_NA94:
		setHellenNA94();
		break;
#endif // HW_HELLEN
#if HW_FRANKENSO
	case DEFAULT_FRANKENSO:
		setFrankensoConfiguration();
		break;
	case FRANKENSO_QA_ENGINE:
		setFrankensoBoardTestConfiguration();
		break;
	case FRANKENSO_BMW_M73_F:
		setBMW_M73_TwoCoilUnitTest();
		break;
	case BMW_M73_M:
		setEngineBMW_M73_Manhattan();
		break;
	case DODGE_NEON_1995:
		setDodgeNeon1995EngineConfiguration();
		break;
	case DODGE_NEON_2003_CRANK:
		setDodgeNeonNGCEngineConfiguration();
		break;
	case FORD_ASPIRE_1996:
		setFordAspireEngineConfiguration();
		break;
	case NISSAN_PRIMERA:
		setNissanPrimeraEngineConfiguration();
		break;
	case FRANKENSO_MIATA_NA6_MAP:
		setMiataNA6_MAP_Frankenso();
		break;
	case FRANKENSO_MIATA_NA6_VAF:
		setMiataNA6_VAF_Frankenso();
		break;
	case ETB_BENCH_ENGINE:
		setEtbTestConfiguration();
		break;
	case L9779_BENCH_ENGINE:
		setL9779TestConfiguration();
		break;
	case EEPROM_BENCH_ENGINE:
#if EFI_PROD_CODE
		setEepromTestConfiguration();
#endif
		break;
	case TLE8888_BENCH_ENGINE:
		setTle8888TestConfiguration();
		break;
	case FRANKENSO_MAZDA_MIATA_NA8:
		setFrankensoMazdaMiataNA8Configuration();
		break;
	case MITSU_4G93:
		setMitsubishiConfiguration();
		break;
	case FORD_INLINE_6_1995:
		setFordInline6();
		break;
	case GY6_139QMB:
		setGy6139qmbDefaultEngineConfiguration();
		break;
	case HONDA_600:
		setHonda600();
		break;
	case FORD_ESCORT_GT:
		setFordEscortGt();
		break;
	case MIATA_1996:
		setFrankensteinMiata1996();
		break;
	case CITROEN_TU3JP:
		setCitroenBerlingoTU3JPConfiguration();
		break;
	case SUBARU_2003_WRX:
		setSubaru2003Wrx();
		break;
	case DODGE_RAM:
		setDodgeRam1996();
		break;
	case VW_ABA:
		setVwAba();
		break;
	case FRANKENSO_MAZDA_MIATA_2003:
		setMazdaMiata2003EngineConfiguration();
		break;
	case MAZDA_MIATA_2003_NA_RAIL:
		setMazdaMiata2003EngineConfigurationNaFuelRail();
		break;
	case MAZDA_MIATA_2003_BOARD_TEST:
		setMazdaMiata2003EngineConfigurationBoardTest();
		break;
	case TEST_ENGINE_VVT:
		setTestVVTEngineConfiguration();
		break;
	case SACHS:
		setSachs();
		break;
	case CAMARO_4:
		setCamaro4();
		break;
	case TOYOTA_2JZ_GTE_VVTi:
		setToyota_2jz_vics();
		break;
	case TOYOTA_JZS147:
		setToyota_jzs147EngineConfiguration();
		break;
	case TEST_33816:
		setTest33816EngineConfiguration();
		break;
	case TEST_100:
	case TEST_101:
	case TEST_102:
	case TEST_ROTARY:
		setRotary();
		break;
#endif // HW_FRANKENSO
#ifdef HW_SUBARU_EG33
	case SUBARUEG33_DEFAULTS:
		setSubaruEG33Defaults();
		break;
#endif //HW_SUBARU_EG33
	default:
		firmwareError(CUSTOM_UNEXPECTED_ENGINE_TYPE, "Unexpected engine type: %d", engineType);
	}
	applyNonPersistentConfiguration();
}

void emptyCallbackWithConfiguration(engine_configuration_s * engineConfiguration) {
	UNUSED(engineConfiguration);
}

void resetConfigurationExt(engine_type_e engineType) {
	resetConfigurationExt(&emptyCallbackWithConfiguration, engineType);
}

void validateConfiguration() {
	if (engineConfiguration->adcVcc > 5.0f || engineConfiguration->adcVcc < 1.0f) {
		engineConfiguration->adcVcc = 3.0f;
	}
	engine->preCalculate();
}

void applyNonPersistentConfiguration() {
#if EFI_PROD_CODE
	efiAssertVoid(CUSTOM_APPLY_STACK, getCurrentRemainingStack() > EXPECTED_REMAINING_STACK, "apply c");
	efiPrintf("applyNonPersistentConfiguration()");
#endif

#if EFI_ENGINE_CONTROL
	engine->updateTriggerWaveform();
#endif // EFI_ENGINE_CONTROL
}

#if EFI_ENGINE_CONTROL

void prepareShapes() {
	prepareOutputSignals();

	engine->injectionEvents.addFuelEvents();
}

#endif

void setTwoStrokeOperationMode() {
	engineConfiguration->twoStroke = true;
}

void setCamOperationMode() {
	engineConfiguration->skippedWheelOnCam = true;
}

void setCrankOperationMode() {
	engineConfiguration->skippedWheelOnCam = false;
}

void commonFrankensoAnalogInputs(engine_configuration_s *engineConfiguration) {
	/**
	 * VBatt
	 */
	engineConfiguration->vbattAdcChannel = EFI_ADC_14;
}

void setFrankenso0_1_joystick(engine_configuration_s *engineConfiguration) {
	
	engineConfiguration->joystickCenterPin = Gpio::C8;
	engineConfiguration->joystickAPin = Gpio::D10;
	engineConfiguration->joystickBPin = Gpio::Unassigned;
	engineConfiguration->joystickCPin = Gpio::Unassigned;
	engineConfiguration->joystickDPin = Gpio::D11;
}

// These symbols are weak so that a board_configuration.cpp file can override them
__attribute__((weak)) void setBoardDefaultConfiguration() { }
__attribute__((weak)) void setBoardConfigOverrides() { }
