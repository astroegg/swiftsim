/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2016 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#ifndef SWIFT_GRAVITY_PROPERTIES
#define SWIFT_GRAVITY_PROPERTIES

/* Config parameters. */
#include "../config.h"

#if defined(HAVE_HDF5)
#include <hdf5.h>
#endif

/* Local includes. */
#include "restart.h"

/* Forward declarations */
struct cosmology;
struct swift_params;

/**
 * @brief Contains all the constants and parameters of the self-gravity scheme
 */
struct gravity_props {

  /* -------------- Softening for the regular DM and baryons ----------- */

  /*! Softening length for high-res. particles at the current redshift.  */
  float epsilon_cur;

  /* -------------- Softening for the background DM -------------------- */

  /*! Conversion factor from cbrt of particle mass to softening assuming
   * a constant fraction of the mean inter-particle separation at that mass. */
  float epsilon_background_fac;

  /* -------------- Properties of the FFM gravity ---------------------- */

  /*! Tree opening angle (Multipole acceptance criterion) */
  double theta_crit;

  /*! Square of opening angle */
  double theta_crit2;

  /*! Inverse of opening angle */
  double theta_crit_inv;

  /* For M2P interactions, if N_gparts(cell j) is less than this number,
   * force P-P interactions with that cell */
  int min_j_M2P;

  /* For M2L interactions, if N_gparts(cell_i) * N_gparts(cell j) is less 
   * than this number, force P-P interactions between the cells */
  int min_ij_M2L;

#ifdef ADVANCED_OPENING_CRITERIA
  /* The relative force error we are aiming for */
  double rel_force_error;
#endif

  /* ------------- Properties of the softened gravity ------------------ */

  /*! Fraction of the mean inter particle separation corresponding to the
   * co-moving softening length of the low-res. particles (DM + baryons) */
  float mean_inter_particle_fraction_high_res;

  /*! Co-moving softening length for for high-res. particles (DM + baryons)
   * assuming a constant fraction of the mean inter-particle separation
   * and a constant particle mass */
  float epsilon_comoving;

  /*! Maximal softening length in physical coordinates for the high-res.
   * particles (DM + baryons) */
  float epsilon_max_physical;

  /*! In case of zoom runs: mass of the DM particles in the zoom region in
   * internal units. Otherwise: mass of the DM particles in internal units. -1
   * If there are no dark matter particles. */
  float high_res_DM_mass;

  /* ------------- Properties of the time integration  ----------------- */

  /*! Frequency of tree-rebuild in units of #gpart updates. */
  float rebuild_frequency;

  /*! Time integration dimensionless multiplier */
  float eta;

  /* ------------- Properties of the mesh-based gravity ---------------- */

  /*! Periodic long-range mesh side-length */
  int mesh_size;

  /*! Mesh smoothing scale in units of top-level cell size */
  float a_smooth;

  /*! Distance below which the truncated mesh force is Newtonian in units of
   * a_smooth */
  float r_cut_min_ratio;

  /*! Distance above which the truncated mesh force is negligible in units of
   * a_smooth */
  float r_cut_max_ratio;
};

void gravity_props_print(const struct gravity_props *p);
void gravity_props_init(struct gravity_props *p, struct swift_params *params,
                        const struct cosmology *cosmo,
                        const double high_res_DM_mass, const int with_cosmology,
                        const int periodic);
void gravity_props_update(struct gravity_props *p,
                          const struct cosmology *cosmo);

#if defined(HAVE_HDF5)
void gravity_props_print_snapshot(hid_t h_grpsph,
                                  const struct gravity_props *p);
#endif

/* Dump/restore. */
void gravity_props_struct_dump(const struct gravity_props *p, FILE *stream);
void gravity_props_struct_restore(struct gravity_props *p, FILE *stream);

#endif /* SWIFT_GRAVITY_PROPERTIES */
