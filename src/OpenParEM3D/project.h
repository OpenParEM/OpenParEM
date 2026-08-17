////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    OpenParEM3D - A fullwave 3D electromagnetic simulator.                  //
//    Copyright (C) 2026 Brian Young                                          //
//                                                                            //
//    This program is free software: you can redistribute it and/or modify    //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //
//    This program is distributed in the hope that it will be useful,         //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// Structure for maintaining and using project setup information in OpenParEM tools.
// This is a structure rather than a class so that it can be passed to C programs.

#ifndef PROJECT_H
#define PROJECT_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <quadmath.h>
#include "inputFrequency.h"
#include "petscsys.h"
#include "petscsystypes.h"

char* removeNewLineChar (char *);
char *removeComment (char *);
int removeQuote (char *);
void removeQuotes (char *);

struct inputAntennaPattern {
   int lineNumber;
   int dim;
   char *quantity1;
   char *quantity2;
   char *plane;
   double theta,phi,latitude,rotation;
};

struct physicalGroupMaterial {
    int lineNumber;        // line number in the project file
    int dim;               // dimension of the physicalGroup in the Brep model
    int tag;               // tag of the physicalGroup in the Brep model
    char *materialName;    // material name for the physical group
};

struct projectData {
   char* version_name;       // set in init_project 
   char* version_value;      // set in init_project

   // user settable

   int project_calculate_poynting;
   int project_save_fields;

   char *mesh_file;
   int mesh_save_refined;
   double mesh_2D_refinement_fraction;
   double mesh_3D_refinement_fraction;
   double mesh_quality_limit;

   int fem_order;

   char *port_definition_file;

   char *materials_global_path;
   char *materials_global_name;
   char *materials_local_path;
   char *materials_local_name;
   char *materials_default_boundary;
   int materials_check_limits;

   char *refinement_frequency;
   int refinement_iteration_min;
   int refinement_iteration_max;
   int refinement_required_passes;
   double refinement_relative_tolerance;
   double refinement_absolute_tolerance;
   char *refinement_variable;

   struct inputFrequencyPlan *inputFrequencyPlans;
   unsigned long int inputFrequencyPlansAllocated;
   unsigned long int inputFrequencyPlansCount;

   double reference_impedance;
   char *touchstone_frequency_unit;
   char *touchstone_format;

   int solution_modes;
   double solution_temperature;
   double solution_2D_tolerance;
   double solution_3D_tolerance;
   int solution_iteration_limit;
   int solution_modes_buffer;
   char *solution_impedance_definition;
   char *solution_impedance_calculation;
   int solution_check_closed_loop;
   int solution_check_homogeneous;
   int solution_accurate_residual;
   int solution_shift_invert;
   double solution_shift_factor;
   int solution_initial_guess_level;

   int inputAntennaPatternsCount;
   int inputAntennaPatternsAllocated;
   struct inputAntennaPattern *inputAntennaPatterns;

   double antenna_plot_current_resolution;
   double antenna_plot_2D_range;
   double antenna_plot_2D_interval;
   double antenna_plot_2D_resolution;
   int antenna_plot_2D_annotations;
   int antenna_plot_2D_save;
   int antenna_plot_3D_refinement;
   int antenna_plot_3D_sphere;
   int antenna_plot_3D_save;
   int antenna_plot_raw_save;

   int output_show_refining_mesh;
   int output_show_postprocessing;
   int output_show_iterations;
   int output_show_license;

   int test_create_cases;
   int test_show_detailed_cases;

   int debug_show_memory;
   int debug_show_project;
   int debug_show_frequency_plan;
   int debug_show_materials;
   int debug_show_port_definitions;
   int debug_show_impedance_details;
   int debug_save_port_fields;
   int debug_skip_mixed_conversion;
   int debug_skip_forced_reciprocity;
   int debug_tempfiles_keep;
   int debug_refine_preconditioner;

   int field_points_count;
   int field_points_allocated;
   double *field_points_x;
   double *field_points_y;
   double *field_points_z;

   // GUI keywords - not required for standalone operation

   char *gui_units;
   int gui_slot_count;
   int gui_oversubscribe;
   double gui_grid_size;
   double gui_mesh_scale;
   double gui_mesh_minSize;
   double gui_mesh_maxSize;

   // keyword and format:
   int physicalGroupMaterialCount;
   int physicalGroupMaterialAllocated;
   struct physicalGroupMaterial *physicalGroupMaterials;

   // not user settable

   char *project_name;

   int version_major;
   int version_minor;
   int version_patch;

   int modified;
};

#endif
