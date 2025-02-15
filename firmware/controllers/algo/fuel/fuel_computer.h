#pragma once

class ValueProvider3D;

#include "rusefi_types.h"

struct IFuelComputer {
	virtual mass_t getCycleFuel(mass_t airmass, int rpm, float load) const = 0;
};

// This contains the math of the fuel model, but doesn't actually read any configuration
class FuelComputerBase : public IFuelComputer {
public:
	mass_t getCycleFuel(mass_t airmass, int rpm, float load) const override;

protected:
	virtual float getStoichiometricRatio() const = 0;
	virtual float getTargetLambda(int rpm, float load) const = 0;
	virtual float getTargetLambdaLoadAxis(float defaultLoad) const = 0;
};

// This class is a usable implemenation of a fuel model that reads real configuration
class FuelComputer final : public FuelComputerBase {
public:
	FuelComputer(const ValueProvider3D& lambdaTable);

protected:
	float getStoichiometricRatio() const override;
	float getTargetLambda(int rpm, float load) const override;
	float getTargetLambdaLoadAxis(float defaultLoad) const override;

private:
	const ValueProvider3D* const m_lambdaTable;
};

float getLoadOverride(float defaultLoad, load_override_e overrideMode);
constexpr float fuelDensity = 0.72; // g/cc
