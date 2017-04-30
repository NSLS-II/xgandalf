/*
 * LatticeAssembler.cpp
 *
 *  Created on: 22.04.2017
 *      Author: Yaro
 */

#include <LatticeAssembler.h>
#include <ctype.h>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>

#include <iostream>

using namespace Eigen;
using namespace std;

LatticeAssembler::LatticeAssembler(Vector2f& determinantRange) :
        determinantRange(determinantRange)
{
    accuracyConstants.maxCountGlobalPassingWeightFilter = 500;
    accuracyConstants.maxCountLocalPassingWeightFilter = 15;
    accuracyConstants.maxCountPassingRelativeDefectFilter = 50;

    accuracyConstants.minPointsOnLattice = 5;
}

LatticeAssembler::LatticeAssembler(Vector2f& determinantRange, accuracyConstants_t accuracyConstants) :
        determinantRange(determinantRange), accuracyConstants(accuracyConstants)
{
}

void LatticeAssembler::assembleLattices(vector< Lattice >& assembledLattices, Matrix3Xf& candidateVectors, RowVectorXf& candidateVectorWeights,
        vector< vector< uint16_t > >& pointIndicesOnVector, Matrix3Xf& pointsToFitInReciprocalSpace)
{
    vector< assembledLatticeStatistics_t > assembledLatticesStatistics;
    assembleLattices(assembledLattices, assembledLatticesStatistics, candidateVectors, candidateVectorWeights, pointIndicesOnVector,
            pointsToFitInReciprocalSpace);
}

void LatticeAssembler::assembleLattices(vector< Lattice >& assembledLattices, vector< assembledLatticeStatistics_t >& assembledLatticesStatistics,
        Matrix3Xf& candidateVectors, RowVectorXf& candidateVectorWeights, vector< vector< uint16_t > >& pointIndicesOnVector,
        Matrix3Xf& pointsToFitInReciprocalSpace)
{
    computeCandidateLattices(candidateVectors, candidateVectorWeights, pointIndicesOnVector);

    list< candidateLattice_t > finalCandidateLattices;

    filterCandidateLatticesByWeight(accuracyConstants.maxCountGlobalPassingWeightFilter);

    for (uint32_t i = 0; i < candidateLattices.size(); ++i) {
        auto& candidateLattice = candidateLattices[i];
        candidateLattice.realSpaceLattice.minimize();
        candidateLattice.det = abs(candidateLattice.realSpaceLattice.det());
        computeAssembledLatticeStatistics(candidateLattice, pointsToFitInReciprocalSpace);
    };

    finalCandidateLattices.insert(finalCandidateLattices.end(), candidateLattices.begin(),
            candidateLattices.begin() + accuracyConstants.maxCountLocalPassingWeightFilter); // assume that candidateVectors is sorted descending for weight!

    filterCandidateBasesByMeanRelativeDefect(accuracyConstants.maxCountPassingRelativeDefectFilter);

    finalCandidateLattices.insert(finalCandidateLattices.end(), candidateLattices.begin(),
            candidateLattices.end());

    selectBestLattices(assembledLattices, assembledLatticesStatistics, finalCandidateLattices);
}

void LatticeAssembler::selectBestLattices(vector< Lattice >& assembledLattices, vector< assembledLatticeStatistics_t >& assembledLatticesStatistics,
        list< candidateLattice_t >& finalCandidateLattices)
{
    assembledLattices.clear();
    if (finalCandidateLattices.size() == 0) {
        return;
    }

    finalCandidateLattices.sort([&](candidateLattice_t i, candidateLattice_t j) {return i.pointOnLatticeIndices.size() > j.pointOnLatticeIndices.size();}); //descending

    float significantDetReductionFactor = 0.75;
    float significantPointCountReductionFactor = 0.85;
    float significantMeanDefectReductionFactor = 0.7;
    float significantMeanRelativeDefectReductionFactor = 0.8;

    vector< uint16_t > combinedPointIndicesOnSelectedLattices;
    vector< uint16_t > tmp_indices;
    tmp_indices.reserve(4000); //just for speed
    combinedPointIndicesOnSelectedLattices.reserve(1000); //just for speed

    auto bestCandidateLattice = finalCandidateLattices.begin();
    auto nextCandidateLattice = finalCandidateLattices.begin();
    while (bestCandidateLattice != prev(finalCandidateLattices.end()) && bestCandidateLattice != finalCandidateLattices.end()) {
        tmp_indices.clear();
        set_union(combinedPointIndicesOnSelectedLattices.begin(), combinedPointIndicesOnSelectedLattices.end(),
                bestCandidateLattice->pointOnLatticeIndices.begin(), bestCandidateLattice->pointOnLatticeIndices.end(),
                back_inserter(tmp_indices));   // first resizing to take all elements and then rezizing to right size may be slightly faster than back-inserter 
        combinedPointIndicesOnSelectedLattices.swap(tmp_indices);

        nextCandidateLattice = next(bestCandidateLattice);
        while (nextCandidateLattice != finalCandidateLattices.end()) {
            tmp_indices.clear();
            set_difference(nextCandidateLattice->pointOnLatticeIndices.begin(), nextCandidateLattice->pointOnLatticeIndices.end(),
                    combinedPointIndicesOnSelectedLattices.begin(), combinedPointIndicesOnSelectedLattices.end(),
                    back_inserter(tmp_indices));
            uint16_t uniquelyReachedNodesCount = tmp_indices.size();
            if (uniquelyReachedNodesCount >= accuracyConstants.minPointsOnLattice) { //enough new points on lattice => lattice cannot be rejected
                ++nextCandidateLattice;
            } else if (nextCandidateLattice->pointOnLatticeIndices.size()
                    > bestCandidateLattice->pointOnLatticeIndices.size() * significantPointCountReductionFactor) { //subset of previous lattice + zero to few points; Not significantly fewer points than current lattice
                if (
                (nextCandidateLattice->det <= bestCandidateLattice->det * significantDetReductionFactor
                        && nextCandidateLattice->assembledLatticeStatistics.meanDefect
                                * min(significantMeanDefectReductionFactor, 1.5f / (bestCandidateLattice->det / nextCandidateLattice->det))
                                < bestCandidateLattice->assembledLatticeStatistics.meanDefect
                        && nextCandidateLattice->assembledLatticeStatistics.meanRelativeDefect * significantMeanRelativeDefectReductionFactor
                                < bestCandidateLattice->assembledLatticeStatistics.meanRelativeDefect)
                        ||
                        (nextCandidateLattice->det * significantDetReductionFactor <= bestCandidateLattice->det
                                && nextCandidateLattice->assembledLatticeStatistics.meanDefect < bestCandidateLattice->assembledLatticeStatistics.meanDefect)
                        ||
                        (nextCandidateLattice->assembledLatticeStatistics.meanDefect
                                < bestCandidateLattice->assembledLatticeStatistics.meanDefect
                                        * min(significantMeanDefectReductionFactor, 1.5f / (nextCandidateLattice->det / bestCandidateLattice->det))
                                && nextCandidateLattice->assembledLatticeStatistics.meanRelativeDefect
                                        < bestCandidateLattice->assembledLatticeStatistics.meanRelativeDefect * significantMeanRelativeDefectReductionFactor)
                        ) {
                    auto temp = next(nextCandidateLattice);
                    finalCandidateLattices.splice(next(bestCandidateLattice), finalCandidateLattices, nextCandidateLattice);
                    bestCandidateLattice = finalCandidateLattices.erase(bestCandidateLattice);
                    nextCandidateLattice = temp;
                } else {
                    nextCandidateLattice = finalCandidateLattices.erase(nextCandidateLattice);
                }
            } else {
                nextCandidateLattice = finalCandidateLattices.erase(nextCandidateLattice);
            }
        }
        ++bestCandidateLattice;
    }

    assembledLattices.reserve(finalCandidateLattices.size());
    for (auto& selectedCandidateLattice : finalCandidateLattices) {
        assembledLattices.push_back(selectedCandidateLattice.realSpaceLattice);
        assembledLatticesStatistics.push_back(selectedCandidateLattice.assembledLatticeStatistics);
    }

}

uint16_t LatticeAssembler::countUniqueColumns(const Matrix3Xf& millerIndices)
{
    uint16_t count = 0;

    sortIndices.resize(millerIndices.cols());
    iota(sortIndices.begin(), sortIndices.end(), 0);
    sort(sortIndices.begin(), sortIndices.end(), [&millerIndices](uint16_t i, uint16_t j) {return millerIndices(0,i) < millerIndices(0,j);});

    Matrix3Xf millerIndices_sorted(3, millerIndices.cols() + 1);
    for (int i = 0; i < millerIndices.cols(); ++i) {
        millerIndices_sorted.col(i) = millerIndices.col(sortIndices[i]);
    }
    millerIndices_sorted(0, millerIndices.cols()) = 0.5f; //not a valid miller index! This saves an out of bounds check in a later loop

    for (int i = 0; i < millerIndices.cols(); ++i) {
        bool isUnique = true;
        for (int j = i + 1; millerIndices_sorted(0, i) == millerIndices_sorted(0, j); j++) {
            if ((millerIndices_sorted(1, i) == millerIndices_sorted(1, j)) & (millerIndices_sorted(2, i) == millerIndices_sorted(2, j))) {
                isUnique = false;
                break;
            }
        }
        if (isUnique) {
            count++;
        }
    }

    return count;
}

void LatticeAssembler::computeCandidateLattices(Matrix3Xf candidateVectors, RowVectorXf candidateVectorWeights,
        vector< vector< uint16_t > > pointIndicesOnVector)
{
//hand-crafted remove-if for three variables. Delete
    for (int i = candidateVectors.cols() - 1; i >= 0; i--) {
        if (pointIndicesOnVector[i].size() < accuracyConstants.minPointsOnLattice) {
            pointIndicesOnVector[i] = pointIndicesOnVector.back();
            pointIndicesOnVector.pop_back();

            candidateVectors.col(i) = candidateVectors.col(candidateVectors.cols() - 1);
            candidateVectors.conservativeResize(NoChange, candidateVectors.cols() - 1); //possibly slow, because of call to realloc. can be avoided if needed

            candidateVectorWeights[i] = candidateVectorWeights[candidateVectorWeights.size() - 1];
            candidateVectorWeights.conservativeResize(candidateVectorWeights.size() - 1);
        }
    }

    for_each(pointIndicesOnVector.begin(), pointIndicesOnVector.end(), [](vector< uint16_t >& v) {sort(v.begin(),v.end());}); //needed for easier intersection computation later

    candidateLattices.reserve(10000);
    int candidateVectorsCount = candidateVectors.cols();
    for (uint16_t i = 0; i < candidateVectorsCount - 2; ++i) {
        for (uint16_t j = (i + 1); j < candidateVectorsCount - 1; ++j) {
            for (uint16_t k = (j + 1); k < candidateVectorsCount; ++k) {
                if (i == 14 && j == 21 && k == 49) {  //DEBUG!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    continue;
                }

                Lattice latticeToCheck(candidateVectors.col(i), candidateVectors.col(j), candidateVectors.col(k));

                float absDet = abs(latticeToCheck.det());
                if ((absDet < determinantRange[0]) | (absDet > determinantRange[1])) {
                    continue;
                }

                vector< uint16_t > pointIndicesOnTwoVectors(max(pointIndicesOnVector[i].size(), pointIndicesOnVector[j].size()));
                auto it = set_intersection(pointIndicesOnVector[i].begin(), pointIndicesOnVector[i].end(),
                        pointIndicesOnVector[j].begin(), pointIndicesOnVector[j].end(),
                        pointIndicesOnTwoVectors.begin());
                uint16_t pointsOnBothVectorsCount = it - pointIndicesOnTwoVectors.begin();
                if (pointsOnBothVectorsCount < accuracyConstants.minPointsOnLattice) {
                    continue;
                }
                pointIndicesOnTwoVectors.resize(pointsOnBothVectorsCount);

                vector< uint16_t > pointIndicesOnLatticeToCheck(max(pointIndicesOnTwoVectors.size(), pointIndicesOnVector[k].size()));
                it = set_intersection(pointIndicesOnTwoVectors.begin(), pointIndicesOnTwoVectors.end(),
                        pointIndicesOnVector[k].begin(), pointIndicesOnVector[k].end(),
                        pointIndicesOnLatticeToCheck.begin());
                uint16_t pointsOnLatticeToCheckCount = it - pointIndicesOnLatticeToCheck.begin();
                if (pointsOnLatticeToCheckCount < accuracyConstants.minPointsOnLattice) {
                    continue;
                }
                pointIndicesOnLatticeToCheck.resize(pointsOnLatticeToCheckCount);

                candidateLattices.resize(candidateLattices.size() + 1);
                auto& newCandidateBasis = candidateLattices.back();
                newCandidateBasis.realSpaceLattice = latticeToCheck;
                newCandidateBasis.weight = candidateVectorWeights[i] + candidateVectorWeights[j] + candidateVectorWeights[k];
                newCandidateBasis.pointOnLatticeIndices.swap(pointIndicesOnLatticeToCheck);
                newCandidateBasis.vectorIndices = {i,j,k};
            }
        }
    }
}

void LatticeAssembler::computeAssembledLatticeStatistics(candidateLattice_t& candidateLattice, const Matrix3Xf& pointsToFitInReciprocalSpace)
{
    auto& pointOnLatticeIndices = candidateLattice.pointOnLatticeIndices;
    Matrix3Xf currentPointsToFitInReciprocalSpace(3, pointOnLatticeIndices.size());
    for (uint32_t j = 0; j < pointOnLatticeIndices.size(); ++j) {
        currentPointsToFitInReciprocalSpace.col(j) = pointsToFitInReciprocalSpace.col(pointOnLatticeIndices[j]);
    }

//    auto reciprocalBasis = candidateLattice.realSpaceLattice.getBasis().transpose().inverse();
//    Matrix3Xf factorsToReachPoints = reciprocalBasis.inverse()*pointsToFitInInverseSpace;
    Matrix3Xf factorsToReachPoints = candidateLattice.realSpaceLattice.getBasis().transpose() * currentPointsToFitInReciprocalSpace; //realSpaceLattice is inverse of the transpose of the reciprocal basis. Inverse of the reciprocal basis is needed => transpose!

    Matrix3Xf millerIndices = factorsToReachPoints.array().round();

    candidateLattice.assembledLatticeStatistics.occupiedLatticePointsCount = countUniqueColumns(millerIndices);

    auto predictedPoints = candidateLattice.realSpaceLattice.getBasis().transpose().inverse() * millerIndices;

//    cout << predictedPoints << endl << endl;
//    cout << (predictedPoints - currentPointsToFitInReciprocalSpace).eval() << endl << endl;
//    cout << ((predictedPoints - currentPointsToFitInReciprocalSpace).colwise().norm()).eval() << endl << endl;

    candidateLattice.assembledLatticeStatistics.meanDefect = ((predictedPoints - currentPointsToFitInReciprocalSpace).colwise().norm()).mean();
    candidateLattice.assembledLatticeStatistics.meanRelativeDefect = ((factorsToReachPoints - millerIndices).colwise().norm()).mean();
}

void LatticeAssembler::filterCandidateLatticesByWeight(uint32_t maxToTake)
{
//    auto& candidateLattices = this->candidateLattices;  //needed for lambda function

    uint32_t toTakeCount = min(maxToTake, (uint32_t) candidateLattices.size());

    sortIndices.resize(candidateLattices.size());
    iota(sortIndices.begin(), sortIndices.end(), 0);
    nth_element(sortIndices.begin(), sortIndices.begin() + toTakeCount - 1, sortIndices.end(),
            [&](uint32_t i, uint32_t j) {return candidateLattices[i].weight > candidateLattices[j].weight;});  //descending

    sortIndices.resize(toTakeCount);
    sort(sortIndices.begin(), sortIndices.end(),
            [&](uint32_t i, uint32_t j) {return candidateLattices[i].weight > candidateLattices[j].weight;});  //descending

    vector< candidateLattice_t > candidateLatticesFiltered;
    candidateLatticesFiltered.resize(toTakeCount);
    for (uint32_t i = 0; i < toTakeCount; ++i) {
        candidateLatticesFiltered[i] = candidateLattices[sortIndices[i]];
    }

    candidateLattices.swap(candidateLatticesFiltered);
}

void LatticeAssembler::filterCandidateBasesByMeanRelativeDefect(uint32_t maxToTake)
{
    uint32_t toTakeCount = min(maxToTake, (uint32_t) candidateLattices.size());

    sortIndices.resize(candidateLattices.size());
    iota(sortIndices.begin(), sortIndices.end(), 0);
    nth_element(sortIndices.begin(), sortIndices.begin() + toTakeCount - 1, sortIndices.end(),
            [&](uint32_t i, uint32_t j)
            {   return candidateLattices[i].assembledLatticeStatistics.meanRelativeDefect < candidateLattices[j].assembledLatticeStatistics.meanRelativeDefect;});

    sortIndices.resize(toTakeCount);
    sort(sortIndices.begin(), sortIndices.end(),
            [&](uint32_t i, uint32_t j)
            {   return candidateLattices[i].assembledLatticeStatistics.meanRelativeDefect < candidateLattices[j].assembledLatticeStatistics.meanRelativeDefect;});

    vector< candidateLattice_t > candidateLatticesFiltered;
    candidateLatticesFiltered.resize(toTakeCount);
    for (uint32_t i = 0; i < toTakeCount; ++i) {
        candidateLatticesFiltered[i] = candidateLattices[sortIndices[i]];
    }

    candidateLattices.swap(candidateLatticesFiltered);
}
