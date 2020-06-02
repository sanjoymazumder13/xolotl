#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Regression

#include <boost/test/unit_test.hpp>
#include <mpi.h>
#include <fstream>
#include <iostream>
#include <xolotl/core/diffusion/Diffusion2DHandler.h>
#include <xolotl/config.h>
#include <xolotl/options/Options.h>
#include <xolotl/core/network/PSIReactionNetwork.h>

using namespace std;
using namespace xolotl::core;
using namespace diffusion;

class KokkosContext {
public:
	KokkosContext() {
		::Kokkos::initialize();
	}

	~KokkosContext() {
		::Kokkos::finalize();
	}
};
BOOST_GLOBAL_FIXTURE(KokkosContext);

/**
 * This suite is responsible for testing the Diffusion2DHandler.
 */
BOOST_AUTO_TEST_SUITE(Diffusion2DHandler_testSuite)

/**
 * Method checking the initialization of the off-diagonal part of the Jacobian,
 * and the compute diffusion methods.
 */
BOOST_AUTO_TEST_CASE(checkDiffusion) {

    xolotl::options::Options opts;
	// Create a good parameter file
	std::ofstream paramFile("param.txt");
	paramFile << "netParam=8 0 0 1 0" << std::endl;
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
	// Initialize MPI for HDF5
	MPI_Init(&argc, &argv);
	opts.readParams(argc, argv);

	// Create a grid
	std::vector<double> grid;
	std::vector<double> temperatures;
	for (int l = 0; l < 5; l++) {
		grid.push_back((double) l);
		temperatures.push_back(1000.0);
	}

	// Create the network
	using NetworkType =
	network::PSIReactionNetwork<network::PSIFullSpeciesList>;
	NetworkType::AmountType maxV = opts.getMaxV();
	NetworkType::AmountType maxI = opts.getMaxI();
	NetworkType::AmountType maxHe = opts.getMaxImpurity();
	NetworkType::AmountType maxD = opts.getMaxD();
	NetworkType::AmountType maxT = opts.getMaxT();
	NetworkType network( { maxHe, maxD, maxT, maxV, maxI }, grid.size(), opts);
	network.syncClusterDataOnHost();
	network.getSubpaving().syncZones(plsm::onHost);
	// Get its size
	const int dof = network.getDOF();

	// Create the diffusion handler
	Diffusion2DHandler diffusionHandler(opts.getMigrationThreshold());

	// Create a collection of advection handlers
	std::vector<advection::IAdvectionHandler*> advectionHandlers;

	// Create ofill
	network::IReactionNetwork::SparseFillMap ofill;

	// Initialize it
	diffusionHandler.initializeOFill(network, ofill);
	diffusionHandler.initializeDiffusionGrid(advectionHandlers, grid, 5, 0, 3,
			1.0, 0);

	// Check the total number of diffusing clusters
	BOOST_REQUIRE_EQUAL(diffusionHandler.getNumberOfDiffusing(), 8);

	// The step size in the x direction
	double hx = 1.0;
	// The size parameter in the y direction
	double sy = 1.0;

	// The arrays of concentration
	double concentration[9 * dof];
	double newConcentration[9 * dof];

	// Initialize their values
	for (int i = 0; i < 9 * dof; i++) {
		concentration[i] = (double) i * i;
		newConcentration[i] = 0.0;
	}

	// Set the temperature to 1000K to initialize the diffusion coefficients
	network.setTemperatures(temperatures);
	network.syncClusterDataOnHost();

	// Get pointers
	double *conc = &concentration[0];
	double *updatedConc = &newConcentration[0];

	// Get the offset for the grid point in the middle
	// Supposing the 9 grid points are laid-out as follow:
	// 6 | 7 | 8
	// 3 | 4 | 5
	// 0 | 1 | 2
	double *concOffset = conc + 4 * dof;
	double *updatedConcOffset = updatedConc + 4 * dof;

	// Fill the concVector with the pointer to the middle, left, right, bottom, and top grid points
	double **concVector = new double*[5];
	concVector[0] = concOffset; // middle
	concVector[1] = conc + 3 * dof; // left
	concVector[2] = conc + 5 * dof; // right
	concVector[3] = conc + 1 * dof; // bottom
	concVector[4] = conc + 7 * dof; // top

	// Compute the diffusion at this grid point
	diffusionHandler.computeDiffusion(network, concVector, updatedConcOffset,
			hx, hx, 0, sy, 1);

	// Check the new values of updatedConcOffset
	BOOST_REQUIRE_CLOSE(updatedConcOffset[1], 3.7081e+13, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[3], 1.8160e+13, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[5], 7.3065e+12, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[7], 9.6476e+12, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[9], 7.1800e+12, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[11], 1.7783e+11, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[13], 2.7860e+10, 0.01);
	BOOST_REQUIRE_CLOSE(updatedConcOffset[15], 0.0, 0.01); // He_8 does not diffuse
	BOOST_REQUIRE_CLOSE(updatedConcOffset[0], 2.9207e+09, 0.01);

	// Initialize the indices and values to set in the Jacobian
	int nDiff = diffusionHandler.getNumberOfDiffusing();
	int indices[nDiff];
	double val[5 * nDiff];
	// Get the pointer on them for the compute diffusion method
	int *indicesPointer = &indices[0];
	double *valPointer = &val[0];

	// Compute the partial derivatives for the diffusion a the grid point 1
	diffusionHandler.computePartialsForDiffusion(network, valPointer,
			indicesPointer, hx, hx, 0, sy, 1);

	// Check the values for the indices
	BOOST_REQUIRE_EQUAL(indices[0], 0);
	BOOST_REQUIRE_EQUAL(indices[1], 1);
	BOOST_REQUIRE_EQUAL(indices[2], 3);
	BOOST_REQUIRE_EQUAL(indices[3], 5);
	BOOST_REQUIRE_EQUAL(indices[4], 7);
	BOOST_REQUIRE_EQUAL(indices[5], 9);
	BOOST_REQUIRE_EQUAL(indices[6], 11);
	BOOST_REQUIRE_EQUAL(indices[7], 13);

	// Check some values
	BOOST_REQUIRE_CLOSE(val[0], -2021250, 0.01);
	BOOST_REQUIRE_CLOSE(val[3], 505312, 0.01);
	BOOST_REQUIRE_CLOSE(val[8], 6415444736, 0.01);
	BOOST_REQUIRE_CLOSE(val[12], 3141913616, 0.01);
	BOOST_REQUIRE_CLOSE(val[15], -5056420168, 0.01);

	// Remove the created file
	std::string tempFile = "param.txt";
	std::remove(tempFile.c_str());

	// Finalize MPI
	MPI_Finalize();
}

BOOST_AUTO_TEST_SUITE_END()
