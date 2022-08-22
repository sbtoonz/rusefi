#include "pch.h"
#include "rusefi_lua.h"

TEST(LuaBasic, ReturnsNumber) {
	auto script = R"(
						function testFunc()
							return 5.5
						end
				)";

	float result = testLuaReturnsNumber(script);

	EXPECT_FLOAT_EQ(result, 5.5f);
}

TEST(LuaBasic, MathLib) {
	auto script = R"(
		function testFunc()
			return math.min(3, 1, 2)
		end
	)";

	float result = testLuaReturnsNumber(script);

	EXPECT_FLOAT_EQ(result, 1.0f);
}

TEST(LuaBasic, MathLibFloor) {
	auto script = R"(
		function testFunc()
			return math.floor(3.2)
		end
	)";

	EXPECT_FLOAT_EQ(testLuaReturnsNumber(script), 3.0f);
}

TEST(LuaBasic, ReturnsInteger) {
	auto script = R"(
						function testFunc()
							return 33
						end
				)";

	int result = testLuaReturnsInteger(script);

	EXPECT_EQ(result, 33);
}

TEST(LuaBasic, InvalidScript) {
	EXPECT_ANY_THROW(testLuaReturnsInteger("ggg"));
}

TEST(LuaBasic, NoFunction) {
	EXPECT_ANY_THROW(testLuaReturnsInteger(R"(
		function testFuncccccc()
			return 3
		end
	)"));
}

TEST(LuaBasic, WrongReturnTypeExpectIntegerReturnsNothing) {
	EXPECT_ANY_THROW(testLuaReturnsInteger(R"(
		function testFunc()
		end
	)"));
}

TEST(LuaBasic, WrongReturnTypeExpectIntegerReturnsNil) {
	EXPECT_ANY_THROW(testLuaReturnsInteger(R"(
		function testFunc()
			return nil
		end
	)"));
}

TEST(LuaBasic, ExpectNumOrNilReturnsNil) {
	EXPECT_EQ(testLuaReturnsNumberOrNil(R"(
		function testFunc()
			return nil
		end
	)"), unexpected);
}

TEST(LuaBasic, ExpectNumOrNilReturnsNumber) {
	EXPECT_FLOAT_EQ(56.3f, testLuaReturnsNumberOrNil(R"(
		function testFunc()
			return 56.3
		end
	)").value_or(0));
}

TEST(LuaPid, Offset) {
	EXPECT_FLOAT_EQ(18.0f, testLuaReturnsNumberOrNil(R"(
		function testFunc()
pid = Pid.new(2, 0, 0, -100, 100)
pid:setOffset(20)
			return pid:get(20, 21)
		end
	)").value_or(0));
}

TEST(LuaBasic, ExpectNumOrNilReturnsNothing) {
	// Returning nothing is generally functionally equivalent to returning nil
	EXPECT_EQ(testLuaReturnsNumberOrNil(R"(
		function testFunc()
			return
		end
	)"), unexpected);
}

TEST(SystemLua, ScriptLoads) {
	startLua();
}
