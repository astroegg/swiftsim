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
#ifndef SWIFT_ACTIVE_H
#define SWIFT_ACTIVE_H

/* Config parameters. */
#include "../config.h"

/* Local includes. */
#include "cell.h"
#include "const.h"
#include "engine.h"
#include "part.h"

/**
 * @brief Does a cell contain any active particle ?
 *
 * @param c The #cell.
 * @param e The #engine containing information about the current time.
 */
__attribute__((always_inline)) INLINE static int cell_is_active(
    const struct cell *c, const struct engine *e) {

#ifdef SWIFT_DEBUG_CHECKS
  if (c->ti_end_min < e->ti_current)
    error("cell in an impossible time-zone! c->ti_end_min=%d e->ti_current=%d",
          c->ti_end_min, e->ti_current);
#endif

  return (c->ti_end_min == e->ti_current);
}

/**
 * @brief Are *all* particles in a cell active ?
 *
 * @param c The #cell.
 * @param e The #engine containing information about the current time.
 */
__attribute__((always_inline)) INLINE static int cell_is_all_active(
    const struct cell *c, const struct engine *e) {

#ifdef SWIFT_DEBUG_CHECKS
  if (c->ti_end_max < e->ti_current)
    error("cell in an impossible time-zone! c->ti_end_max=%d e->ti_current=%d",
          c->ti_end_max, e->ti_current);
#endif

  return (c->ti_end_max == e->ti_current);
}

/**
 * @brief Checks whether a given cell needs drifting or not.
 *
 * @param c the #cell.
 * @param e The #engine (holding current time information).
 *
 * @return 1 If the cell needs drifting, 0 otherwise.
 */
INLINE static int cell_is_drift_needed(struct cell *c, const struct engine *e) {

  /* Do we have at least one active particle in the cell ?*/
  if (cell_is_active(c, e)) return 1;

  /* Loop over the pair tasks that involve this cell */
  for (struct link *l = c->density; l != NULL; l = l->next) {

    if (l->t->type != task_type_pair && l->t->type != task_type_sub_pair)
      continue;

    /* Is the other cell in the pair active ? */
    if ((l->t->ci == c && cell_is_active(l->t->cj, e)) ||
        (l->t->cj == c && cell_is_active(l->t->ci, e)))
      return 1;
  }

  /* No neighbouring cell has active particles. Drift not necessary */
  return 0;
}



/**
 * @brief Is this particle active ?
 *
 * @param p The #part.
 * @param e The #engine containing information about the current time.
 */
__attribute__((always_inline)) INLINE static int part_is_active(
    const struct part *p, const struct engine *e) {

#ifdef SWIFT_DEBUG_CHECKS
  if (p->ti_end < e->ti_current)
    error("particle in an impossible time-zone! p->ti_end=%d e->ti_current=%d",
          p->ti_end, e->ti_current);
#endif

  return (p->ti_end == e->ti_current);
}

/**
 * @brief Is this g-particle active ?
 *
 * @param gp The #gpart.
 * @param e The #engine containing information about the current time.
 */
__attribute__((always_inline)) INLINE static int gpart_is_active(
    const struct gpart *gp, const struct engine *e) {

#ifdef SWIFT_DEBUG_CHECKS
  if (gp->ti_end < e->ti_current)
    error(
        "g-particle in an impossible time-zone! gp->ti_end=%d e->ti_current=%d",
        gp->ti_end, e->ti_current);
#endif

  return (gp->ti_end == e->ti_current);
}

#endif /* SWIFT_ACTIVE_H */
