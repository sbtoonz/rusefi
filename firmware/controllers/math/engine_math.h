/**
 * @file	engine_math.h
 *
 * @date Jul 13, 2013
 * @author Andrey Belomutskiy, (c) 2012-2020
 */

#pragma once

#include "trigger_structure.h"

void setAlgorithm(engine_load_mode_e algo);

void setFlatInjectorLag(float value);

/**
 * See also wrapVvt
 * TODO: replace all usages of fixAngle with wrapAngle?
 * Should we make this a nice method instead of that off macro which changes parameter value?
 */
#define fixAngle(angle, msg, code) wrapAngle2(angle, msg, code, engine->engineCycle)
#define wrapAngle(angle, msg, code) fixAngle(angle, msg, code)

/**
 * @return time needed to rotate crankshaft by one degree, in milliseconds.
 * @deprecated use at least Us, maybe even Nt
 */
#define getOneDegreeTimeMs(rpm) (1000.0f * 60 / 360 / (rpm))

/**
 * @return float, time needed to rotate crankshaft by one degree, in microseconds.
 * See also engine->rpmCalculator.oneDegreeUs
 */
#define getOneDegreeTimeUs(rpm) (1000000.0f * 60 / 360 / (rpm))

floatms_t getCrankshaftRevolutionTimeMs(int rpm);
floatms_t getEngineCycleDuration(int rpm);

float getFuelingLoad();
float getIgnitionLoad();

floatms_t getSparkDwell(int rpm);

ignition_mode_e getCurrentIgnitionMode();

/**
 * This lightweight method is invoked in case of a configuration change or initialization.
 * But also it's used for "Spinning-up to Cranking" transition.
 */
void prepareIgnitionPinIndices(ignition_mode_e ignitionMode);

size_t getCylinderId(size_t index);
size_t getNextFiringCylinderId(size_t prevCylinderId);

void setTimingRpmBin(float from, float to);
void setTimingLoadBin(float from, float to);

void setSingleCoilDwell();

// we combine trigger-defined triggerShape.tdcPosition with user-defined engineConfiguration->globalTriggerAngleOffset
// expectation is that for well-known triggers engineConfiguration->globalTriggerAngleOffset would usually be zero
// while for toothed wheels user would have to provide a value
#define tdcPosition() \
		(TRIGGER_WAVEFORM(tdcPosition) + engineConfiguration->globalTriggerAngleOffset)

/** Gets phase offset for a particular cylinder's ID and number
 * For example on 4 cylinder engine with firing order 1-3-4-2, this
 * returns 0-180-360-540 for index 0-1-2-3
 * Cylinder number is used for per-cylinder adjustment, if you have
 * an odd-fire engine (v-twin, V10, some v6, etc)
 */
angle_t getCylinderAngle(uint8_t cylinderIndex, uint8_t cylinderNumber);
