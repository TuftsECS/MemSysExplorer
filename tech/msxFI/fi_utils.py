import numpy as np
import torch
import random
from statistics import NormalDist
import scipy.stats as ss
import pickle
import sys
import os
import time
import cProfile, pstats
import re
import math
from .data_transforms import * 
from . import fi_config
import importlib.util

def get_error_map(max_lvls_cell, refresh_t=None, vth_sigma=0.05, custom_vdd=None):
  """
  Retrieve the correct per-storage-cell error map for the configured NVM settings according to the maximum levels-per-cell used
  OR generate DRAM error map based on physical parameters

  :param max_lvls_cell: Across the storage settings for fault injection experiment, provide the maximum number of levels-per-cell required (max 16 for 4BPC for provided fault models)
  :param refresh_t: Refresh time in seconds for DRAM models
  :param vth_sigma: Standard deviation of Vth in Volts for DRAM fault rate calculation
  :param custom_vdd: Custom vdd in volts for DRAM models (optional)
  """
  if 'dram' in fi_config.mem_model:
    mem_data_path = os.path.dirname(__file__)
    print("Using DRAM model " + fi_config.mem_model)
    dram_params_path = os.path.join(mem_data_path, 'mem_data', fi_config.mem_dict[fi_config.mem_model])
    with open(dram_params_path, 'rb') as f:
        dram_params_data = pickle.load(f)
    available_sizes = sorted(dram_params_data.keys())
    # Select the largest available size that is <= target feature_size
    selected_size = None
    for size in reversed(available_sizes):
      if size <= fi_config.feature_size:
        selected_size = size
        break
    if selected_size is None:
      selected_size = available_sizes[0]
    tech_node_data = dram_params_data[selected_size]
    dist_args = (tech_node_data, fi_config.temperature, selected_size)
    
    fault_prob = fault_rate_gen(dist_args, refresh_t, vth_sigma, custom_vdd)
    error_map = np.zeros(1, dtype=object)  # DRAM uses SLC
    error_map[0] = np.zeros((2, 2))
    error_map[0][0, 1] = 0.0  # 0->1 fault probability = 0
    error_map[0][1, 0] = fault_prob  # 1->0 fault probability
  
  else:
    #max_lvls_cell is the maximum number of levels encoded in a nvm cell
    mem_data_path = os.path.dirname(__file__)
    print("Using NVM model "+ fi_config.mem_model)
    f = open(mem_data_path+'/mem_data/'+fi_config.mem_dict[fi_config.mem_model], 'rb')
    args_lut = pickle.load(f)
    
    # computes the probability of level faults (either up or down one level)
    # for each possible cell configuration in fixed point representation (up to max_lvls_cell levels)

    emap_entries = int(np.log2(max_lvls_cell))
    error_map = np.zeros(emap_entries, dtype=object)
    if len(args_lut) < emap_entries:
      raise SystemExit("ERROR: model does not support "+str(emap_entries)+"-bit cells")

    for i in range(emap_entries):
      # create a list so for each level we get a probability of a fault shifting a level up or down one.
      num_levels = int(2**(i+1))
      error_map[i] = np.zeros((num_levels, 2))

      #always solve Gauss to be sure, otherwise # of faults is overestimated
      for j in range(num_levels-1):
        th = get_temp_th(args_lut[i], j)
        # print("Threshold ", th)
        dist_args = (th, *args_lut[i][j+1])
        # print("Args ", dist_args)
        error_map[i][j+1, 0] = fault_rate_gen(dist_args, vth_sigma=vth_sigma)
        dist_args = (th, *args_lut[i][j])
        # print("Args ", dist_args)
        error_map[i][j, 1] = 1. - fault_rate_gen(dist_args, vth_sigma=vth_sigma)

    if fi_config.Debug:
      for i, emap in enumerate(error_map):
        print("Error map for", int(2**(i+1)), "levels")
        print(emap, "\n\n")
  
  return error_map

 
def fault_rate_gen(dist_args, refresh_t=None, vth_sigma=0.05, custom_vdd=None):
  """
  Randomly generate fault rate per experiment and storage cell config according to fault model

  :param dist_args: arguments describing the distribution of level-to-level faults (programmed level means and sdevs) for RRAM, 
                    or tuple of (tech_node_data, temperature, selected_size) for DRAM
  :param refresh_t: refresh time in seconds for DRAM (required for DRAM models)
  :param vth_sigma: standard deviation of Vth in Volts for DRAM fault rate calculation
  :param custom_vdd: custom vdd in volts for DRAM models (optional)
  """
  if 'rram' in fi_config.mem_model:
    x = dist_args[0]
    mu = dist_args[1]
    sigma = dist_args[2]
    cdf = NormalDist(mu, sigma).cdf(x)
    return cdf
  elif 'fefet' in fi_config.mem_model:
    cdf = ss.gamma.cdf(*dist_args)
    return cdf
  elif 'dram' in fi_config.mem_model:
    # Physical constants
    kB = 1.380649e-23  # Boltzmann constant in J/K
    q = 1.60217663e-19  # Elementary charge in C

    # DRAM fault rate calculation
    if refresh_t is None:
      raise ValueError("refresh_t is required for DRAM models")
    tech_node_data, temperature, selected_size = dist_args
    cap_F = tech_node_data['CellCap']
    vdd = custom_vdd if custom_vdd is not None else tech_node_data['vdd']
    # Find the closest temperature to the target temperature
    available_temps = sorted(tech_node_data['Ioff'].keys())
    temp_diffs = [abs(temp - temperature) for temp in available_temps]
    closest_temp_idx = temp_diffs.index(min(temp_diffs))
    selected_temp = available_temps[closest_temp_idx]
    
    mu_Ioff = tech_node_data['Ioff'][selected_temp]

    # Calculate Ioff distribution parameters
    Vt = (kB * selected_temp) / q
    # When Vgs ↑ by SS mV → Isub ↑ 10× ⇒ Isub ∝ 10^((Vgs - Vth) / SS)
    # Isub ≈ I0 · exp[(Vgs - Vth) / (n·Vt)] → exponential dominates
    # Use 10^x = e^(x·ln(10)): Isub ∝ exp[(Vgs - Vth) · ln(10) / SS]
    # Comparing exponents: ln(10)/SS = 1 / (n·Vt) ⇒ n = SS / (Vt · ln(10))
    n_factor = fi_config.SS * 1e-3 / (Vt * math.log(10))
    # Compute stddev of ln(Ioff) from threshold voltage variation
    sigma_ln_Ioff = vth_sigma / (n_factor * Vt)
    
    # For log-normal distribution: if X ~ LogNormal(μ, σ), then ln(X) ~ Normal(μ, σ)
    # We need μ and σ such that E[X] = mu_Ioff
    # E[X] = exp(μ + σ²/2) → μ = ln(E[X]) - σ²/2
    mu_ln_ioff = math.log(mu_Ioff) - (sigma_ln_Ioff**2) / 2

    # Calculate critical current and fault probability using log-normal CDF
    I_critical = (cap_F * vdd / 2) / refresh_t
    
    # For log-normal distribution, P(X > I_critical) = P(ln(X) > ln(I_critical))
    # where ln(X) ~ Normal(mu_ln_ioff, sigma_ln_Ioff)
    ln_I_critical = math.log(I_critical)
    
    # P(Ioff > I_critical) using normal CDF in log space
    cdf = 1.0 - NormalDist(mu=mu_ln_ioff, sigma=sigma_ln_Ioff).cdf(ln_I_critical)
    cdf = max(0, cdf)
    
    print(f"DRAM Params: Ioff={mu_Ioff:.2e}A, I_critical={I_critical:.2e}A, Bit-flip Rate (1->0): {cdf*100:.2f}%")
    
    return cdf
  else:
    raise SystemExit("ERROR: model not defined; please update fi_config.py")
  
# Use this when std dev btwn levels are not even
def solveGauss(mu1, sdev1, mu2, sdev2):
  """
  Helper function to compute intersection of two normal distributions; used to calculate probability of level-to-level fault for specific current/voltage distributions

  :param mu1: mean of first distribution
  :param mu2: mean of second distribution
  :param sdev1: standard dev of first distribution
  :param sdev2: standard dev of second distribution
  """
  a = 1./(2*sdev1**2) - 1./(2*sdev2**2)
  b = mu2/(sdev2**2) - mu1/(sdev1**2)
  c = mu1**2/(2*sdev1**2) -  mu2**2/(2*sdev2**2) - np.log(sdev2/sdev1)
  return np.roots([a, b, c])

def get_temp_th(dist_args, lvl):
  """
  Helper function to compute threshold for detecting a mis-read storage cell according to input fault model and stored value

  :param dist_args: arguments describing the distribution of level-to-level faults
  :param lvl: programmed value to specific memory cell (e.g., 0 or 1 for SLC)
  """
  if 'rram' in fi_config.mem_model:
    temp_th = solveGauss(dist_args[lvl][0], dist_args[lvl][1], dist_args[lvl+1][0], dist_args[lvl+1][1])
    for temp in temp_th:
      if temp > dist_args[lvl][0] and temp < dist_args[lvl+1][0]:
        th = temp
  elif 'fefet' in fi_config.mem_model:
    th = 0.5*(ss.gamma.median(*dist_args[lvl])+ss.gamma.median(*dist_args[lvl+1]))
  else:
    raise SystemExit("ERROR: model not defined; please update fi_config.py")
  return th

def inject_faults(weights, rep_conf=None, error_map=None):
  """
  Perform fault injection on input MLC-packed data values according to storage settings and fault model (NVM)
  or on binary data according to a single fault rate (DRAM).

  :param weights: MLC-packed data values (NVM) or binary data tensor (DRAM).
  :param rep_conf: storage setting dictating bits-per-cell per data value (NVM). Unused for DRAM.
  :param error_map: generated base fault rates according to storage configs and fault model (NVM), or single fault rate (DRAM).
  """
  
  if 'dram' in fi_config.mem_model:
    random_tensor = torch.rand_like(weights, device=weights.device)
    ones_mask = (weights == 1)
    fault_prob = error_map[0][1, 0]
    fault_mask = (random_tensor < fault_prob)
    actual_faults_to_inject = ones_mask & fault_mask
    weights[actual_faults_to_inject] = 0
    total_num_faults = torch.sum(actual_faults_to_inject).item()
    
    if total_num_faults > 0:
      faulty_indices = torch.where(actual_faults_to_inject)
      affected_elements = torch.unique(faulty_indices[0]).numel()
      total_elements = weights.shape[0]
      print(f"Number of generated faults: {total_num_faults}")
      print(f"Number of affected data values: {affected_elements} (out of {total_elements})")
    else:
      print(f"Number of generated faults: 0")

    return weights
  else:
    # perform fault injection
    total_num_faults = 0

    for cell in range(np.size(rep_conf)):
      max_level = rep_conf[cell] - 1

      cell_error_map_index = int(np.log2(rep_conf[cell])) - 1
      cell_errors = error_map[cell_error_map_index]

      # Loop through all possible levels for cell
      for lvl in range(rep_conf[cell]):
        lvl_cell_addresses = np.where(weights[:, cell].cpu().numpy() == lvl)[0]
        if len(lvl_cell_addresses) > 0:
          # Get error probabilities for both up and down transitions
          # the probability of min level going down and max level going up is always 0

          prob_faults_down = cell_errors[lvl][0]
          prob_faults_up = cell_errors[lvl][1]

          # Compute total number of errors for lvl
          num_lvl_faults = int((prob_faults_up+prob_faults_down) * lvl_cell_addresses.size)

          if num_lvl_faults > 0:

            faulty_lvls_indexes = np.random.choice(lvl_cell_addresses, int(num_lvl_faults), replace=False)
            # divide the total number of faults according to up/down fault ratio
            faulty_middle = int(faulty_lvls_indexes.size * prob_faults_up /(prob_faults_up + prob_faults_down))
            
            if prob_faults_up > 0:
              weights[faulty_lvls_indexes[:faulty_middle], cell] += 1
            if prob_faults_down > 0:
              weights[faulty_lvls_indexes[faulty_middle:], cell] -= 1 
            
            total_num_faults += num_lvl_faults
    
    if total_num_faults > 0:
      print(f"Number of generated faults: {total_num_faults}")
    else:
      print(f"Number of generated faults: 0")

    if (torch.sum(weights[:, cell] > max_level) != 0) or (torch.sum(weights[:, cell] < 0) != 0):
      print("ERROR: fault injection out of bound")
      sys.exit(0)

    return weights
  
def import_model_class(py_path):
    """Import model class from the specified Python file."""
    spec = importlib.util.spec_from_file_location("model", py_path)
    model_module = importlib.util.module_from_spec(spec)
    sys.modules["model"] = model_module
    spec.loader.exec_module(model_module)
    return model_module
  
def get_q_type_bit_width(q_type, int_bits=0, frac_bits=0):
    """Returns the total bit width for a given q_type."""
    width_map = {
        'float16': 16,
        'bfloat16': 16,
        'float32': 32,
        'float64': 64,
    }
    if q_type in width_map:
        return width_map[q_type]
    elif q_type in ['signed', 'unsigned', 'afloat', 'int']:
        return int_bits + frac_bits
    else:
        return None # Unknown q_type

def validate_config(args, rep_conf):
    """
    Validates memory and quantization configuration.
    Returns True if valid, False otherwise.
    """
    # --- Validation and Defaulting for q_type vs. int_bits/frac_bits ---
    float_types = ['float16', 'bfloat16', 'float32', 'float64']
    fixed_point_types = ['signed', 'unsigned', 'afloat', 'int']
    
    if args.q_type in float_types:
        if args.int_bits is not None or args.frac_bits is not None:
            print(f"Error: --int_bits and --frac_bits are not applicable for q_type '{args.q_type}'.")
            print("Please remove these arguments when using floating-point q_types.")
            return False
        args.int_bits = 0
        args.frac_bits = 0
    elif args.q_type in fixed_point_types:
        if args.int_bits is None:
            args.int_bits = 2  # Default for fixed-point
            print(f"Info: --int_bits not provided for fixed-point type, using default value of {args.int_bits}.")
        if args.frac_bits is None:
            args.frac_bits = 4  # Default for fixed-point
            print(f"Info: --frac_bits not provided for fixed-point type, using default value of {args.frac_bits}.")

    # --- Validation for NVM capacity ---
    if 'dram' in args.mode:
        return True # This validation is for NVM models

    q_type_width = get_q_type_bit_width(args.q_type, args.int_bits, args.frac_bits)
    if q_type_width is None:
        print(f"ERROR: Unsupported q_type '{args.q_type}'.")
        print(f"Supported q_types: float16, bfloat16, float32, float64, signed, unsigned, afloat, int")
        return False

    rep_conf_capacity = 0
    for level in rep_conf:
        if level <= 1 or (level & (level - 1) != 0):
            print(f"ERROR: rep_conf values must be powers of 2 and > 1. Found: {level}")
            return False
        rep_conf_capacity += math.log2(level)
    
    rep_conf_capacity = int(rep_conf_capacity)

    if rep_conf_capacity > q_type_width:
        print(f"\nERROR: rep_conf capacity ({rep_conf_capacity} bits) is greater than q_type width ({q_type_width} bits).")
        print("This configuration is invalid because you cannot store more bits than the data type provides.")
        return False

    if rep_conf_capacity < q_type_width:
        print(f"\nERROR: rep_conf capacity ({rep_conf_capacity} bits) is less than q_type width ({q_type_width} bits).")
        print("This configuration is invalid because all data bits must be mapped to a cell.")
        return False

    if rep_conf_capacity == q_type_width:
        print(f"Configuration valid: rep_conf capacity ({rep_conf_capacity} bits) matches q_type width ({q_type_width} bits).")

    return True
  
