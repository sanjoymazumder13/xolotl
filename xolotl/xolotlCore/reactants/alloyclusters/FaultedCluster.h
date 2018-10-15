#ifndef FAULTEDCLUSTER_H
#define FAULTEDCLUSTER_H

// Includes
#include "AlloyCluster.h"
#include <xolotlPerf.h>

namespace xolotlCore {

/**
 * This class represents a cluster composed entirely of interstitial.
 */
class FaultedCluster: public AlloyCluster {

public:

	/**
	 * Default constructor, deleted because we require info to construct.
	 */
	FaultedCluster() = delete;

	/**
	 * The constructor. All FaultedClusters must be initialized with a size.
	 *
	 * @param n The size of the cluster
	 * @param registry The performance handler registry
	 */
	FaultedCluster(int n, IReactionNetwork& _network,
			std::shared_ptr<xolotlPerf::IHandlerRegistry> registry) :
			AlloyCluster(_network, registry) {
		// Set the size
		size = n;
		// Update the composition map
		composition[toCompIdx(Species::Faulted)] = size;

		// Set the reactant name appropriately
		std::stringstream nameStream;
		nameStream << "Fa_" << size;
		name = nameStream.str();
		// Set the typename appropriately
		type = ReactantType::Faulted;

		// Define the diffusion pre-factor
		{
			//double jumpDistance = xolotlCore::alloyLatticeConstant / sqrt(2.0);
			//double phononFrequency = 1.0e13;
			//double jumpsPerPhonon = 1.0;
			//double prefactorExponent = -1.0;
			//diffusionFactor = phononFrequency * jumpsPerPhonon * jumpDistance
			//    * jumpDistance * pow(double(size),prefactorExponent) / (6.0);
			diffusionFactor = 0.0;
		}

		// Define the formation energy
		formationEnergy = _network.getFormationEnergy(type, size);

		// Define the migration energy
		migrationEnergy = 1.2;

		// Define the reaction radius
		reactionRadius = _network.getReactionRadius(type, size);
	}

	/**
	 * Destructor
	 */
	~FaultedCluster() {
	}

};
//end class FaultedCluster

} /* namespace xolotlCore */
#endif
