#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Regression

#include <boost/test/unit_test.hpp>
#include <mpi.h>
#include <fstream>
#include <iostream>
#include <xolotl/core/modified/DummyNucleationHandler.h>
#include <xolotl/core/network/NEReactionNetwork.h>
#include <xolotl/config.h>
#include <xolotl/options/Options.h>

using namespace std;
using namespace xolotl::core;
using namespace modified;

using Kokkos::ScopeGuard;
BOOST_GLOBAL_FIXTURE(ScopeGuard);

/**
 * This suite is responsible for testing the DummyNucleationHandler.
 */
BOOST_AUTO_TEST_SUITE(DummyNucleationHandler_testSuite)

/**
 * Method checking the initialization and the compute dummy nucleation methods.
 */
BOOST_AUTO_TEST_CASE(checkDummyNucleation) {
	// Create the option to create a network
    xolotl::options::Options opts;
	// Create a good parameter file
	std::ofstream paramFile("param.txt");
	paramFile << "netParam=1000 0 0 0 0" << std::endl;
	paramFile.close();

	// Create a fake command line to read the options
	int argc = 2;
	char **argv = new char*[3];
	std::string appName = "fakeXolotlAppNameForTests";
	argv[0] = new char[appName.length() + 1];
	strcpy(argv[0], appName.c_str());
	std::string parameterFile = "param.txt";
	argv[1] = new char[parameterFile.length() + 1];
	strcpy(argv[1], parameterFile.c_str());
	argv[2] = 0; // null-terminate the array
	// Initialize MPI
	MPI_Init(&argc, &argv);
	opts.readParams(argc, argv);

	// Create a grid
	std::vector<double> grid;
	std::vector<double> temperatures;
	int nGrid = 3;
	for (int l = 0; l < nGrid; l++) {
		grid.push_back((double) l * 0.1);
		temperatures.push_back(1800.0);
	}
	// Specify the surface position
	int surfacePos = 0;

	// Create the network
	using NetworkType = network::NEReactionNetwork;
	NetworkType::AmountType maxXe = opts.getMaxImpurity();
	NetworkType network( { maxXe }, grid.size(), opts);
	network.syncClusterDataOnHost();
	network.getSubpaving().syncZones(plsm::onHost);
	// Get its size
	const int dof = network.getDOF();

	// Create the nucleation handler
	DummyNucleationHandler nucleationHandler;

	// Initialize it
	network::IReactionNetwork::SparseFillMap dfill;
	nucleationHandler.initialize(network, dfill);
	nucleationHandler.setFissionYield(0.5);
	nucleationHandler.updateHeterogeneousNucleationRate(1.0);

	// Check that nothing was added to dfill
	BOOST_REQUIRE_EQUAL(dfill.size(), 0);

	// The arrays of concentration
	double concentration[nGrid * dof];
	double newConcentration[nGrid * dof];

	// Initialize their values
	for (int i = 0; i < nGrid * dof; i++) {
		concentration[i] = (double) i * i;
		newConcentration[i] = 0.0;
	}

	// Get pointers
	double *conc = &concentration[0];
	double *updatedConc = &newConcentration[0];

	// Get the offset for the fifth grid point
	double *concOffset = conc + 1 * dof;
	double *updatedConcOffset = updatedConc + 1 * dof;

	// Set the temperature to compute the rates
	network.setTemperatures(temperatures);
	network.syncClusterDataOnHost();

	// Compute the modified trap mutation at the sixth grid point
	nucleationHandler.computeHeterogeneousNucleation(network, concOffset,
			updatedConcOffset, 1, 0);

	// Check the new values of updatedConcOffset
	BOOST_REQUIRE_CLOSE(updatedConcOffset[0], 0.0, 0.01); // Create Xe
	BOOST_REQUIRE_CLOSE(updatedConcOffset[1], 0.0, 0.01); // Create Xe_2

	// Remove the created file
	std::string tempFile = "param.txt";
	std::remove(tempFile.c_str());

	// Finalize MPI
	MPI_Finalize();

	return;
}

BOOST_AUTO_TEST_SUITE_END()
