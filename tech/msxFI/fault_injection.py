import numpy as np
import torch
import random
import pickle
import sys
import time
import cProfile, pstats
from .fi_utils import *
from . import fi_config
from .data_transforms import *

def mat_fi(mat, seed=0, int_bits=2, frac_bits=6, rep_conf = np.array([2, 2, 2, 2, 2, 2, 2, 2]), q_type = 'signed', encode = 'dense', refresh_t=None, vth_sigma=0.05, custom_vdd=None):
  """ Single fault injection experiment for an input matrix with provided quantization, datatype, optional envocing per value to MLCs, and optional sparse encoding
  Or, if mem_model contains 'dram', simulates DRAM faults.

  :param mat: input matrix (can be 1,2,N-dimensional; will be flattened into NVM storage)
  :param seed: random seed for use in fault modeling
  :param int_bits: number of integer bits per value in data format (if applicable)
  :param frac_bits: number of fractional or decimal bits per value in data format (if applicable)
  :param rep_conf: array of number of levels per cell used for storage per data value (SLC default would be np.array([2, 2, 2, 2, 2, 2, 2, 2]) for 8-bit datatype, for example). For DRAM, this will be configured to represent each bit in an SLC-like manner.
  :param q_type: datatype specification (e.g., signed or unsigned, or AdaptivFloat)
  :param encode: indicate whether the data should be mapped into NVM using a sparse encoding (e.g., bitmask) or in standard format (dense).
  :param refresh_t: refresh time in seconds for DRAM models
  :param vth_sigma: standard deviation of Vth in Volts for DRAM fault rate calculation
  :param custom_vdd: custom vdd in volts for DRAM models (optional)
  """
    
  np.random.seed(seed)
  torch.manual_seed(seed)

  if 'dram' in fi_config.mem_model:
    error_map = get_error_map(None, refresh_t=refresh_t, vth_sigma=vth_sigma, custom_vdd=custom_vdd)
    shape = mat.shape
    flattened_mat = torch.from_numpy(mat).view(-1)
    if fi_config.pt_device == "cuda":
      flattened_mat = flattened_mat.to(torch.device(fi_config.pt_device))
    exp_bias = 0
    if q_type == 'afloat': 
      exp_bias = get_afloat_bias(abs(flattened_mat), frac_bits)
    bit_width = get_q_type_bit_width(q_type, int_bits, frac_bits)
    if bit_width is None:
        raise ValueError(f"DRAM mode: unsupported q_type '{q_type}'")
    rep_conf = np.array([2] * bit_width)
    binary_data, mask = convert_mlc_mat(flattened_mat, rep_conf, int_bits, frac_bits, exp_bias, q_type)
    faulty_binary_data = inject_faults(binary_data, rep_conf=None, error_map=error_map)
    flattened_mat = convert_f_mat(faulty_binary_data, rep_conf, int_bits, frac_bits, exp_bias, q_type, mask)
    mat = np.reshape(flattened_mat.cpu().data.numpy(), shape)
    
    return mat

  else:
    error_map = get_error_map(max(rep_conf), vth_sigma=vth_sigma)
    
    shape = mat.shape

    flattened_mat = torch.from_numpy(mat).view(-1)
    if fi_config.pt_device == "cuda":
      flattened_mat = flattened_mat.to(torch.device(fi_config.pt_device))
    exp_bias = 0
    if q_type == 'afloat': #support for adaptive float
      exp_bias = get_afloat_bias(abs(flattened_mat), frac_bits)

    if encode == 'dense': #no sparse encoding, just inject on dense weight matrix
      mlc_values = torch.zeros((flattened_mat.size()[0], rep_conf.size), device=fi_config.pt_device, dtype=torch.float32)
      mlc_values, mask = convert_mlc_mat(flattened_mat, rep_conf, int_bits, frac_bits, exp_bias, q_type)
      mlc_values = inject_faults(mlc_values, rep_conf, error_map)
      flattened_mat = convert_f_mat(mlc_values, rep_conf, int_bits, frac_bits, exp_bias, q_type, mask)
    elif encode == 'bitmask': #encode data with bitmask FIXME assumed bitmask always stored with SLC and inject on bitmask and non-zero data
      bitmask, data = to_bitmask(flattened_mat)
      #optional check capcity of encoded version
      #print(encoded_capacity(bitmask, data, int_bits+frac_bits)/8.0/1024.0, "KB")
      #set up and inject weights
      mlc_values=torch.zeros((data.size()[0], rep_conf.size), device=fi_config.pt_device, dtype=torch.float32)
      mlc_values, data_mask = convert_mlc_mat(data, rep_conf, int_bits, frac_bits, exp_bias, q_type)
      mlc_values = inject_faults(mlc_values, rep_conf, error_map)
      data = convert_f_mat(mlc_values, rep_conf, int_bits, frac_bits, exp_bias, q_type, data_mask)
      #set up and inject bitmask
      mlc_bitmask, bitmask_mask = convert_mlc_mat(bitmask, np.array([2]), 1, 0, 0, 'unsigned')
      mlc_bitmask = inject_faults(mlc_bitmask, np.array([2]), error_map)
      bitmask = convert_f_mat(mlc_bitmask, np.array([2]), 1, 0, 0, 'unsigned', bitmask_mask)
      #decode
      flattened_mat = from_bitmask(bitmask, data)

    mat = np.reshape(flattened_mat.cpu().data.numpy(), shape)

    return mat

def dnn_fi(model=None, model_def_path=None, model_path=None, seed=0, int_bits=2, frac_bits=6, rep_conf = np.array([2, 2, 2, 2, 2, 2, 2, 2]), q_type = 'signed', encode = 'dense', refresh_t=None, vth_sigma=0.05, custom_vdd=None):
  """ Single fault injection experiment for an input DNN model.
  Supports NVM fault injection (original mode) or DRAM fault injection.

  :param model: input dnn model (injection on all weights) - for backward compatibility
  :param model_def_path: path to Python file containing model class definition
  :param model_path: path to saved model file (.pth)
  :param seed: random seed for fault modeling
  :param int_bits: number of integer bits for data format
  :param frac_bits: number of fractional bits for data format
  :param rep_conf: NVM: array of levels per cell. DRAM: bits are SLC.
  :param q_type: datatype specification ('signed', 'unsigned', 'afloat')
  :param encode: NVM: 'dense' or 'bitmask'.
  :param refresh_t: refresh time in seconds for DRAM models
  :param vth_sigma: standard deviation of Vth in Volts for DRAM fault rate calculation
  :param custom_vdd: custom vdd in volts for DRAM models (optional)
  """ 
  np.random.seed(seed)
  torch.manual_seed(seed)

  if model is None and model_def_path is not None and model_path is not None:
    print(f"Loading model class from {model_def_path}...")
    import_model_class(model_def_path)
    print(f"Successfully imported model class from {model_def_path}")
    
    print(f"Loading DNN model from {model_path}...")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    model = torch.load(model_path, map_location=device, weights_only=False)
    model.to(device)
    print(f"Loaded DNN model from {model_path}. Model type: {type(model).__name__}")

  if 'dram' in fi_config.mem_model:
    error_map = get_error_map(None, refresh_t=refresh_t, vth_sigma=vth_sigma, custom_vdd=custom_vdd)
    for name, weights in model.named_parameters():
        if weights.requires_grad:
            w = weights.data
            flattened_weights = w.view(-1).to(fi_config.pt_device)
            exp_bias = 0
            if q_type == 'afloat':
                exp_bias = get_afloat_bias(abs(flattened_weights), frac_bits) 
            bit_width = get_q_type_bit_width(q_type, int_bits, frac_bits)
            if bit_width is None:
                raise ValueError(f"DRAM mode: unsupported q_type '{q_type}'")
            rep_conf = np.array([2] * bit_width)
            binary_representation, mask = convert_mlc_mat(flattened_weights, rep_conf, int_bits, frac_bits, exp_bias, q_type)
            faulty_binary_representation = inject_faults(binary_representation, rep_conf=None, error_map=error_map)
            flattened_weights = convert_f_mat(faulty_binary_representation, rep_conf, int_bits, frac_bits, exp_bias, q_type, mask)
            weights.data.copy_(flattened_weights.view(w.size()))
            
    return model

  else:
    error_map = get_error_map(max(rep_conf), vth_sigma=vth_sigma)
    for name, weights in model.named_parameters():
      w = weights.data
      flattened_weights = w.view(-1)
        
      exp_bias = 0
      if q_type == 'afloat': #support for adaptive float
        exp_bias = get_afloat_bias(abs(flattened_weights), frac_bits)

      if encode == 'dense': #no sparse encoding, just inject on dense weight matrix
        mlc_weights=torch.zeros((flattened_weights.size()[0], rep_conf.size), device=fi_config.pt_device, dtype=torch.float32)
        mlc_weights, mask = convert_mlc_mat(flattened_weights, rep_conf, int_bits, frac_bits, exp_bias, q_type)
        mlc_weights = inject_faults(mlc_weights, rep_conf, error_map)
        flattened_weights = convert_f_mat(mlc_weights, rep_conf, int_bits, frac_bits, exp_bias, q_type, mask)
        model.state_dict()[name].data.copy_(flattened_weights.view(w.size()))
      elif encode == 'bitmask': #encode data with bitmask FIXME assumed bitmask always stored with SLC and inject on bitmask and non-zero data
        bitmask, data = to_bitmask(flattened_weights)
        #optional check capcity of encoded version
        #print(encoded_capacity(bitmask, data, int_bits+frac_bits)/8.0/1024.0, "KB")
        #set up and inject weights
        mlc_weights=torch.zeros((data.size()[0], rep_conf.size), device=fi_config.pt_device, dtype=torch.float32)
        mlc_weights, data_mask = convert_mlc_mat(data, rep_conf, int_bits, frac_bits, exp_bias, q_type)
        mlc_weights = inject_faults(mlc_weights, rep_conf, error_map)
        data = convert_f_mat(mlc_weights, rep_conf, int_bits, frac_bits, exp_bias, q_type, data_mask)
        #set up and inject bitmask
        mlc_bitmask, bitmask_mask = convert_mlc_mat(bitmask, np.array([2]), 1, 0, 0, 'unsigned')
        mlc_bitmask = inject_faults(mlc_bitmask, np.array([2]), error_map)
        bitmask = convert_f_mat(mlc_bitmask, np.array([2]), 1, 0, 0, 'unsigned', bitmask_mask)
        #decode
        flattened_weights = from_bitmask(bitmask, data)
        model.state_dict()[name].data.copy_(flattened_weights.view(w.size()))
            
    return model
