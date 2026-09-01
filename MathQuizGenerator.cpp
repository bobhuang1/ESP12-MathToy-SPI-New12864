#include "MathQuizGenerator.h"

String generateMathQuestion(String &answer, int numberCeiling, bool includeDivision) {
	const String strPlusSign = "+";
	const String strMinusSign = "-";
	const String strMultiplySign = "X";
	const String strDivideSign = String((char)247); // division sign, U+00F7
	const String strEqualSign = "=";

	String question = "";
	int firstOperationType = random(1, includeDivision ? 5 : 4); // 1=plus, 2=minus, 3=multiply, 4=divide
	int secondOperationType = 1;
	int firstNumber = 1;
	int secondNumber = 1;
	int thirdNumber = 1;

	if (firstOperationType == 4) // divide - always evenly divisible
	{
		firstNumber = random(2, 100);
		secondNumber = random(2, 100);
		question = String(firstNumber * secondNumber) + strDivideSign + String(firstNumber) + strEqualSign + "?";
		answer = String(firstNumber * secondNumber) + strDivideSign + String(firstNumber) + strEqualSign + String(secondNumber);
	}
	else if (firstOperationType == 3) // multiply, combined with a second +/- operation
	{
		secondOperationType = random(1, 3);
		firstNumber = random(1, 10);
		secondNumber = random(1, 10);
		if (secondOperationType == 2)
		{
			thirdNumber = random(1, firstNumber * secondNumber);
			question = String(firstNumber) + strMultiplySign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strMultiplySign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + String(firstNumber * secondNumber - thirdNumber);
		}
		else
		{
			thirdNumber = random(1, numberCeiling);
			question = String(firstNumber) + strMultiplySign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strMultiplySign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + String(firstNumber * secondNumber + thirdNumber);
		}
	}
	else if (firstOperationType == 2) // minus, combined with a second +/-/X operation
	{
		secondOperationType = random(1, 4);
		firstNumber = random(50, numberCeiling);
		if (secondOperationType == 1)
		{
			secondNumber = random(0, firstNumber);
			thirdNumber = random(1, numberCeiling);
			question = String(firstNumber) + strMinusSign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strMinusSign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + String(firstNumber - secondNumber + thirdNumber);
		}
		else if (secondOperationType == 2)
		{
			secondNumber = random(30, firstNumber);
			thirdNumber = random(1, (firstNumber - secondNumber));
			question = String(firstNumber) + strMinusSign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strMinusSign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + String(firstNumber - secondNumber - thirdNumber);
		}
		else // multiply
		{
			secondNumber = random(1, 10);
			thirdNumber = random(1, 10);
			firstNumber = random(secondNumber * thirdNumber, numberCeiling);
			question = String(firstNumber) + strMinusSign + String(secondNumber) + strMultiplySign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strMinusSign + String(secondNumber) + strMultiplySign + String(thirdNumber) + strEqualSign + String(firstNumber - (secondNumber * thirdNumber));
		}
	}
	else // plus, combined with a second +/-/X operation
	{
		secondOperationType = random(1, 4);
		firstNumber = random(1, numberCeiling);
		if (secondOperationType == 1)
		{
			secondNumber = random(1, numberCeiling);
			thirdNumber = random(1, numberCeiling);
			question = String(firstNumber) + strPlusSign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strPlusSign + String(secondNumber) + strPlusSign + String(thirdNumber) + strEqualSign + String(firstNumber + secondNumber + thirdNumber);
		}
		else if (secondOperationType == 2)
		{
			secondNumber = random(1, numberCeiling);
			thirdNumber = random(1, (firstNumber - secondNumber));
			question = String(firstNumber) + strPlusSign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strPlusSign + String(secondNumber) + strMinusSign + String(thirdNumber) + strEqualSign + String(firstNumber + secondNumber - thirdNumber);
		}
		else
		{
			secondNumber = random(1, 10);
			thirdNumber = random(1, 10);
			question = String(firstNumber) + strPlusSign + String(secondNumber) + strMultiplySign + String(thirdNumber) + strEqualSign + "?";
			answer = String(firstNumber) + strPlusSign + String(secondNumber) + strMultiplySign + String(thirdNumber) + strEqualSign + String(firstNumber + (secondNumber * thirdNumber));
		}
	}
	return question;
}
