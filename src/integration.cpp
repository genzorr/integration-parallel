#define _USE_MATH_DEFINES

#include <iostream>
#include <cmath>

/// Precision in number of digits.
const int desiredPrecision = 10;

const double xDown = 0.001;
const double xUp = 1;

/// h(x) = PI/alpha*x^2
double alpha = 100;
/// x_(n+1) = x_(n)*borderCoef
double borderCoef = 1.1;

/// Max error on current area.
double calculateMaxErrorOnArea(double x1, double x2, double step)
{
	return 1 / 12.0 * (x2-x1)*step*step/(x1*x1*x1*x1);
}

/// Function to integrate.
double function(double x)
{
	if (x == 0)
	{
		std::cout << "Error: Zero division\n";
		return 0;
	}
	return sin(1 / x);
}

/// Number of areas.
int initAreasCount()
{
	double tempBorder = xDown;
	int count = 0;
	while (tempBorder <= xUp)
	{
		tempBorder *= borderCoef;
		count++;
	}
	return count;
}

/// Number of steps on area.
int getStepCount(double x1, double x2, double step)
{
	return (int)((x2 - x1) / step);
}

/// Step for area with left border equal to x.
double getStep(double x)
{
	return x == 0 ? -1 : M_PI / alpha * x * x;
}

/// Next x on the right from current.
double getNextX(double x, double step)
{
	return x+step;
}

/// Right border of area.
double getAreaBorderUp(int area)
{
	double borderUp = xDown;
	for (int i = 0; i < area; i++)
	    borderUp *= borderCoef;

	return borderUp > xUp ? xUp : borderUp;
}

/// Left border of area.
double getAreaBorderDown(int area)
{
	double borderDown = xDown;
	for (int i = 0; i < area-1; i++)
		borderDown *= borderCoef;

	return borderDown;
}

/// Trapezoidal integration.
double trapezoidValue(double x1, double x2, double step)
{
	return (function(x1) + function(x2)) / 2 * step;
}

/// Sets alpha and borderCoef params to get desired precision.
void setPrecision()
{
	alpha = 10;
	borderCoef = 5;

	double targetPrecision = pow(10, -desiredPrecision);
	std::cout << "Target precision = " << targetPrecision << '\n';

	while (true)
	{
		int areasCount = initAreasCount();
		double error = 0;
		for (int area = 1; area <= areasCount; area++)
		{
			double borderUp = getAreaBorderUp(area);
			double borderDown = getAreaBorderDown(area);
			double step = getStep(borderDown);
			error += calculateMaxErrorOnArea(borderDown, borderUp, step);
			if (error >= targetPrecision)
			{
				alpha *= 10;
				borderCoef = 1 + (borderCoef - 1) / 2;
				std::cout << "Current precision (not enough) = " << error << '\n';
				break;
			}
		}
		if (error < targetPrecision)
		{
			std::cout << "Current precision (enough) = " << error << '\n';
			break;
		}
	}
}


int main()
{
	std::cout.precision(17);

	setPrecision();

	int areasCount = initAreasCount();
	std::cout << " Areas: " << areasCount << '\n';

	double sum = 0;
	double sumError = 0;
	for (int area = 1; area <= areasCount; area++)
	{
		double borderUp = getAreaBorderUp(area);
		double borderDown = getAreaBorderDown(area);
		double step = getStep(borderDown);
		double error = calculateMaxErrorOnArea(borderDown, borderUp, step);

		sumError += error;
		std::cout << "Error on area " << area << " = " << error << '\n';

		/// Init.
		double x = borderDown;
		double nextX = getNextX(x, step);

		/// Calculate integral with trapezoidal rule, until right x became righter from right border (or equal).
		while (nextX < borderUp)
		{
			sum += trapezoidValue(x, nextX, step);
			x = nextX;
			nextX = getNextX(x, step);
		}

		/// Last step "by hands", step will be less or equal than on the rest of the area.
		sum += trapezoidValue(x, borderUp, borderUp - x);
	}

	std::cout << "integral(sin(1/x)) = " << sum << '\n';
	std::cout << "Error = " << sumError << '\n';

	return 0;
}