/*
 * @file test_real_cranking_nissan_vq40.cpp
 *
 * @date Jul 21, 2021
 * @author Andrey Belomutskiy, (c) 2012-2021
 */

#include "pch.h"
#include "logicdata_csv_reader.h"

TEST(realCrankingVQ40, normalCranking) {
	CsvReader reader(1, /* vvtCount */ 2);
	int indeces[] = {0};

	reader.open("tests/trigger/resources/nissan_vq40_cranking-1.csv", indeces);
	EngineTestHelper eth (HELLEN_121_NISSAN_6_CYL);
	engineConfiguration->isFasterEngineSpinUpEnabled = false;
	engineConfiguration->alwaysInstantRpm = true;

	bool hasSeenFirstVvt = false;

	while (reader.haveMore()) {
		reader.processLine(&eth);
		float vvt1 = engine->triggerCentral.getVVTPosition(/*bankIndex*/0, /*camIndex*/0);

		if (vvt1 != 0 && !hasSeenFirstVvt) {
			EXPECT_NEAR(vvt1, -45.56, 1);
			hasSeenFirstVvt = true;
		}
	}

	EXPECT_NEAR(engine->triggerCentral.getVVTPosition(/*bankIndex*/0, /*camIndex*/0), -45.64, 1e-2);
	EXPECT_NEAR(engine->triggerCentral.getVVTPosition(/*bankIndex*/1, /*camIndex*/0), -45.45, 1e-2);
	ASSERT_EQ(101, round(Sensor::getOrZero(SensorType::Rpm)))<< reader.lineIndex();

	// TODO: why warnings?
	ASSERT_EQ(3, eth.recentWarnings()->getCount());
	ASSERT_EQ(CUSTOM_SYNC_COUNT_MISMATCH, eth.recentWarnings()->get(0).Code);
	ASSERT_EQ(CUSTOM_OUT_OF_ORDER_COIL, eth.recentWarnings()->get(1).Code);	// this is from a coil being protected by overdwell protection
	ASSERT_EQ(CUSTOM_SYNC_ERROR, eth.recentWarnings()->get(2).Code);
}
