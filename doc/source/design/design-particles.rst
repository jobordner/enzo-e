.. include:: ../roles.incl

****************
Particles Design
****************
.. toctree::

============
Requirements
============

======
Design
======

Particles are organized into `types`, where each type is defined using
some number of `attributes`, and particle attributes are stored in
some number of arrays called `batches`.

.. image:: particles.png

**Types**. Cello supports multiple different particle `types`. Some
types used in Enzo-E include ``"dark"`` for dark-matter particles, and
``"trace"`` for tracer particles. Particle types are identified using
an integer index, and by convention are stored in variables named
e.g. ``it`` (when operating on a single type, or looping over multiple
types), ``it_dark``, ``it_trace``, etc.; the total number of types is
stored as ``nt``. Types are indicated in yellow in the above figure.

**Attributes**. Each particle type can have one or more particle
`attributes`. All particles must have at least have attributes to
represent the position of a particle, such as ``x``, ``y``, and
``z``. Other attributes, such as mass, velocity, etc., are
optional. Particle attributes have user-defined types, which can be
integer or floating-point values. Attributes are indentified using an
integer index, and by convention are stored in variables named
e.g. ``ia_y`` (y-component of position), ``ia_mass``, ``ia_density``,
``ia_vz`` (z-component of velocity), etc.; the number of attributes is
stored as ``np``, ``np_dark``, or ``np_trace``. Attributes are
indicated in green in the above figure.

**Batches**. Particle attributes are stored in arrays called
`batches`, which span some fixed number of particles of a given type,
and all attributes.  By default, the 'attribute' array axis varies
fastest, but can be overridden so that the 'particle' array axis
varies fastest, in which case the particles are said to be
`interleaved`.  By convention batches are indexed using variables
``ib``, ``ib_dark``, ``ib_trace``, etc., and particles `within a
batch` are indexed using variables ``ip``, ``ip_dark``, ``ip_trace``,
etc., and the number of particles in the current batch as ``np``,
``np_dark``, or ``np_trace``. In the above figure, batch arrays are
represeted in gray, batch indices in magenta, and particle indices in
cyan. Batch array interelaving is represented in blue, where 'i'
represents the fastest-varying axis.

Interfaces
==========

-------------------
ParticleDescr class
-------------------

------------------
ParticleData class
------------------

--------------
Particle class
--------------

.. code-block:: C++
   :linenos:

     // Proposed code for accessing "mass" or "density" particle attributes (as
     // both) whether constant or not per particle type

     // Get particle type and position attributes
     const it =   particle.type_index("dark");
     const ia_x = particle.attribute_index(it,"x");
     const ia_y = particle.attribute_index(it,"y");
     const ia_z = particle.attribute_index(it,"z");

     // Get 'mass or density' attribute ia_mord, and scalings to convert
     // to mass or to density. Exactly one of "mass" or "density" attribute or constant
     // must be defined

     int ia_mord;
     enzo_float to_mass,to_density;

     // Determine cell volume (assumes particle density defined in terms of
     // containing block cells)
     double h3[3];
     block->cell_width(h3,h3+1,h3+2)
     const enzo_float volume = h3[0]*h3[1]*h3[2];

     // Which of mass or density attribute / constant is defined
     const bool mass_defined    = particle.is_attribute(it,"mass");
     const bool density_defined = particle.is_attribute(it,"density");

     if (mass_defined && (! density_defined)) {
       // "mass" constant or attribute is defined; compute scaling for
       // density (1.0 for mass)
       ia_mord = particle.attribute_index,"mass");
       to_mass = 1.0;
       to_density = 1.0/(hx*hy*hz);
     } else if (density_defined && (! mass_defined)) {
       // "density" constant or attribute is defined; compute scaling for
       // mass (1.0 for density)
       ia_mord = particle.attribute_index,"density");
       to_mass = hx*hy*hz;
       to_density = 1.0;
     } else {
       ERROR (..., "Exactly one of \"mass\" or \"density\" must be defined!");
     }

     // Get attribute index strides. This is normally 1, but may be greater
     // than 1 if attributes are interleaved, or may be 0 if the attribute
     // is really a constant
     const int is_x = particle.stride(it,ia_x);
     const int is_y = particle.stride(it,ia_y);
     const int is_z = particle.stride(it,ia_z);
     const int is_mord = particle.stride(it,ia_mord);

     // Loop over particles batch by batch
     int nb = particle.num_batches(it);
     for (int ib=0; ib<nb; ib++) {
        const int np = particle.num_particles (it,ib);
        enzo_float * ax    = particle.attribute_array(it,ia_x,ib);
        enzo_float * ay    = particle.attribute_array(it,ia_y,ib);
        enzo_float * az    = particle.attribute_array(it,ia_z,ib);
        enzo_float * amord = particle.attribute_array(it,ia_mord,ib);
        for (int ip=0; ip<np; ip++) {
           enzo_float x = ax[ip*is_x];
           enzo_float y = ay[ip*is_y];
           enzo_float z = az[ip*is_z];
           enzo_float mass    = mord[ip*is_mord]*to_mass,,
           enzo_float density = mord[ip*is_mord]*to_density);
           printf ("x %g y %g z %g mass %g density %g\n",
              x,y,z,mass,density);
        }
     }
