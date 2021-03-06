// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//
// $Id: global.C,v 1.10 2002/02/27 12:20:31 sturm Exp $
#include "global.h"

FragmentDB*           frag_db = 0;
FDPB                  fdpb;
AssignChargeProcessor charges("charges/PARSE.crg");
AssignRadiusProcessor radii("radii/PARSE.siz");
ClearChargeProcessor	clear_charge_proc;

ChargeRuleProcessor   charge_rules;
RadiusRuleProcessor   radius_rules;

Options								options;
System								S;

String dump_file;

// default radius is 1.4 Angstrom (water)
float probe_radius = 1.4;

// flags:

// the main options
// true, if the solvent excluded surface is to be calculated
bool ses_calculation = false;

// true, if the solvent accessible surface is to be calculated
bool sas_calculation = false;

// true, if a FDPB calculation is to ber performed
bool fdpb_calculation = false;

// the results of the SES and SAS area calculations
float total_SAS_area = 0.0;
float total_SAS_volume = 0.0;
float total_SES_area = 0.0;
float total_SES_volume = 0.0;

// a hash map containing the atom surfaces of the SAS (if calculated)
HashMap<const Atom*, float> surface_map;


// further options

bool verbose = false;
bool clear_charges = false;
bool normalize_names = false;
bool build_bonds = false;
bool use_charge_rules = false;
bool use_radius_rules = false;
bool calculate_solvation_energy = false;
bool calculate_SES = false;
 

