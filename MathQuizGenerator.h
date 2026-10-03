#pragma once
#include <Arduino.h>

// Generates a random arithmetic question as a display-ready string (e.g. "12+7X3=?"),
// writes the fully-worked question+result into `answer` (e.g. "12+7X3=33"), and returns
// the question. Call randomSeed() once in setup() before using this so questions vary
// between boots: randomSeed(ESP.random()) on an ESP8266 (hardware RNG), or
// randomSeed(analogRead(<an unconnected analog pin>)) on an AVR.
//
// numberCeiling bounds the random operands (higher = harder questions). It must be at
// least 51: subtraction questions start from a first operand of 50 or more.
// includeDivision adds a 4th question type, "A/B=?" (A always evenly divisible by B).
String generateMathQuestion(String &answer, int numberCeiling = 100, bool includeDivision = false);
