/*******************************************************************************
 * This file is part of SWIFT.
 * Coypright (c) 2015 Matthieu Schaller (matthieu.schaller@durham.ac.uk)
 *               2016 Tom Theuns (tom.theuns@durham.ac.uk)
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

#include <float.h>
#include "potentials.h"
/**
 * @brief Computes the gravity time-step of a given particle.
 *
 * @param phys_cont The physical constants in internal units.
 * @param gp Pointer to the g-particle data.
 */
__attribute__((always_inline)) INLINE static float gravity_compute_timestep(
    const struct phys_const* const phys_const, const struct gpart* const gp) {

  float dt = FLT_MAX;

#ifdef EXTERNAL_POTENTIAL_POINTMASS
  dt = fminf(dt, external_gravity_pointmass_timestep(phys_const, gp));
#endif

  return dt;
}

/**
 * @brief Initialises the g-particles for the first time
 *
 * This function is called only once just after the ICs have been
 * read in to do some conversions.
 *
 * @param gp The particle to act upon
 */
__attribute__((always_inline))
    INLINE static void gravity_first_init_gpart(struct gpart* gp) {}

/**
 * @brief Prepares a g-particle for the gravity calculation
 *
 * Zeroes all the relevant arrays in preparation for the sums taking place in
 * the variaous tasks
 *
 * @param gp The particle to act upon
 */
__attribute__((always_inline))
    INLINE static void gravity_init_part(struct gpart* gp) {

  /* Zero the acceleration */
  gp->a_grav[0] = 0.f;
  gp->a_grav[1] = 0.f;
  gp->a_grav[2] = 0.f;
}

/**
 * @brief Finishes the gravity calculation.
 *
 * Multiplies the forces and accelerations by the appropiate constants
 *
 * @param gp The particle to act upon
 */
__attribute__((always_inline))
    INLINE static void gravity_end_force(struct gpart* gp) {}

/**
 * @brief Computes the gravitational acceleration induced by external potentials
 *
 * @param phys_const The physical constants in internal units.
 * @param gp The particle to act upon.
 */
__attribute__((always_inline)) INLINE static void external_gravity(const struct external_potential* potential, const struct phys_const* const phys_const, struct gpart* gp) {

#ifdef EXTERNAL_POTENTIAL_POINTMASS
  external_gravity_pointmass(potential, phys_const, gp);
#endif
}

/**
 * @brief Kick the additional variables
 *
 * @param gp The particle to act upon
 * @param dt The time-step for this kick
 * @param half_dt The half time-step for this kick
 */
__attribute__((always_inline)) INLINE static void gravity_kick_extra(
    struct gpart* gp, float dt, float half_dt) {}
