/**
 * Main.c, currently only able to load clusters
 */
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cassert>
#include <ctime>
#include <mpi.h>
#include <xolotl/solver/PetscSolver.h>
#include <xolotl/io/MPIUtils.h>
#include <xolotl/options/Options.h>
#include <xolotl/perf/xolotlPerf.h>
#include <xolotl/factory/material/IMaterialFactory.h>
#include <xolotl/factory/temperature/TemperatureHandlerFactory.h>
#include <xolotl/factory/viz/VizHandlerRegistryFactory.h>
#include <xolotl/factory/solver/SolverHandlerFactory.h>
#include <xolotl/solver/handler/ISolverHandler.h>
#include <xolotl/factory/network/IReactionHandlerFactory.h>

using namespace std;
using namespace xolotl;

//! This operation prints the start message
void printStartMessage() {
	std::cout << "Starting Xolotl Plasma-Surface Interactions Simulator"
			<< std::endl;
	// TODO! Print copyright message
	// Print date and time
	std::time_t currentTime = std::time(NULL);
	std::cout << std::asctime(std::localtime(&currentTime)); // << std::endl;
}

std::shared_ptr<xolotl::factory::material::IMaterialFactory> initMaterial(
		const xolotl::options::Options &options) {
	// Create the material factory
	auto materialFactory =
			factory::material::IMaterialFactory::createMaterialFactory(options);

	// Initialize it with the options
	materialFactory->initializeMaterial(options);

	return materialFactory;
}

bool initTemp(const xolotl::options::Options &options) {

	bool tempInitOK = factory::temperature::initializeTempHandler(options);
	if (!tempInitOK) {
		std::cerr << "Unable to initialize requested temperature.  Aborting"
				<< std::endl;
		return EXIT_FAILURE;
	} else
		return tempInitOK;
}

bool initViz(bool opts) {

	bool vizInitOK = factory::viz::initializeVizHandler(opts);
	if (!vizInitOK) {
		std::cerr
				<< "Unable to initialize requested visualization infrastructure. "
				<< "Aborting" << std::endl;
		return EXIT_FAILURE;
	} else
		return vizInitOK;
}

std::unique_ptr<xolotl::solver::PetscSolver> setUpSolver(
		std::shared_ptr<xolotl::perf::IHandlerRegistry> handlerRegistry,
		std::shared_ptr<factory::material::IMaterialFactory> material,
		std::shared_ptr<core::temperature::ITemperatureHandler> tempHandler,
		xolotl::solver::handler::ISolverHandler &solvHandler,
        const xolotl::options::Options &options) {
	// Initialize the solver handler
	solvHandler.initializeHandlers(material, tempHandler, options);

	// Setup the solver
	auto solverInitTimer = handlerRegistry->getTimer("initSolver");
	solverInitTimer->start();
	// Once we have widespread C++14 support, use std::make_unique.
	std::unique_ptr<xolotl::solver::PetscSolver> solver(
			new xolotl::solver::PetscSolver(solvHandler, handlerRegistry));
	solver->setCommandLineOptions(options.getPetscArg());
	solver->initialize();
	solverInitTimer->stop();

	return solver;
}

void launchPetscSolver(xolotl::solver::PetscSolver &solver,
		std::shared_ptr<xolotl::perf::IHandlerRegistry> handlerRegistry) {

	perf::IHardwareCounter::SpecType hwctrSpec;
	hwctrSpec.push_back(perf::IHardwareCounter::FPOps);
	hwctrSpec.push_back(perf::IHardwareCounter::Cycles);
	hwctrSpec.push_back(perf::IHardwareCounter::L3CacheMisses);

	// Launch the PetscSolver
	auto solverTimer = handlerRegistry->getTimer("solve");
	auto solverHwctr = handlerRegistry->getHardwareCounter("solve", hwctrSpec);
	solverTimer->start();
	solverHwctr->start();
	solver.solve();
	solverHwctr->stop();
	solverTimer->stop();
}

//! Run the Xolotl simulation.
int runXolotl(const xolotl::options::Options &opts) {

	// Set up our performance data infrastructure.
	perf::initialize(opts.getPerfHandlerType());

	// Get the MPI rank
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		// Print the start message
		printStartMessage();
	}
	// Initialize kokkos
	Kokkos::initialize();
	{

		// Set up the material infrastructure that is used to calculate flux
		auto material = initMaterial(opts);
		// Set up the temperature infrastructure
		bool tempInitOK = initTemp(opts);
		if (!tempInitOK) {
			throw std::runtime_error("Unable to initialize temperature.");
		}
		// Set up the visualization infrastructure.
		bool vizInitOK = initViz(opts.useVizStandardHandlers());
		if (!vizInitOK) {
			throw std::runtime_error(
					"Unable to initialize visualization infrastructure.");
		}

		// Access the temperature handler registry to get the temperature
		auto tempHandler = factory::temperature::getTemperatureHandler();

		// Access our performance handler registry to obtain a Timer
		// measuring the runtime of the entire program.
		auto handlerRegistry = xolotl::perf::getHandlerRegistry();
		auto totalTimer = handlerRegistry->getTimer("total");
		totalTimer->start();

		// Create the network handler factory
		auto networkFactory =
				factory::network::IReactionHandlerFactory::createNetworkFactory(
						opts.getMaterial());

		// Build a reaction network
		auto networkLoadTimer = handlerRegistry->getTimer("loadNetwork");
		networkLoadTimer->start();
		networkFactory->initializeReactionNetwork(opts, handlerRegistry);
		networkLoadTimer->stop();
		if (rank == 0) {
			std::cout << "Network size on device: "
					<< networkFactory->getNetworkHandler().getDeviceMemorySize()
					<< '\n';

			std::time_t currentTime = std::time(NULL);
			std::cout << std::asctime(std::localtime(&currentTime));
		}
		auto &rNetwork = networkFactory->getNetworkHandler();

		// Initialize and get the solver handler
		bool dimOK = factory::solver::initializeDimension(opts, rNetwork);
		if (!dimOK) {
			throw std::runtime_error(
					"Unable to initialize dimension from inputs.");
		}
		auto &solvHandler = factory::solver::getSolverHandler();

		// Setup the solver
		auto solver = setUpSolver(handlerRegistry, material, tempHandler,
				solvHandler, opts);

		// Launch the PetscSolver
		launchPetscSolver(*solver, handlerRegistry);

		// Finalize our use of the solver.
		auto solverFinalizeTimer = handlerRegistry->getTimer("solverFinalize");
		solverFinalizeTimer->start();
		solver->finalize();
		solverFinalizeTimer->stop();

		totalTimer->stop();

		// Report statistics about the performance data collected during
		// the run we just completed.
		perf::PerfObjStatsMap<perf::ITimer::ValType> timerStats;
		perf::PerfObjStatsMap<perf::IEventCounter::ValType> counterStats;
		perf::PerfObjStatsMap<perf::IHardwareCounter::CounterType> hwCtrStats;
		handlerRegistry->collectStatistics(timerStats, counterStats,
				hwCtrStats);
		if (rank == 0) {
			handlerRegistry->reportStatistics(std::cout, timerStats,
					counterStats, hwCtrStats);
		}

        factory::solver::destroySolverHandler();
        factory::network::IReactionHandlerFactory::resetNetworkFactory();
	}

	// Finalize kokkos
	Kokkos::finalize();

	return 0;
}

//! Main program
int main(int argc, char **argv) {

	// Local Declarations
	int ret = EXIT_SUCCESS;

	// Initialize MPI. We do this instead of leaving it to some
	// other package (e.g., PETSc), because we want to avoid problems
	// with overlapping Timer scopes.
	// We do this before our own parsing of the command line,
	// because it may change the command line.
	MPI_Init(&argc, &argv);

	try {
		// Check the command line arguments.
        xolotl::options::Options opts;
		opts.readParams(argc, argv);
		if (opts.shouldRun()) {
			// Skip the name of the parameter file that was just used.
			// The arguments should be empty now.
			// TODO is this needed?
			argc -= 2;
			argv += 2;

			// Run the simulation.
			ret = runXolotl(opts);
		} else {
			ret = opts.getExitCode();
		}
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Aborting." << std::endl;
		ret = EXIT_FAILURE;
	} catch (const std::string &error) {
		std::cerr << error << std::endl;
		std::cerr << "Aborting." << std::endl;
		ret = EXIT_FAILURE;
	} catch (...) {
		std::cerr << "Unrecognized exception seen." << std::endl;
		std::cerr << "Aborting." << std::endl;
		ret = EXIT_FAILURE;
	}

	// Clean up.
	MPI_Finalize();

	return ret;
}
