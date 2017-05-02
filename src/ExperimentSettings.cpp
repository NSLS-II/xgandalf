/*
 * experimentSettings.cpp
 *
 *  Created on: 09.04.2017
 *      Author: Yaro
 */

#include <assert.h>
#include <ExperimentSettings.h>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace std;
using namespace Eigen;

ExperimentSettings::ExperimentSettings(float coffset_m, float clen_mm, float beamEenergy_eV, float divergenceAngle_deg, float nonMonochromaticity,
        float pixelLength_m, float detectorRadius_pixel, float minRealLatticeVectorLength_A, float maxRealLatticeVectorLength_A) :
        latticeParametersKnown(false), minRealLatticeVectorLength_A(minRealLatticeVectorLength_A), maxRealLatticeVectorLength_A(maxRealLatticeVectorLength_A)
{
    constructFromGeometryFileValues(coffset_m, clen_mm, beamEenergy_eV, divergenceAngle_deg, nonMonochromaticity, pixelLength_m, detectorRadius_pixel);

    minRealLatticeDeterminant_A3 = pow(minRealLatticeVectorLength_A, 3);
    maxRealLatticeDeterminant_A3 = pow(maxRealLatticeVectorLength_A, 3);

    minReciprocalLatticeVectorLength_1A = 1 / maxRealLatticeVectorLength_A;
    maxReciprocalLatticeVectorLength_1A = 1 / minRealLatticeVectorLength_A;
    minReciprocalLatticeDeterminant_1A3 = 1 / maxRealLatticeDeterminant_A3;
    maxReciprocalLatticeDeterminant_1A3 = 1 / minRealLatticeDeterminant_A3;

    differentRealLatticeVectorLengths_A.resize(2);
    differentRealLatticeVectorLengths_A[0] = minRealLatticeVectorLength_A;
    differentRealLatticeVectorLengths_A[1] = maxRealLatticeVectorLength_A;
}

ExperimentSettings::ExperimentSettings(float detectorDistance_m, float detectorRadius_m, float divergenceAngle_deg,
        float nonMonochromaticity, float minRealLatticeVectorLength_A, float maxRealLatticeVectorLength_A) :
        latticeParametersKnown(false), minRealLatticeVectorLength_A(minRealLatticeVectorLength_A), maxRealLatticeVectorLength_A(maxRealLatticeVectorLength_A)
{
    constructFromPrecomputedCalues(detectorDistance_m, detectorRadius_m, divergenceAngle_deg, nonMonochromaticity);

    minRealLatticeDeterminant_A3 = pow(minRealLatticeVectorLength_A, 3);
    maxRealLatticeDeterminant_A3 = pow(maxRealLatticeVectorLength_A, 3);

    minReciprocalLatticeVectorLength_1A = 1 / maxRealLatticeVectorLength_A;
    maxReciprocalLatticeVectorLength_1A = 1 / minRealLatticeVectorLength_A;
    minReciprocalLatticeDeterminant_1A3 = 1 / maxRealLatticeDeterminant_A3;
    maxReciprocalLatticeDeterminant_1A3 = 1 / minRealLatticeDeterminant_A3;

    differentRealLatticeVectorLengths_A.resize(2);
    differentRealLatticeVectorLengths_A[0] = minRealLatticeVectorLength_A;
    differentRealLatticeVectorLengths_A[1] = maxRealLatticeVectorLength_A;
}

ExperimentSettings::ExperimentSettings(float coffset_m, float clen_mm, float beamEenergy_eV, float divergenceAngle_deg, float nonMonochromaticity,
        float pixelLength_m, float detectorRadius_pixel, float minRealLatticeVectorLength_A, float maxRealLatticeVectorLength_A,
        float minRealLatticeDeterminant_A3, float maxRealLatticeDeterminant_A3) :
        latticeParametersKnown(false), minRealLatticeVectorLength_A(minRealLatticeVectorLength_A), maxRealLatticeVectorLength_A(maxRealLatticeVectorLength_A),
                minRealLatticeDeterminant_A3(minRealLatticeDeterminant_A3), maxRealLatticeDeterminant_A3(maxRealLatticeDeterminant_A3)
{
    constructFromGeometryFileValues(coffset_m, clen_mm, beamEenergy_eV, divergenceAngle_deg, nonMonochromaticity, pixelLength_m, detectorRadius_pixel);

    minReciprocalLatticeVectorLength_1A = 1 / maxRealLatticeVectorLength_A;
    maxReciprocalLatticeVectorLength_1A = 1 / minRealLatticeVectorLength_A;
    minReciprocalLatticeDeterminant_1A3 = 1 / maxRealLatticeDeterminant_A3;
    maxReciprocalLatticeDeterminant_1A3 = 1 / minRealLatticeDeterminant_A3;

    differentRealLatticeVectorLengths_A.resize(2);
    differentRealLatticeVectorLengths_A[0] = minRealLatticeVectorLength_A;
    differentRealLatticeVectorLengths_A[1] = maxRealLatticeVectorLength_A;
}

ExperimentSettings::ExperimentSettings(float detectorDistance_m, float detectorRadius_m, float divergenceAngle_deg, float nonMonochromaticity,
        float minRealLatticeVectorLength_A, float maxRealLatticeVectorLength_A, float minRealLatticeDeterminant_A3, float maxRealLatticeDeterminant_A3) :
        latticeParametersKnown(false), minRealLatticeVectorLength_A(minRealLatticeVectorLength_A), maxRealLatticeVectorLength_A(maxRealLatticeVectorLength_A),
                minRealLatticeDeterminant_A3(minRealLatticeDeterminant_A3), maxRealLatticeDeterminant_A3(maxRealLatticeDeterminant_A3)
{
    constructFromPrecomputedCalues(detectorDistance_m, detectorRadius_m, divergenceAngle_deg, nonMonochromaticity);

    minReciprocalLatticeVectorLength_1A = 1 / maxRealLatticeVectorLength_A;
    maxReciprocalLatticeVectorLength_1A = 1 / minRealLatticeVectorLength_A;
    minReciprocalLatticeDeterminant_1A3 = 1 / maxRealLatticeDeterminant_A3;
    maxReciprocalLatticeDeterminant_1A3 = 1 / minRealLatticeDeterminant_A3;

    differentRealLatticeVectorLengths_A.resize(2);
    differentRealLatticeVectorLengths_A[0] = minRealLatticeVectorLength_A;
    differentRealLatticeVectorLengths_A[1] = maxRealLatticeVectorLength_A;
}

ExperimentSettings::ExperimentSettings(float coffset_m, float clen_mm, float beamEenergy_eV, float divergenceAngle_deg, float nonMonochromaticity,
        float pixelLength_m, float detectorRadius_pixel, const Lattice& sampleReciprocalLattice_1A) :
        latticeParametersKnown(true), sampleReciprocalLattice_1A(sampleReciprocalLattice_1A)
{
    constructFromGeometryFileValues(coffset_m, clen_mm, beamEenergy_eV, divergenceAngle_deg, nonMonochromaticity, pixelLength_m, detectorRadius_pixel);
    deduceValuesFromSampleReciprocalLattice();

}

ExperimentSettings::ExperimentSettings(float detectorDistance_m, float detectorRadius_m, float divergenceAngle_deg, float nonMonochromaticity,
        const Lattice& sampleReciprocalLattice_1A) :
        latticeParametersKnown(true), sampleReciprocalLattice_1A(sampleReciprocalLattice_1A)
{
    constructFromPrecomputedCalues(detectorDistance_m, detectorRadius_m, divergenceAngle_deg, nonMonochromaticity);
    deduceValuesFromSampleReciprocalLattice();
}

void ExperimentSettings::deduceValuesFromSampleReciprocalLattice()
{
    sampleReciprocalLattice_1A.minimize();

    sampleRealLattice_A = this->sampleReciprocalLattice_1A.getReciprocalLattice().minimize();

    realLatticeVectorLengths_A = sampleRealLattice_A.getBasisVectorNorms();
    realLatticeVectorAngles_rad = sampleRealLattice_A.getBasisVectorAngles();
    realLatticeDeterminant_A3 = sampleRealLattice_A.det();
    reciprocalLatticeVectorLengths_1A = this->sampleReciprocalLattice_1A.getBasisVectorNorms();
    reciprocalLatticeVectorAngles_rad = this->sampleReciprocalLattice_1A.getBasisVectorAngles();
    reciprocalLatticeDeterminant_1A3 = this->sampleReciprocalLattice_1A.det();

    //norms are ordered due to minimization of matrix
    assert(reciprocalLatticeVectorLengths_1A[0] <= reciprocalLatticeVectorLengths_1A[1]
            && reciprocalLatticeVectorLengths_1A[1] <= reciprocalLatticeVectorLengths_1A[2]);
    assert(realLatticeVectorLengths_A[0] <= realLatticeVectorLengths_A[1] && realLatticeVectorLengths_A[1] <= realLatticeVectorLengths_A[2]);

    float minSimilarityFactor = 0.96;
    if (reciprocalLatticeVectorLengths_1A[0] / reciprocalLatticeVectorLengths_1A[1] > minSimilarityFactor) {
        if (reciprocalLatticeVectorLengths_1A[1] / reciprocalLatticeVectorLengths_1A[2] > minSimilarityFactor) {
            differentRealLatticeVectorLengths_A.resize(1);
            differentRealLatticeVectorLengths_A[0] = reciprocalLatticeVectorLengths_1A.mean();
        } else {
            differentRealLatticeVectorLengths_A.resize(2);
            differentRealLatticeVectorLengths_A[0] = reciprocalLatticeVectorLengths_1A.head(2).mean();
            differentRealLatticeVectorLengths_A[1] = reciprocalLatticeVectorLengths_1A[2];
        }
    } else if (reciprocalLatticeVectorLengths_1A[1] / reciprocalLatticeVectorLengths_1A[2] > minSimilarityFactor) {
        differentRealLatticeVectorLengths_A.resize(2);
        differentRealLatticeVectorLengths_A[0] = reciprocalLatticeVectorLengths_1A[0];
        differentRealLatticeVectorLengths_A[1] = reciprocalLatticeVectorLengths_1A.tail(2).mean();
    } else {
        differentRealLatticeVectorLengths_A = reciprocalLatticeVectorLengths_1A;
    }
}

void ExperimentSettings::constructFromGeometryFileValues(float coffset_m, float clen_mm, float beamEenergy_eV, float divergenceAngle_deg,
        float nonMonochromaticity, float pixelLength_m, float detectorRadius_pixel)
{
    this->nonMonochromaticity = nonMonochromaticity;

    detectorDistance_m = clen_mm * 1e-3 + coffset_m;
    detectorRadius_m = detectorRadius_pixel * pixelLength_m;
    divergenceAngle_rad = divergenceAngle_deg * M_PI / 180;
    maxResolutionAngle_rad = atan(detectorRadius_m / detectorDistance_m);

    float h_Plank = 4.135667662e-15; //Planck constant [eV*s]
    float c_light = 299792458; //speed of light [m/s]
    lambda_A = h_Plank * c_light / beamEenergy_eV * 1e10;
    lambdaShort_A = lambda_A * (1 - nonMonochromaticity / 2);
    lambdaLong_A = lambda_A * (1 + nonMonochromaticity / 2);

    reciprocal_lambda_1A = 1 / lambda_A;
    reciprocal_lambdaShort_1A = 1 / lambdaShort_A;
    reciprocal_lambdaLong_1A = 1 / lambdaLong_A;
}

void ExperimentSettings::constructFromPrecomputedCalues(float detectorDistance_m, float detectorRadius_m, float divergenceAngle_deg,
        float nonMonochromaticity)
{
    this->detectorDistance_m = detectorDistance_m;
    this->detectorRadius_m = detectorRadius_m;
    divergenceAngle_rad = divergenceAngle_deg * M_PI / 180;
    this->nonMonochromaticity = nonMonochromaticity;
}

float ExperimentSettings::getDetectorDistance_m() const
{
    return detectorDistance_m;
}

float ExperimentSettings::getDetectorRadius_m() const
{
    return detectorRadius_m;
}

const Eigen::VectorXf& ExperimentSettings::getDifferentRealLatticeVectorLengths_A() const
{
    return differentRealLatticeVectorLengths_A;
}

float ExperimentSettings::getDivergenceAngle_rad() const
{
    return divergenceAngle_rad;
}

float ExperimentSettings::getLambda_A() const
{
    return lambda_A;
}

float ExperimentSettings::getLambdaLong_A() const
{
    return lambdaLong_A;
}

float ExperimentSettings::getLambdaShort_A() const
{
    return lambdaShort_A;
}

bool ExperimentSettings::isLatticeParametersKnown() const
{
    return latticeParametersKnown;
}

float ExperimentSettings::getMaxRealLatticeDeterminant_A3() const
{
    return maxRealLatticeDeterminant_A3;
}

float ExperimentSettings::getMaxRealLatticeVectorLength_A() const
{
    return maxRealLatticeVectorLength_A;
}

float ExperimentSettings::getMaxReciprocalLatticeDeterminant_1A3() const
{
    return maxReciprocalLatticeDeterminant_1A3;
}

float ExperimentSettings::getMaxReciprocalLatticeVectorLength_1A() const
{
    return maxReciprocalLatticeVectorLength_1A;
}

float ExperimentSettings::getMaxResolutionAngle_rad() const
{
    return maxResolutionAngle_rad;
}

float ExperimentSettings::getMinRealLatticeDeterminant_A3() const
{
    return minRealLatticeDeterminant_A3;
}

float ExperimentSettings::getMinRealLatticeVectorLength_A() const
{
    return minRealLatticeVectorLength_A;
}

float ExperimentSettings::getMinReciprocalLatticeDeterminant_1A3() const
{
    return minReciprocalLatticeDeterminant_1A3;
}

float ExperimentSettings::getMinReciprocalLatticeVectorLength_1A() const
{
    return minReciprocalLatticeVectorLength_1A;
}

float ExperimentSettings::getNonMonochromaticity() const
{
    return nonMonochromaticity;
}

float ExperimentSettings::getRealLatticeDeterminant_A3() const
{
    return realLatticeDeterminant_A3;
}

const Eigen::Vector3f& ExperimentSettings::getRealLatticeVectorAngles_rad() const
{
    return realLatticeVectorAngles_rad;
}

const Eigen::Vector3f& ExperimentSettings::getRealLatticeVectorLengths_A() const
{
    return realLatticeVectorLengths_A;
}

float ExperimentSettings::getReciprocalLambda_1A() const
{
    return reciprocal_lambda_1A;
}

float ExperimentSettings::getReciprocalLambdaLong_1A() const
{
    return reciprocal_lambdaLong_1A;
}

float ExperimentSettings::getReciprocalLambdaShort_1A() const
{
    return reciprocal_lambdaShort_1A;
}

float ExperimentSettings::getReciprocalLatticeDeterminant_1A3() const
{
    return reciprocalLatticeDeterminant_1A3;
}

const Eigen::Vector3f& ExperimentSettings::getReciprocalLatticeVectorAngles_rad() const
{
    return reciprocalLatticeVectorAngles_rad;
}

const Eigen::Vector3f& ExperimentSettings::getReciprocalLatticeVectorLengths_1A() const
{
    return reciprocalLatticeVectorLengths_1A;
}

const Lattice& ExperimentSettings::getSampleRealLattice_A() const
{
    return sampleRealLattice_A;
}

const Lattice& ExperimentSettings::getSampleReciprocalLattice_1A() const
{
    return sampleReciprocalLattice_1A;
}