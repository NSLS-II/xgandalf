/*
 * IndexerAutocorrPrefit.cpp
 *
 *  Created on: 08.06.2017
 *      Author: Yaro
 */

#include <IndexerAutocorrPrefit.h>

#include "pointAutocorrelation.h"

using namespace Eigen;
using namespace std;

IndexerAutocorrPrefit::IndexerAutocorrPrefit(const ExperimentSettings& experimentSettings) :
        IndexerBase(experimentSettings)
{
    precompute();
}

IndexerAutocorrPrefit::IndexerAutocorrPrefit(const ExperimentSettings& experimentSettings, const std::string& precomputedSamplePointsPath) :
        IndexerBase(experimentSettings, precomputedSamplePointsPath)
{
    precompute();
}

void IndexerAutocorrPrefit::precompute()
{
    if (experimentSettings.isLatticeParametersKnown()) {
        float unitPitch = 0.05;
        float tolerance = min(0.05f, experimentSettings.getTolerance());

        samplePointsGenerator.getTightGrid(precomputedSamplePoints, unitPitch, tolerance, experimentSettings.getDifferentRealLatticeVectorLengths_A());
    } else {
        float unitPitch = 0.05;
        float minRadius = experimentSettings.getMinRealLatticeVectorLength_A() * 0.98;
        float maxRadius = experimentSettings.getMaxRealLatticeVectorLength_A() * 1.02;

        samplePointsGenerator.getDenseGrid(precomputedSamplePoints, unitPitch, minRadius, maxRadius);
    }

    maxNormInAutocorrelation = experimentSettings.getMaxReciprocalLatticeVectorLength_1A() * 5;
    minNormInAutocorrelation = experimentSettings.getMinReciprocalLatticeVectorLength_1A() * 0.7;
    dbscanEpsilon = experimentSettings.getMinReciprocalLatticeVectorLength_1A() * 0.15;
    dbscan.init(dbscanEpsilon, maxNormInAutocorrelation);

    float minSpacingBetweenPeaks = experimentSettings.getDifferentRealLatticeVectorLengths_A().minCoeff() * 0.3;
    float maxPossiblePointNorm = experimentSettings.getDifferentRealLatticeVectorLengths_A().maxCoeff() * 1.2;
    sparsePeakFinder.precompute(minSpacingBetweenPeaks, maxPossiblePointNorm);

    maxCloseToPeakDeviation = 0.15;
    inverseSpaceTransform = InverseSpaceTransform(maxCloseToPeakDeviation);
}

void IndexerAutocorrPrefit::getGoodAutocorrelationPoints(Matrix3Xf& goodAutocorrelationPoints, RowVectorXf& goodAutocorrelationPointWeights,
        const Matrix3Xf& points, uint32_t maxAutocorrelationPointsCount)
{
    Matrix3Xf autocorrelationPoints;

    getPointAutocorrelation(autocorrelationPoints, points, minNormInAutocorrelation, maxNormInAutocorrelation);

    vector< Dbscan::cluster_t > clusters;
    uint16_t minPoints = 2;
    dbscan.computeClusters(clusters, autocorrelationPoints, minPoints, dbscanEpsilon);

    sort(clusters.begin(), clusters.end(), [&](const Dbscan::cluster_t& i, const Dbscan::cluster_t& j) {return i.size() > j.size();});

    Array< uint8_t, 1, Dynamic > autocorrelationPointIsInCluster(autocorrelationPoints.cols());
    autocorrelationPointIsInCluster.setZero();
    for (auto& cluster : clusters) {
        for (uint32_t index : cluster) {
            autocorrelationPointIsInCluster[index] = 1;
        }
    }
    uint32_t pointsOutsideOfClustersCount = autocorrelationPointIsInCluster.size() - autocorrelationPointIsInCluster.sum();

    goodAutocorrelationPoints.resize(3, min(pointsOutsideOfClustersCount + (uint32_t) clusters.size(), maxAutocorrelationPointsCount));
    goodAutocorrelationPointWeights.resize(goodAutocorrelationPoints.cols());

    uint32_t goodAutocorrelationPointsCount = 0;

    uint32_t clusterMeansToTakeCount = min((uint32_t) goodAutocorrelationPoints.size(), (uint32_t) clusters.size());
    for (; goodAutocorrelationPointsCount < clusterMeansToTakeCount; ++goodAutocorrelationPointsCount) {
        Vector3f sum(0, 0, 0);
        for (uint32_t index : clusters[goodAutocorrelationPointsCount]) {
            sum += autocorrelationPoints.col(index);
        }
        Vector3f mean = sum / clusters[goodAutocorrelationPointsCount].size();

        goodAutocorrelationPoints.col(goodAutocorrelationPointsCount) = mean;
        goodAutocorrelationPointWeights[goodAutocorrelationPointsCount] = clusters[goodAutocorrelationPointsCount].size();
    }

    uint32_t pointsOutsideOfClustersToTakeCount = goodAutocorrelationPoints.cols() - goodAutocorrelationPointsCount;
    if (pointsOutsideOfClustersToTakeCount == 0) {
        return;
    }

    EigenSTL::vector_Vector3f pointsOutsideOfClusters;
    pointsOutsideOfClusters.reserve(pointsOutsideOfClustersCount);
    for (int i = 0; i < autocorrelationPoints.cols(); ++i) {
        if (!autocorrelationPointIsInCluster[i]) {
            pointsOutsideOfClusters.push_back(autocorrelationPoints.col(i));
        }
    }

//    nth_element(pointsOutsideOfClusters.begin(), pointsOutsideOfClusters.begin() + pointsOutsideOfClustersToTakeCount, pointsOutsideOfClusters.end(),
//            [&](const Vector3f& i, const Vector3f& j) {return i.squaredNorm() < j.squaredNorm();});
    sort(pointsOutsideOfClusters.begin(), pointsOutsideOfClusters.end(), /////////////DEBUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                [&](const Vector3f& i, const Vector3f& j) {return i.squaredNorm() < j.squaredNorm();});

    for (uint32_t i = 0; i < pointsOutsideOfClustersToTakeCount; ++i, ++goodAutocorrelationPointsCount) {
        goodAutocorrelationPoints.col(goodAutocorrelationPointsCount) = pointsOutsideOfClusters[i];
        goodAutocorrelationPointWeights[goodAutocorrelationPointsCount] = 1;
    }

    goodAutocorrelationPointWeights.array().cube().sqrt(); //weighten bigger clusters more
}

void IndexerAutocorrPrefit::autocorrPrefit(const Matrix3Xf& reciprocalPeaks_A, Matrix3Xf& samplePoints,
        HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_autocorr)
{
    Matrix3Xf autocorrelationReciprocalPeaks;
    RowVectorXf autocorrelationPointWeights;

    float maxAutocorrelationPointsCount = 20;

    getGoodAutocorrelationPoints(autocorrelationReciprocalPeaks, autocorrelationPointWeights, reciprocalPeaks_A, maxAutocorrelationPointsCount);

    if (autocorrelationReciprocalPeaks.cols() < 5) {
        return;
    }

    hillClimbing_accuracyConstants_autocorr.functionSelection = 1;
    hillClimbing_accuracyConstants_autocorr.optionalFunctionArgument = 1;
    hillClimbingOptimizer.setHillClimbingAccuracyConstants(hillClimbing_accuracyConstants_autocorr);
    hillClimbingOptimizer.performOptimization(autocorrelationReciprocalPeaks, samplePoints);

    Matrix3Xf samplePointsPeaks_autocorr11 = samplePoints;
    RowVectorXf samplePointsPeaksEvaluation_autocorr11 = hillClimbingOptimizer.getLastInverseTransformEvaluation();
    sparsePeakFinder.findPeaks_fast(samplePointsPeaks_autocorr11, samplePointsPeaksEvaluation_autocorr11);

    uint32_t maxToTakeCount_autocorr11 = 100;
    keepSamplePointsWithHighestEvaluation(samplePointsPeaks_autocorr11, samplePointsPeaksEvaluation_autocorr11, maxToTakeCount_autocorr11);

    inverseSpaceTransform.setPointsToTransform(autocorrelationReciprocalPeaks);
    inverseSpaceTransform.setFunctionSelection(9);
    inverseSpaceTransform.setOptionalFunctionArgument(8);
    inverseSpaceTransform.clearLocalTransformFlag();
    inverseSpaceTransform.clearRadialWeightingFlag();
    inverseSpaceTransform.performTransform(samplePoints);

    Matrix3Xf samplePointsPeaks_autocorr98 = samplePoints;
    RowVectorXf samplePointsPeaksEvaluation_autocorr98 = inverseSpaceTransform.getInverseTransformEvaluation();
    sparsePeakFinder.findPeaks_fast(samplePointsPeaks_autocorr98, samplePointsPeaksEvaluation_autocorr98);

    uint32_t maxToTakeCount_autocorr98 = 100;
    keepSamplePointsWithHighestEvaluation(samplePointsPeaks_autocorr98, samplePointsPeaksEvaluation_autocorr98, maxToTakeCount_autocorr98);

    inverseSpaceTransform.setPointsToTransform(reciprocalPeaks_A);
    inverseSpaceTransform.setFunctionSelection(9);
    inverseSpaceTransform.setOptionalFunctionArgument(8);
    inverseSpaceTransform.clearLocalTransformFlag();
    inverseSpaceTransform.clearRadialWeightingFlag();
    inverseSpaceTransform.performTransform(samplePoints);

    Matrix3Xf samplePointsPeaks_standard98 = samplePoints;
    RowVectorXf samplePointsPeaksEvaluation_standard98 = inverseSpaceTransform.getInverseTransformEvaluation();
    sparsePeakFinder.findPeaks_fast(samplePointsPeaks_standard98, samplePointsPeaksEvaluation_standard98);

    uint32_t maxToTakeCount_98 = 100;
    keepSamplePointsWithHighestEvaluation(samplePointsPeaks_standard98, samplePointsPeaksEvaluation_standard98, maxToTakeCount_98);

    uint32_t prefittedSamplePointsCount = samplePointsPeaks_autocorr11.cols() + samplePointsPeaks_autocorr98.cols() + samplePointsPeaks_standard98.cols();
    if (prefittedSamplePointsCount < 3) {
        return;
    }

    samplePoints.resize(3, prefittedSamplePointsCount);
    samplePoints << samplePointsPeaks_autocorr11, samplePointsPeaks_autocorr98, samplePointsPeaks_standard98;
}

void IndexerAutocorrPrefit::index(std::vector< Lattice >& assembledLattices, const Eigen::Matrix2Xf& detectorPeaks_m)
{
    if (precomputedSamplePoints.size() == 0) {
        precompute();
    }

    Matrix3Xf samplePoints = precomputedSamplePoints;

    Matrix3Xf reciprocalPeaks_A;
    detectorToReciprocalSpaceTransform.computeReciprocalPeaksFromDetectorPeaks(reciprocalPeaks_A, detectorPeaks_m);

    //////// autocorr prefit
    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_autocorr;

    hillClimbing_accuracyConstants_autocorr.maxCloseToPeakDeviation = maxCloseToPeakDeviation;

    hillClimbing_accuracyConstants_autocorr.initialIterationCount = 5;
    hillClimbing_accuracyConstants_autocorr.calmDownIterationCount = 3;
    hillClimbing_accuracyConstants_autocorr.calmDownFactor = 0.8;
    hillClimbing_accuracyConstants_autocorr.localFitIterationCount = 3;
    hillClimbing_accuracyConstants_autocorr.localCalmDownIterationCount = 3;
    hillClimbing_accuracyConstants_autocorr.localCalmDownFactor = 0.75;

    hillClimbing_accuracyConstants_autocorr.stepComputationAccuracyConstants.gamma = 0.65;
    hillClimbing_accuracyConstants_autocorr.stepComputationAccuracyConstants.maxStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 10;
    hillClimbing_accuracyConstants_autocorr.stepComputationAccuracyConstants.minStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 200;
    hillClimbing_accuracyConstants_autocorr.stepComputationAccuracyConstants.directionChangeFactor = 1.5;

    autocorrPrefit(reciprocalPeaks_A, samplePoints, hillClimbing_accuracyConstants_autocorr);

    //////// global hill climbing
    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_global;

    hillClimbing_accuracyConstants_global.functionSelection = 1;
    hillClimbing_accuracyConstants_global.optionalFunctionArgument = 1;
    hillClimbing_accuracyConstants_global.maxCloseToPeakDeviation = maxCloseToPeakDeviation;

    hillClimbing_accuracyConstants_global.initialIterationCount = 3;
    hillClimbing_accuracyConstants_global.calmDownIterationCount = 3;
    hillClimbing_accuracyConstants_global.calmDownFactor = 0.7;
    hillClimbing_accuracyConstants_global.localFitIterationCount = 3;
    hillClimbing_accuracyConstants_global.localCalmDownIterationCount = 3;
    hillClimbing_accuracyConstants_global.localCalmDownFactor = 0.7;

    hillClimbing_accuracyConstants_global.stepComputationAccuracyConstants.gamma = 0.65;
    hillClimbing_accuracyConstants_global.stepComputationAccuracyConstants.maxStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 20;
    hillClimbing_accuracyConstants_global.stepComputationAccuracyConstants.minStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 200;
    hillClimbing_accuracyConstants_global.stepComputationAccuracyConstants.directionChangeFactor = 1.5;

    hillClimbingOptimizer.setHillClimbingAccuracyConstants(hillClimbing_accuracyConstants_global);
    hillClimbingOptimizer.performOptimization(reciprocalPeaks_A, samplePoints);

    uint32_t maxPeaksToTakeCount = 50;
    sparsePeakFinder.findPeaks_fast(samplePoints, hillClimbingOptimizer.getLastInverseTransformEvaluation());
    keepSamplePointsWithHighestEvaluation(samplePoints, hillClimbingOptimizer.getLastInverseTransformEvaluation(), maxPeaksToTakeCount);

    /////// peaks hill climbing
    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_peaks;

    hillClimbing_accuracyConstants_peaks.functionSelection = 9;
    hillClimbing_accuracyConstants_peaks.optionalFunctionArgument = 8;
    hillClimbing_accuracyConstants_peaks.maxCloseToPeakDeviation = maxCloseToPeakDeviation;

    hillClimbing_accuracyConstants_peaks.initialIterationCount = 0;
    hillClimbing_accuracyConstants_peaks.calmDownIterationCount = 0;
    hillClimbing_accuracyConstants_peaks.calmDownFactor = 0;
    hillClimbing_accuracyConstants_peaks.localFitIterationCount = 10;
    hillClimbing_accuracyConstants_peaks.localCalmDownIterationCount = 20;
    hillClimbing_accuracyConstants_peaks.localCalmDownFactor = 0.85;

    hillClimbing_accuracyConstants_peaks.stepComputationAccuracyConstants.gamma = 0.1;
    hillClimbing_accuracyConstants_peaks.stepComputationAccuracyConstants.maxStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 2000;
    hillClimbing_accuracyConstants_peaks.stepComputationAccuracyConstants.minStep = experimentSettings.getDifferentRealLatticeVectorLengths_A().mean() / 20000;
    hillClimbing_accuracyConstants_peaks.stepComputationAccuracyConstants.directionChangeFactor = 2.5;

    hillClimbingOptimizer.setHillClimbingAccuracyConstants(hillClimbing_accuracyConstants_peaks);
    hillClimbingOptimizer.performOptimization(reciprocalPeaks_A, samplePoints);

    /////// assemble lattices
    inverseSpaceTransform.setPointsToTransform(reciprocalPeaks_A);
    inverseSpaceTransform.setFunctionSelection(9);
    inverseSpaceTransform.setOptionalFunctionArgument(8);
    inverseSpaceTransform.setLocalTransformFlag();
    inverseSpaceTransform.clearRadialWeightingFlag();
    inverseSpaceTransform.performTransform(samplePoints);

    LatticeAssembler::accuracyConstants_t accuracyConstants_LatticeAssembler;
    accuracyConstants_LatticeAssembler.maxCountGlobalPassingWeightFilter = 500;
    accuracyConstants_LatticeAssembler.maxCountLocalPassingWeightFilter = 15;
    accuracyConstants_LatticeAssembler.maxCountPassingRelativeDefectFilter = 50;
    accuracyConstants_LatticeAssembler.minPointsOnLattice = 5;

    //    latticeAssembler.setDeterminantRange(experimentSettings.getMinRealLatticeDeterminant_A3(), experimentSettings.getMaxRealLatticeDeterminant_A3());
    latticeAssembler.setDeterminantRange(experimentSettings.getRealLatticeDeterminant_A3() * 0.8, experimentSettings.getRealLatticeDeterminant_A3() * 1.2);

    latticeAssembler.setAccuracyConstants(accuracyConstants_LatticeAssembler);

    vector< LatticeAssembler::assembledLatticeStatistics_t > assembledLatticesStatistics;
    Matrix3Xf& candidateVectors = samplePoints;
    RowVectorXf& candidateVectorWeights = inverseSpaceTransform.getInverseTransformEvaluation();
    vector< vector< uint16_t > >& pointIndicesOnVector = inverseSpaceTransform.getPointsCloseToEvaluationPositions_indices();
    latticeAssembler.assembleLattices(assembledLattices, assembledLatticesStatistics, candidateVectors,
            candidateVectorWeights, pointIndicesOnVector, reciprocalPeaks_A);

}
