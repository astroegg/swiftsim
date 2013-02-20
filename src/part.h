/*******************************************************************************
 * This file is part of SWIFT.
 * Coypright (c) 2012 Pedro Gonnet (pedro.gonnet@durham.ac.uk)
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


/* Some constants. */
#define part_maxwait                    3
#define part_maxunlock                  39
#define part_dtmax                      10


/* Condensed data of a single particle. */
struct cpart {

    /* Particle position. */
    double x[3];
    
    /* Particle cutoff radius. */
    float h;
    
    /* Particle time-step. */
    float dt;
    
    } __attribute__((aligned (32)));
    
    
/* Extra particle data not needed during the computation. */
struct xpart {

    /* Old position, at last tree rebuild. */
    double x_old[3];
    
    /* Old velocity. */
    float v_old[3];
    
    /* Old entropy. */
    float u_old;
    
    } __attribute__((aligned (32)));
    

/* Data of a single particle. */
struct part {

    /* Particle velocity. */
    float v[3] __attribute__((aligned (16)));
    
    /* Particle mass. */
    float mass;
    
    /* Particle density. */
    float rho;

    /* Particle velocity divergence. */
    float div_v;

    /* Particle velocity curl. */
    float curl_v[3] __attribute__((aligned (16)));

    /* Balsara switch */
    float Balsara;
  
    /* Particle pressure. */
    // float P;
    
    /* Aggregate quantities. */
    float POrho2;
    
    /* Change in particle energy over time. */
    float u_dt;
    
    /* Change in smoothing length over time. */
    float h_dt;
    
    /* Particle acceleration. */
    float a[3] __attribute__((aligned (16)));
    
    /* Sound speed */
    float c;

    /* Signal velocity */
    float v_sig;
    
    /* Derivative of the density with respect to this particle's smoothing length. */
    float rho_dh;
    
    /* Particle number density. */
    // int icount;
    float wcount;
    float wcount_dh;
    
    /* Particle internal energy. */
    float u;
    
    /* Particle ID. */
    unsigned long long id;
    
    /* Pointer to extra particle data. */
    struct xpart *xtras;
    
    /* Particle position. */
    double x[3];
    
    /* Particle cutoff radius. */
    float h;
    
    /* Particle time-step. */
    float dt;
    
    } __attribute__((aligned (32)));
    

