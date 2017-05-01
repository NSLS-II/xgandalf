/*
 * ReciprocalSpaceTranslation.cpp
 *
 *  Created on: 01.05.2017
 *      Author: Yaro
 */

#include <ReciprocalSpaceTranslation.h>

using namespace Eigen;
using namespace std;

ReciprocalSpaceTranslation::ReciprocalSpaceTranslation(const ExperimentSettings& experimentSettings)
{
    reciprocal_lambda_1A = experimentSettings.getReciprocalLambda_1A();
    detectorDistance_m = experimentSettings.getDetectorDistance_m();
}

void ReciprocalSpaceTranslation::computeReciprocalPeaksFromDetectorPeaks(Eigen::Matrix3Xf& reciprocalPeaks_A, const Eigen::Matrix2Xf& detectorPeaks_m)
{
    Matrix3Xf backprojectionDirectionVectors(3, reciprocalPeaks_A.cols());
    backprojectionDirectionVectors.row(0).setConstant(detectorDistance_m);
    backprojectionDirectionVectors.block(1, 0, 2, backprojectionDirectionVectors.cols()) = detectorPeaks_m;

    reciprocalPeaks_A = backprojectionDirectionVectors.colwise().normalized() * reciprocal_lambda_1A;
    reciprocalPeaks_A.col(0) -= Matrix3Xf::Constant(3, reciprocalPeaks_A.cols(), reciprocal_lambda_1A);
}
