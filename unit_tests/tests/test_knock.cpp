#include "pch.h"

#include "knock_logic.h"

struct MockKnockController : public KnockController {
	float getKnockThreshold() const override {
		// Knock threshold of 20dBv
		return 20;
	}
};

TEST(Knock, Retards) {
	EngineTestHelper eth(TEST_ENGINE);

	// Aggression of 10%
	engineConfiguration->knockRetardAggression = 10;
	// Maximum 8 degrees retarded
	engineConfiguration->knockRetardMaximum = 8;

	MockKnockController dut;
	dut.onFastCallback();

	// No retard unless we knock
	ASSERT_FLOAT_EQ(dut.getKnockRetard(), 0);

	// Send some weak knocks, should yield no response
	for (size_t i = 0; i < 10; i++) {
		dut.onKnockSenseCompleted(0, 10, 0);
	}

	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 0);

	// Send a strong knock!
	dut.onKnockSenseCompleted(0, 30, 0);

	// Should retard 10% of the distance between current timing and "maximum"
	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 2);

	// Send tons of strong knocks, make sure we don't go over the configured limit
	for (size_t i = 0; i < 100; i++) {
		dut.onKnockSenseCompleted(0, 30, 0);
	}

	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 8);
}

TEST(Knock, Reapply) {
	EngineTestHelper eth(TEST_ENGINE);

	MockKnockController dut;
	dut.onFastCallback();

	// Aggression of 10%
	engineConfiguration->knockRetardAggression = 10;
	// Maximum 8 degrees retarded
	engineConfiguration->knockRetardMaximum = 8;
	// Apply 1 degree/second
	engineConfiguration->knockRetardReapplyRate = 1;

	// Send a strong knock!
	dut.onKnockSenseCompleted(0, 30, 0);

	// Should retard 10% of the distance between current timing and "maximum"
	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 2);

	constexpr auto fastPeriodSec = FAST_CALLBACK_PERIOD_MS / 1000.0f;

	// call the fast callback, should reapply 1 degree * callback period
	dut.onFastCallback();
	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 2 - 1.0f * fastPeriodSec);

	// 10 updates total
	for (size_t i = 0; i < 9; i++) {
		dut.onFastCallback();
	}
	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 2 - 10 * 1.0f * fastPeriodSec);

	// Spend a long time without knock
	for (size_t i = 0; i < 1000; i++) {
		dut.onFastCallback();
	}

	// Should have no knock retard
	EXPECT_FLOAT_EQ(dut.getKnockRetard(), 0);
}
