#define _USE_MATH_DEFINES

#include <iostream>
#include <cmath>
#include <pthread.h>

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

const int NUM_THREADS = 4;

struct Area
{
    double borderUp = 0;
    double borderDown = 0;
    double step = 0;
};

struct ThreadDataIn
{
    double xUp = 0;
    double xDown = 0;
    int areaDown = 0;
    int numAreas = 0;
    Area *areas = nullptr;
};

struct ThreadDataOut
{
    double sum;
};

double updateX(double x, int *currentAreaNumber, int maxAreaNumber, Area *areas)
{
    double nextX = getNextX(x, areas[*currentAreaNumber].step);
    if (nextX > areas[*currentAreaNumber].borderUp)
    {
        *currentAreaNumber += 1;
        if (*currentAreaNumber < maxAreaNumber)
            nextX = areas[*currentAreaNumber].borderDown + areas[*currentAreaNumber].step;
        else
            nextX = areas[*currentAreaNumber-1].borderUp;
    }
    return nextX;
}

void* thread_function(void* arg)
{
    auto *data = (ThreadDataIn*) arg;
    auto areas = data->areas;
    auto retval = new ThreadDataOut;

    int currentAreaNumber = data->areaDown;
    double x = data->xDown;
    double nextX = updateX(x, &currentAreaNumber, data->numAreas, areas);

    /// Calculate integral with trapezoidal rule, until right x became righter from right border (or equal).
    while (nextX < data->xUp)
    {
        retval->sum += trapezoidValue(x, nextX, areas[currentAreaNumber].step);
        x = nextX;
        nextX = updateX(x, &currentAreaNumber, data->numAreas, areas);
    }

    /// Last step "by hands", step will be less or equal than on the rest of the area.
    retval->sum += trapezoidValue(x, data->xUp, data->xUp - x);
    pthread_exit(retval);
}

int main()
{
    std::cout.precision(17);
    setPrecision();

    int stepsNumber = 0;
    double sum = 0;
    double sumError = 0;

    int areasCount = initAreasCount();
    std::cout << "Areas: " << areasCount << '\n';
    auto areas = new Area[areasCount];
    for (int area = 0; area < areasCount; area++)
    {
        areas[area].borderUp = getAreaBorderUp(area + 1);
        areas[area].borderDown = getAreaBorderDown(area+1);
        areas[area].step = getStep(areas[area].borderDown);
        sumError += calculateMaxErrorOnArea(areas[area].borderDown, areas[area].borderUp, areas[area].step);
        stepsNumber += getStepCount(areas[area].borderDown, areas[area].borderUp, areas[area].step);
    }

    int stepsPerThread = ceil(stepsNumber / NUM_THREADS);
    int rc;
    pthread_t thread[NUM_THREADS] = {};
    ThreadDataIn threadData[NUM_THREADS] = {};

    int areaDown = 0;
    double xmin = xDown;
    int stepsLeft = 0;
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        threadData[i].areaDown = areaDown;
        threadData[i].xDown = xmin;
        threadData[i].areas = nullptr;
        int steps = (i == NUM_THREADS-1) ? (stepsNumber - (NUM_THREADS-1)*stepsPerThread) : stepsPerThread;
        while (steps > 0)
        {
            int stepsArea = getStepCount(areas[areaDown].borderDown, areas[areaDown].borderUp, areas[areaDown].step);
            stepsArea -= stepsLeft;
            if (steps >= stepsArea)
            {
                steps -= stepsArea;
                areaDown += 1;
                xmin = areas[areaDown].borderDown;
                stepsLeft = 0;
            }
            else
            {
                threadData[i].xUp = areas[areaDown].borderDown + steps * areas[areaDown].step;
                xmin = threadData[i].xUp;
                stepsLeft += steps;
                break;
            }
        }

        threadData[3].xUp = xUp;

        int tareaDown = threadData[i].areaDown;
        int numAreas = areaDown - tareaDown + 1;
        if (areaDown >= areasCount)
        {
            numAreas -= 1;
            areaDown -= 1;
        }

        threadData[i].numAreas = numAreas;
        threadData[i].areas = new Area[numAreas];
        for (int k = tareaDown; k <= areaDown; k++)
            threadData[i].areas[k - tareaDown] = areas[k];

        if ((rc = pthread_create(&thread[i], nullptr, thread_function, (void *)&threadData[i])))
        {
            std::cout << "error: pthread_create, rc: " << rc << '\n';
            return -1;
        }
    }

    void *data;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(thread[i], &data);
        auto *tdata = static_cast<ThreadDataOut *>(data);
        sum += tdata->sum;
        delete tdata;
        delete [] threadData[i].areas;
    }

    delete [] areas;

    std::cout << "int sin(1/x) dx, x=0.001..1: " << sum << '\n';
    std::cout << "Error = " << sumError << '\n';

    return 0;
}