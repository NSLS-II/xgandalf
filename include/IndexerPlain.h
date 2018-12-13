/*
 * IndexerPlain.h
 *
 *  Created on: 07.06.2017
 *      Author: Yaro
 */

#ifndef INDEXERPLAIN_H_
#define INDEXERPLAIN_H_

#include "HillClimbingOptimizer.h"
#include <IndexerBase.h>

class IndexerPlain : public IndexerBase
{
  public:
    enum class SamplingPitch
    {
        extremelyLoose,
        loose,
        standard,
        dense,
        extremelyDense,

        standardWithSeondaryMillerIndices,
        denseWithSeondaryMillerIndices,
        extremelyDenseWithSeondaryMillerIndices
    };

    enum class GradientDescentIterationsCount
    {
        exremelyFew,
        few,
        standard,
        many,
        manyMany,
        extremelyMany,

        custom
    };

    IndexerPlain(const ExperimentSettings& experimentSettings);

    void index(std::vector<Lattice>& assembledLattices, const Eigen::Matrix3Xf& reciprocalPeaks_1_per_A);
    void index(std::vector<Lattice>& assembledLattices, const Eigen::Matrix3Xf& reciprocalPeaks_1_per_A,
               std::vector<int>& peakCountOnLattices);

    void setSamplingPitch(SamplingPitch samplingPitch);
    void setSamplingPitch(float unitPitch, bool coverSecondaryMillerIndices);
    void setRefineWithExactLattice(bool flag);

    void setGradientDescentIterationsCount(GradientDescentIterationsCount gradientDescentIterationsCount);

  private:
    void precompute();
    void reducePeakCount(Eigen::Matrix3Xf& reciprocalPeaks_1_per_A);

    Eigen::Matrix3Xf precomputedSamplePoints;

    HillClimbingOptimizer hillClimbingOptimizer;
    SparsePeakFinder sparsePeakFinder;
    InverseSpaceTransform inverseSpaceTransform;

    float maxCloseToPointDeviation;

    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_global;
    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_additionalGlobal;
    HillClimbingOptimizer::hillClimbingAccuracyConstants_t hillClimbing_accuracyConstants_peaks;
    LatticeAssembler::accuracyConstants_t accuracyConstants_LatticeAssembler;
};

#endif /* INDEXERPLAIN_H_ */
