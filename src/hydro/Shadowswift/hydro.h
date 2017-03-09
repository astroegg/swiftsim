/*******************************************************************************
 * This file is part of SWIFT.
 * Copyright (c) 2016 Bert Vandenbroucke (bert.vandenbroucke@gmail.com).
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
#include "adiabatic_index.h"
#include "approx_math.h"
#include "equation_of_state.h"
#include "hydro_gradients.h"
#include "voronoi_algorithm.h"

/**
 * @brief Computes the hydro time-step of a given particle
 *
 * @param p Pointer to the particle data.
 * @param xp Pointer to the extended particle data.
 * @param hydro_properties Pointer to the hydro parameters.
 */
__attribute__((always_inline)) INLINE static float hydro_compute_timestep(
    const struct part* restrict p, const struct xpart* restrict xp,
    const struct hydro_props* restrict hydro_properties) {

  const float CFL_condition = hydro_properties->CFL_condition;

  float R = get_radius_dimension_sphere(p->cell.volume);

  return CFL_condition * R / fabsf(p->timestepvars.vmax);
}

/**
 * @brief Does some extra hydro operations once the actual physical time step
 * for the particle is known.
 *
 * We use this to store the physical time step, since it is used for the flux
 * exchange during the force loop.
 *
 * We also set the active flag of the particle to inactive. It will be set to
 * active in hydro_init_part, which is called the next time the particle becomes
 * active.
 *
 * @param p The particle to act upon.
 * @param dt Physical time step of the particle during the next step.
 */
__attribute__((always_inline)) INLINE static void hydro_timestep_extra(
    struct part* p, float dt) {}

/**
 * @brief Initialises the particles for the first time
 *
 * This function is called only once just after the ICs have been
 * read in to do some conversions.
 *
 * In this case, we copy the particle velocities into the corresponding
 * primitive variable field. We do this because the particle velocities in GIZMO
 * can be independent of the actual fluid velocity. The latter is stored as a
 * primitive variable and integrated using the linear momentum, a conserved
 * variable.
 *
 * @param p The particle to act upon
 * @param xp The extended particle data to act upon
 */
__attribute__((always_inline)) INLINE static void hydro_first_init_part(
    struct part* p, struct xpart* xp) {

  xp->v_full[0] = p->v[0];
  xp->v_full[1] = p->v[1];
  xp->v_full[2] = p->v[2];

  p->primitives.v[0] = p->v[0];
  p->primitives.v[1] = p->v[1];
  p->primitives.v[2] = p->v[2];
}

/**
 * @brief Prepares a particle for the volume calculation.
 *
 * Simply makes sure all necessary variables are initialized to zero.
 * Initializes the Voronoi cell.
 *
 * @param p The particle to act upon
 */
__attribute__((always_inline)) INLINE static void hydro_init_part(
    struct part* p) {

  p->density.wcount = 0.0f;
  p->density.wcount_dh = 0.0f;
  p->rho = 0.0f;

  voronoi_cell_init(&p->cell, p->x);
}

/**
 * @brief Finishes the volume calculation.
 *
 * Calls the finalize method on the Voronoi cell, which calculates the volume
 * and centroid of the cell. We use the return value of this function to set
 * a new value for the smoothing length and possibly force another iteration
 * of the volume calculation for this particle. We then use the volume to
 * convert conserved variables into primitive variables.
 *
 * This method also initializes the gradient variables (if gradients are used).
 *
 * @param p The particle to act upon.
 */
__attribute__((always_inline)) INLINE static void hydro_end_density(
    struct part* restrict p) {

  float volume;
  float m, momentum[3], energy;

  hydro_gradients_init(p);

  float hnew = voronoi_cell_finalize(&p->cell);
  /* Enforce hnew as new smoothing length in the iteration
     This is annoyingly difficult, as we do not have access to the variables
     that govern the loop...
     So here's an idea: let's force in some method somewhere that makes sure
     r->e->hydro_properties->target_neighbours is 1, and
     r->e->hydro_properties->delta_neighbours is 0.
     This way, we can accept an iteration by setting p->density.wcount to 1.
     To get the right correction for h, we set wcount to something else
     (say 0), and then set p->density.wcount_dh to 1/(hnew-h). */
  if (hnew < p->h) {
    /* Iteration succesful: we accept, but manually set h to a smaller value
       for the next time step */
    p->density.wcount = 1.0f;
    p->h = 1.1f * hnew;
  } else {
    /* Iteration not succesful: we force h to become 1.1*hnew */
    p->density.wcount = 0.0f;
    p->density.wcount_dh = 1.0f / (1.1f * hnew - p->h);
  }
  volume = p->cell.volume;

  /* compute primitive variables */
  /* eqns (3)-(5) */
  m = p->conserved.mass;
  if (m) {
    momentum[0] = p->conserved.momentum[0];
    momentum[1] = p->conserved.momentum[1];
    momentum[2] = p->conserved.momentum[2];
    p->primitives.rho = m / volume;
    p->primitives.v[0] = momentum[0] / m;
    p->primitives.v[1] = momentum[1] / m;
    p->primitives.v[2] = momentum[2] / m;
    energy = p->conserved.energy;
    p->primitives.P = hydro_gamma_minus_one * energy / volume;
  }
}

/**
 * @brief Prepare a particle for the gradient calculation.
 *
 * The name of this method is confusing, as this method is really called after
 * the density loop and before the gradient loop.
 *
 * We use it to set the physical timestep for the particle and to copy the
 * actual velocities, which we need to boost our interfaces during the flux
 * calculation. We also initialize the variables used for the time step
 * calculation.
 *
 * @param p The particle to act upon.
 * @param xp The extended particle data to act upon.
 * @param ti_current Current integer time.
 * @param timeBase Conversion factor between integer time and physical time.
 */
__attribute__((always_inline)) INLINE static void hydro_prepare_force(
    struct part* restrict p, struct xpart* restrict xp) {

  /* Set the physical time step */
  //  p->force.dt = (p->ti_end - p->ti_begin) * timeBase;

  /* Initialize time step criterion variables */
  p->timestepvars.vmax = 0.0f;

  /* Set the actual velocity of the particle */
  p->force.v_full[0] = xp->v_full[0];
  p->force.v_full[1] = xp->v_full[1];
  p->force.v_full[2] = xp->v_full[2];
}

/**
 * @brief Finishes the gradient calculation.
 *
 * Just a wrapper around hydro_gradients_finalize, which can be an empty method,
 * in which case no gradients are used.
 *
 * @param p The particle to act upon.
 */
__attribute__((always_inline)) INLINE static void hydro_end_gradient(
    struct part* p) {

  hydro_gradients_finalize(p);
}

/**
 * @brief Reset acceleration fields of a particle
 *
 * This is actually not necessary for Shadowswift, since we just set the
 * accelerations after the flux calculation.
 *
 * @param p The particle to act upon.
 */
__attribute__((always_inline)) INLINE static void hydro_reset_acceleration(
    struct part* p) {

  /* Reset the acceleration. */
  p->a_hydro[0] = 0.0f;
  p->a_hydro[1] = 0.0f;
  p->a_hydro[2] = 0.0f;

  /* Reset the time derivatives. */
  p->force.h_dt = 0.0f;
}

/**
 * @brief Sets the values to be predicted in the drifts to their values at a
 * kick time
 *
 * @param p The particle.
 * @param xp The extended data of this particle.
 */
__attribute__((always_inline)) INLINE static void hydro_reset_predicted_values(
    struct part* restrict p, const struct xpart* restrict xp) {}

/**
 * @brief Converts the hydrodynamic variables from the initial condition file to
 * conserved variables that can be used during the integration
 *
 * Requires the volume to be known.
 *
 * The initial condition file contains a mixture of primitive and conserved
 * variables. Mass is a conserved variable, and we just copy the particle
 * mass into the corresponding conserved quantity. We need the volume to
 * also derive a density, which is then used to convert the internal energy
 * to a pressure. However, we do not actually use these variables anymore.
 * We do need to initialize the linear momentum, based on the mass and the
 * velocity of the particle.
 *
 * @param p The particle to act upon.
 * @param xp The extended particle data to act upon.
 */
__attribute__((always_inline)) INLINE static void hydro_convert_quantities(
    struct part* p, struct xpart* xp) {

  float volume;
  float m;
  float momentum[3];
  volume = p->cell.volume;

  p->mass = m = p->conserved.mass;
  p->primitives.rho = m / volume;

  p->conserved.momentum[0] = momentum[0] = m * p->primitives.v[0];
  p->conserved.momentum[1] = momentum[1] = m * p->primitives.v[1];
  p->conserved.momentum[2] = momentum[2] = m * p->primitives.v[2];

  p->primitives.P =
      hydro_gamma_minus_one * p->conserved.energy * p->primitives.rho;

  p->conserved.energy *= m;
}

/**
 * @brief Extra operations to be done during the drift
 *
 * Not used for Shadowswift.
 *
 * @param p Particle to act upon.
 * @param xp The extended particle data to act upon.
 * @param dt The drift time-step.
 */
__attribute__((always_inline)) INLINE static void hydro_predict_extra(
    struct part* p, struct xpart* xp, float dt) {}

/**
 * @brief Set the particle acceleration after the flux loop
 *
 * We use the new conserved variables to calculate the new velocity of the
 * particle, and use that to derive the change of the velocity over the particle
 * time step. We also add a correction to the velocity which steers the
 * generator towards the centroid of the cell.
 *
 * If the particle time step is zero, we set the accelerations to zero. This
 * should only happen at the start of the simulation.
 *
 * @param p Particle to act upon.
 */
__attribute__((always_inline)) INLINE static void hydro_end_force(
    struct part* p) {

  float vcell[3];

  /* Update the conserved variables. We do this here and not in the kick,
     since we need the updated variables below. */
  p->conserved.mass += p->conserved.flux.mass;
  p->conserved.momentum[0] += p->conserved.flux.momentum[0];
  p->conserved.momentum[1] += p->conserved.flux.momentum[1];
  p->conserved.momentum[2] += p->conserved.flux.momentum[2];
  p->conserved.energy += p->conserved.flux.energy;

  /* reset fluxes */
  /* we can only do this here, since we need to keep the fluxes for inactive
     particles */
  p->conserved.flux.mass = 0.0f;
  p->conserved.flux.momentum[0] = 0.0f;
  p->conserved.flux.momentum[1] = 0.0f;
  p->conserved.flux.momentum[2] = 0.0f;
  p->conserved.flux.energy = 0.0f;

  /* Set the hydro acceleration, based on the new momentum and mass */
  /* NOTE: the momentum and mass are only correct for active particles, since
           only active particles have received flux contributions from all their
           neighbours. Since this method is only called for active particles,
           this is indeed the case. */
  if (p->force.dt) {
    /* We want the cell velocity to be as close as possible to the fluid
       velocity */
    vcell[0] = p->conserved.momentum[0] / p->conserved.mass;
    vcell[1] = p->conserved.momentum[1] / p->conserved.mass;
    vcell[2] = p->conserved.momentum[2] / p->conserved.mass;

    /* To prevent stupid things like cell crossovers or generators that move
       outside their cell, we steer the motion of the cell somewhat */
    if (p->primitives.rho) {
      float centroid[3], d[3];
      float volume, csnd, R, vfac, fac, dnrm;
      voronoi_get_centroid(&p->cell, centroid);
      d[0] = centroid[0] - p->x[0];
      d[1] = centroid[1] - p->x[1];
      d[2] = centroid[2] - p->x[2];
      dnrm = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
      csnd = sqrtf(hydro_gamma * p->primitives.P / p->primitives.rho);
      volume = p->cell.volume;
      R = get_radius_dimension_sphere(volume);
      fac = 4.0f * dnrm / R;
      if (fac > 0.9f) {
        if (fac < 1.1f) {
          vfac = csnd * (dnrm - 0.225f * R) / dnrm / (0.05f * R);
        } else {
          vfac = csnd / dnrm;
        }
      } else {
        vfac = 0.0f;
      }
      vcell[0] += vfac * d[0];
      vcell[1] += vfac * d[1];
      vcell[2] += vfac * d[2];
    }

    /* We know the desired velocity; now make sure the particle is accelerated
       accordingly during the next time step */
    p->a_hydro[0] = (vcell[0] - p->force.v_full[0]) / p->force.dt;
    p->a_hydro[1] = (vcell[1] - p->force.v_full[1]) / p->force.dt;
    p->a_hydro[2] = (vcell[2] - p->force.v_full[2]) / p->force.dt;
  } else {
    p->a_hydro[0] = 0.0f;
    p->a_hydro[1] = 0.0f;
    p->a_hydro[2] = 0.0f;
  }
}

/**
 * @brief Extra operations done during the kick
 *
 * Not used for Shadowswift.
 *
 * @param p Particle to act upon.
 * @param xp Extended particle data to act upon.
 * @param dt Physical time step.
 */
__attribute__((always_inline)) INLINE static void hydro_kick_extra(
    struct part* p, struct xpart* xp, float dt) {}

/**
 * @brief Returns the internal energy of a particle
 *
 * @param p The particle of interest.
 * @return Internal energy of the particle.
 */
__attribute__((always_inline)) INLINE static float hydro_get_internal_energy(
    const struct part* restrict p) {

  return p->primitives.P / hydro_gamma_minus_one / p->primitives.rho;
}

/**
 * @brief Returns the entropy of a particle
 *
 * @param p The particle of interest.
 * @return Entropy of the particle.
 */
__attribute__((always_inline)) INLINE static float hydro_get_entropy(
    const struct part* restrict p) {

  return p->primitives.P / pow_gamma(p->primitives.rho);
}

/**
 * @brief Returns the sound speed of a particle
 *
 * @param p The particle of interest.
 * @param Sound speed of the particle.
 */
__attribute__((always_inline)) INLINE static float hydro_get_soundspeed(
    const struct part* restrict p) {

  return sqrtf(hydro_gamma * p->primitives.P / p->primitives.rho);
}

/**
 * @brief Returns the pressure of a particle
 *
 * @param p The particle of interest
 * @param Pressure of the particle.
 */
__attribute__((always_inline)) INLINE static float hydro_get_pressure(
    const struct part* restrict p) {

  return p->primitives.P;
}

/**
 * @brief Returns the mass of a particle
 *
 * @param p The particle of interest
 */
__attribute__((always_inline)) INLINE static float hydro_get_mass(
    const struct part* restrict p) {

  return p->conserved.mass;
}

/**
 * @brief Returns the density of a particle
 *
 * @param p The particle of interest
 */
__attribute__((always_inline)) INLINE static float hydro_get_density(
    const struct part* restrict p) {

  return p->primitives.rho;
}

/**
 * @brief Modifies the thermal state of a particle to the imposed internal
 * energy
 *
 * This overrides the current state of the particle but does *not* change its
 * time-derivatives
 *
 * @param p The particle
 * @param u The new internal energy
 */
__attribute__((always_inline)) INLINE static void hydro_set_internal_energy(
    struct part* restrict p, float u) {

  p->conserved.energy = u * p->conserved.mass;
}

/**
 * @brief Modifies the thermal state of a particle to the imposed entropy
 *
 * This overrides the current state of the particle but does *not* change its
 * time-derivatives
 *
 * @param p The particle
 * @param S The new entropy
 */
__attribute__((always_inline)) INLINE static void hydro_set_entropy(
    struct part* restrict p, float S) {

  p->conserved.energy = gas_internal_energy_from_entropy(p->primitives.rho, S) *
                        p->conserved.mass;
}
