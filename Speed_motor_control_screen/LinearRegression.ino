#include "Arduino.h"
#include "LinearRegression.h"

LinearRegression::LinearRegression(double min, double max){
    limits = true;
    minX = min;
    maxX = max;
}

LinearRegression::LinearRegression(){
    limits = false;
}

void LinearRegression::learn(double x, double y){
    if(limits){
        if(x < minX){
            return;
        } else if(x > maxX){
            return;
        }
    }
    n++;

    meanX = meanX + ((x-meanX)/n);
    meanX2 = meanX2 + (((x*x)-meanX2)/n);
    varX = meanX2 - (meanX*meanX);

    meanY = meanY + ((y-meanY)/n);
    meanY2 = meanY2 + (((y*y)-meanY2)/n);
    varY = meanY2 - (meanY*meanY);

    meanXY = meanXY + (((x*y)-meanXY)/n);

    covarXY = meanXY - (meanX*meanY);

    if(n>1){
    m = covarXY / varX;
    b = meanY-(m*meanX);
    }
}

double LinearRegression::correlation() {
    double stdX = sqrt(varX);
    double stdY = sqrt(varY);
    double stdXstdY = stdX*stdY;
    double correlation;

    if(stdXstdY == 0){
        correlation = 1;
    } else {
        correlation = covarXY / stdXstdY;
    }
    return correlation;
}

double LinearRegression::calculate(double x) {
    return((m*x) + b);
}

void LinearRegression::getValues(double values[]){
    values[0] = m;
    values[1] = b;
    values[2] = n;
}

int LinearRegression::getTests(){
    return(n);
}

void LinearRegression::reset(){
    meanX = 0;
    meanX2 = 0;
    varX = 0;
    meanY = 0;
    meanY2 = 0;
    varY = 0;
    meanXY = 0;
    covarXY = 0;
    n = 0;
    m = 1;
    b = 0;  
}
