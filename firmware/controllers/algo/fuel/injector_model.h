#pragma once

#include "expected.h"
#include "injector_model_generated.h"

struct IInjectorModel : public EngineModule {
	virtual void prepare() = 0;
	virtual floatms_t getInjectionDuration(float fuelMassGram) const = 0;
	virtual float getFuelMassForDuration(floatms_t duration) const = 0;
};

class InjectorModelBase : public IInjectorModel, public injector_model_s {
public:
	void prepare() override;
	floatms_t getInjectionDuration(float fuelMassGram) const override;
	float getFuelMassForDuration(floatms_t duration) const override;

	virtual floatms_t getDeadtime() const = 0;
	virtual float getInjectorMassFlowRate() = 0;
	virtual float getInjectorFlowRatio() = 0;
	virtual expected<float> getAbsoluteRailPressure() const = 0;
	virtual float correctShortPulse(float baseDuration) const = 0;

	virtual void postState(float deadTime) const { (void)deadTime; };

private:
	float m_massFlowRate = 0;
};

class InjectorModel : public InjectorModelBase {
public:
	void postState(float deadtime) const override;
	floatms_t getDeadtime() const override;
	float getInjectorMassFlowRate() override;
	float getInjectorFlowRatio() override;
	expected<float> getAbsoluteRailPressure() const override;

	// Small pulse correction logic
	float correctShortPulse(float baseDuration) const override;
	virtual float correctInjectionPolynomial(float baseDuration) const;

	using interface_t = IInjectorModel; // Mock interface
};
