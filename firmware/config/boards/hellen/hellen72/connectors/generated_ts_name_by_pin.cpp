//DO NOT EDIT MANUALLY, let automation work hard.

// auto-generated by PinoutLogic.java based on config/boards/hellen/hellen72/connectors/main.yaml
#include "pch.h"

// see comments at declaration in pin_repository.h
const char * getBoardSpecificPinName(brain_pin_e brainPin) {
	switch(brainPin) {
		case Gpio::A0: return "5N - TPS2";
		case Gpio::A3: return "5P - PPS1";
		case Gpio::A6: return "3V - CAM (A19)";
		case Gpio::A7: return "4J - VTCS/AUX4 (A20)";
		case Gpio::A9: return "3P - O2H2 (O7)";
		case Gpio::B0: return "4F - AC_PRES/AUX1 (A23)";
		case Gpio::B1: return "3Y - CRANK (A24)";
		case Gpio::C0: return "5A - Pressure Input";
		case Gpio::C4: return "5M - PPS2 OR TEMPERATURE SENSOR";
		case Gpio::C6: return "ETB EN";
		case Gpio::C7: return "ETB +";
		case Gpio::C8: return "ETB -";
		case Gpio::C9: return "5E - SOLENOID OUTPUT";
		case Gpio::D10: return "2J - INJ_4";
		case Gpio::D11: return "2G - INJ_3";
		case Gpio::D12: return "2B - ECF (PWM8)";
		case Gpio::D13: return "3O - TACH (PWM7)";
		case Gpio::D14: return "2Q - IDLE (PWM5)";
		case Gpio::D15: return "3M - ALTERN (PWM6)";
		case Gpio::D9: return "2C - AC Fan / INJ_5";
		case Gpio::E12: return "5I - Digital Input";
		case Gpio::E14: return "4H - Neutral";
		case Gpio::E15: return "5C - Digital Input";
		case Gpio::E2: return "3Z - IGN_5 / GNDA";
		case Gpio::E3: return "3N - IGN_4";
		case Gpio::E4: return "2O - IGN_3";
		case Gpio::E5: return "3I - IGN_2 (2&3)";
		case Gpio::F11: return "3T - VSS (D5)";
		case Gpio::F12: return "2N - VTSC / INJ_6";
		case Gpio::F13: return "3C - Purge Solenoid / INJ_7";
		case Gpio::F14: return "3D - EGR Solenoid / INJ_8";
		case Gpio::F5: return "5D - SENSOR INPUT";
		case Gpio::F8: return "4I - Clutch (A8)";
		case Gpio::F9: return "4B - Brake/RES1 (A7)";
		case Gpio::G2: return "2M - FPUMP (O9)";
		case Gpio::G3: return "3E - CANIST (O10)";
		case Gpio::G4: return "2R - CE (O11)";
		case Gpio::G7: return "2A - INJ_1";
		case Gpio::G8: return "2D - INJ_2";
		case Gpio::H13: return "3U - AWARN (O2)";
		case Gpio::H14: return "3J - O2H (O3)";
		case Gpio::H15: return "2K - AC (O4)";
		case Gpio::I0: return "4R - VVT (O5)";
		case Gpio::I2: return "3H - MAIN (O1)";
		case Gpio::I5: return "4K - IGN_6 / +5V_MAP";
		case Gpio::I6: return "3L - IGN_7 / AFR";
		case Gpio::I7: return "4U - MAP2/Ign8 (A10)";
		case Gpio::I8: return "3F - IGN_1 (1&4)";
		default: return nullptr;
	}
	return nullptr;
}
