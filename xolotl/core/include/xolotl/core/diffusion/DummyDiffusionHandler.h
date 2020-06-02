#ifndef DUMMYDIFFUSIONHANDLER_H
#define DUMMYDIFFUSIONHANDLER_H

// Includes
#include <xolotl/core/diffusion/DiffusionHandler.h>

namespace xolotl {
namespace core {
namespace diffusion {

/**
 * This class realizes the IDiffusionHandler interface responsible for all
 * the physical parts for the diffusion of mobile cluster. Here it is a dummy class,
 * meaning that it should not do anything.
 */
class DummyDiffusionHandler: public DiffusionHandler {
public:

	//! The Constructor
	DummyDiffusionHandler(double threshold) :
			DiffusionHandler(threshold) {
	}

	//! The Destructor
	~DummyDiffusionHandler() {
	}

	/**
	 * Initialize the off-diagonal part of the Jacobian. If this step is skipped it
	 * won't be possible to set the partials for the diffusion.
	 *
	 * We don't want any cluster to diffuse, so nothing is set to 1 in ofill, and no index
	 * is added to the vector.
	 *
	 * @param network The network
	 * @param ofillMap Map of connectivity for diffusing clusters.
	 */
	void initializeOFill(const network::IReactionNetwork &network,
			network::IReactionNetwork::SparseFillMap &ofillMap) override {
		// Clear the index vector
		diffusingClusters.clear();

		// And don't do anything else
		return;
	}

	/**
	 * Initialize an array of the dimension of the physical domain times the number of diffusion
	 * clusters. For each location, True means the cluster is diffusion, False means it is not.
	 *
	 * @param advectionHandlers The vector of advection handlers
	 * @param grid The spatial grid in the depth direction
	 * @param nx The number of grid points in the X direction
	 * @param xs The beginning of the grid on this process
	 * @param ny The number of grid points in the Y direction
	 * @param hy The step size in the Y direction
	 * @param ys The beginning of the grid on this process
	 * @param nz The number of grid points in the Z direction
	 * @param hz The step size in the Z direction
	 * @param zs The beginning of the grid on this process
	 */
	void initializeDiffusionGrid(
			std::vector<advection::IAdvectionHandler*> advectionHandlers,
			std::vector<double> grid, int nx, int xs, int ny = 0, double hy =
					0.0, int ys = 0, int nz = 0, double hz = 0.0, int zs = 0)
					override {
		// Don't do anything
		return;
	}

	/**
	 * Compute the flux due to the diffusion for all the cluster that are diffusing,
	 * given the space parameters.
	 * This method is called by the RHSFunction from the PetscSolver.
	 *
	 * Here it won't do anything because it is a dummy class.
	 *
	 * @param network The network
	 * @param concVector The pointer to the pointer of arrays of concentration at middle,
	 * left, and right grid points
	 * @param updatedConcOffset The pointer to the array of the concentration at the grid
	 * point where the diffusion is computed used to find the next solution
	 * @param hxLeft The step size on the left side of the point in the x direction (a)
	 * @param hxRight The step size on the right side of the point in the x direction (b)
	 * @param ix The position on the x grid
	 * @param sy The space parameter, depending on the grid step size in the y direction
	 * @param iy The position on the y grid
	 * @param sz The space parameter, depending on the grid step size in the z direction
	 * @param iz The position on the z grid
	 */
	void computeDiffusion(network::IReactionNetwork &network,
			double **concVector, double *updatedConcOffset, double hxLeft,
			double hxRight, int ix, double sy = 0.0, int iy = 0,
			double sz = 0.0, int iz = 0) const override {
		return;
	}

	/**
	 * Compute the partials due to the diffusion of all the diffusing clusters given
	 * the space parameters.
	 * This method is called by the RHSJacobian from the PetscSolver.
	 *
	 * Here it won't do anything because it is a dummy class.
	 *
	 * @param network The network
	 * @param val The pointer to the array that will contain the values of partials
	 * for the diffusion
	 * @param indices The pointer to the array that will contain the indices of the
	 * diffusing clusters in the network
	 * @param hxLeft The step size on the left side of the point in the x direction (a)
	 * @param hxRight The step size on the right side of the point in the x direction (b)
	 * @param ix The position on the x grid
	 * @param sy The space parameter, depending on the grid step size in the y direction
	 * @param iy The position on the y grid
	 * @param sz The space parameter, depending on the grid step size in the z direction
	 * @param iz The position on the z grid
	 */
	void computePartialsForDiffusion(network::IReactionNetwork &network,
			double *val, int *indices, double hxLeft, double hxRight, int ix,
			double sy = 0.0, int iy = 0, double sz = 0.0, int iz = 0) const
					override {
		return;
	}

};
//end class DummyDiffusionHandler

} /* end namespace diffusion */
} /* end namespace core */
} /* end namespace xolotl */
#endif
