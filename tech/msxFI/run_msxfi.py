import sys
import os

# Add the parent directory of this script's location to the system path
# to allow for absolute imports of 'msxFI' components.
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import numpy as np
import argparse
import torch
import random

def parse_args():
    parser = argparse.ArgumentParser(description="Run NVM/DRAM fault injection experiments.")
    parser.add_argument('--mode', type=str, default='rram_mlc', 
                        help="Memory model to use (rram_mlc, dram3t, dram1t, etc.). See fi_config.py for available models.")
    # DRAM specific
    parser.add_argument('--refresh_t', type=float, 
                        help="DRAM refresh time in microseconds (used for DRAM modes).")
    parser.add_argument('--vth_sigma', type=float, default=50, 
                        help="Standard deviation of threshold voltage (Vth) in mV for DRAM fault rate calculation (default: 50mV).")
    parser.add_argument('--vdd', type=float, 
                        help="Custom vdd in volts for DRAM modes. If not provided, uses default vdd from pickle file.")
    # MLC specific
    parser.add_argument('--rep_conf', nargs='*', default=[8, 8],
                        help="Array of number of levels per cell used for storage per data value, e.g.: --rep_conf 8 8")

    # Quantization and fault injection
    parser.add_argument('--int_bits', type=int, default=None, 
                        help="Number of integer bits for data quantization (for fixed-point types like 'signed', 'afloat').")
    parser.add_argument('--frac_bits', type=int, default=None, 
                        help="Number of fractional bits for data quantization (for fixed-point types like 'signed', 'afloat').")
    parser.add_argument('--q_type', type=str, default='afloat', 
                        help="Quantization type (e.g., 'signed', 'afloat').")
    parser.add_argument('--seed', type=int, default=None, 
                        help="Random seed for fault injection. If not provided, a random seed will be used.")
    parser.add_argument('--matrix_size', type=int, default=1000, 
                        help="Size N for the NxN test matrix (for matrix FI modes).")

    # DNN evaluation
    parser.add_argument('--eval_dnn', action='store_true', default=False,
                        help="Enable DNN fault injection for the selected mode.")
    parser.add_argument('--model', type=str, default='example_nn/lenet/checkpoints/lenet.pth',
                        help="Path to the pre-trained DNN model (.pth file) for all DNN modes.")
    parser.add_argument('--model_def', type=str, default='example_nn/lenet/model.py',
                        help="Path to the Python file containing the model class definition (required for DNN modes).")

    return parser.parse_args()

def parse_rep_conf(rep_conf_input):
    """Parse rep conf from space-separated integers."""
    if isinstance(rep_conf_input, list):
        if len(rep_conf_input) == 0:
            return [8, 8]
        if all(isinstance(x, int) for x in rep_conf_input):
            return rep_conf_input
        try:
            return [int(x) for x in rep_conf_input]
        except ValueError as e:
            raise ValueError(f"All rep_conf values must be integers. Error: {e}")
    else:
        raise ValueError(f"Unsupported rep_conf input type: {type(rep_conf_input)}")

def generate_output_filename(model_path, mem_model, args):
    """Generate output filename for faulty model."""
    original_filename = os.path.basename(model_path)
    base_name, ext = os.path.splitext(original_filename)
    
    float_types = ['float16', 'bfloat16', 'float32', 'float64']
    filename_parts = [base_name, mem_model, f"s{args.seed}", f"q{args.q_type}"]
    # Add quantization bits only for fixed-point types
    if args.q_type not in float_types:
        filename_parts.append(f"i{args.int_bits}")
        filename_parts.append(f"f{args.frac_bits}")

    if 'dram' in mem_model:
        filename_parts.append(f"rt{args.refresh_t}")
        if args.vdd is not None:
            filename_parts.append(f"vdd{args.vdd}")
            
    filename = "_".join(filename_parts) + ext
    
    return os.path.join(os.path.dirname(model_path), filename)

def main():
    args = parse_args()
    
    try:
        rep_conf_list = parse_rep_conf(args.rep_conf)
    except ValueError as e:
        print(f"Error: {e}")
        return
        
    import msxFI.fi_config as fi_config
    from msxFI.fi_utils import validate_config

    if args.mode not in fi_config.mem_dict:
        print(f"Error: Unknown memory model '{args.mode}'")
        print(f"Available models: {list(fi_config.mem_dict.keys())}")
        return

    if not validate_config(args, rep_conf_list):
        return

    fi_config.mem_model = args.mode
    print(f"Set memory model to: {fi_config.mem_model}")
    from msxFI import fault_injection

    if args.seed is None:
        args.seed = random.randint(0, 2**10)
        print(f"No seed provided, using randomly generated seed: {args.seed}")
    else:
        print(f"Using user-provided seed: {args.seed}")

    test_size = (args.matrix_size, args.matrix_size)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    base_params = {
        'seed': args.seed,
        'int_bits': args.int_bits,
        'frac_bits': args.frac_bits,
        'q_type': args.q_type
    }
    
    if 'dram' in args.mode:
        if args.refresh_t is None:
            raise ValueError("refresh_t is required for DRAM models")
        base_params['refresh_t'] = args.refresh_t * 1e-6
        base_params['vth_sigma'] = args.vth_sigma / 1000.0  # convert mV to V
        if args.vdd is not None:
            base_params['custom_vdd'] = args.vdd
            param_info = f"refresh_t={args.refresh_t}us, vth_sigma={args.vth_sigma}mV, vdd={args.vdd}V"
        else:
            param_info = f"refresh_t={args.refresh_t}us, vth_sigma={args.vth_sigma}mV"
    else:
        base_params['rep_conf'] = np.array(rep_conf_list)
        base_params['encode'] = 'dense'
        param_info = f"rep_conf={rep_conf_list}"

    if args.eval_dnn:
        print(f"Running {args.mode.upper()} DNN Fault Injection\n\n")
        base_params.update({
            'model_def_path': args.model_def,
            'model_path': args.model
        })
        print(f"Injecting {args.mode.upper()} faults into DNN model with seed {args.seed}, {param_info}...")
        
        try:
            faulty_model = fault_injection.dnn_fi(**base_params)
            print(f"{args.mode.upper()} DNN fault injection complete.")
        except Exception as e:
            print(f"Error during {args.mode.upper()} DNN fault injection: {e}")
            return

        save_path = generate_output_filename(args.model, args.mode, args)
        torch.save(faulty_model, save_path)
        print(f"Faulty {args.mode.upper()} DNN model saved to {save_path}")

    else:
        print(f"\nTest for {args.mode.upper()} single matrix fault generation\n")
        np.random.seed(args.seed)
        test_matrix = np.random.uniform(-1, 1, size=test_size)
        print(f"Original {args.mode.upper()} Matrix (sample):")
        sample_rows_orig = min(5, test_matrix.shape[0])
        sample_cols_orig = min(5, test_matrix.shape[1])
        print(test_matrix[:sample_rows_orig, :sample_cols_orig])
        
        faulty_matrix = fault_injection.mat_fi(test_matrix.copy(), **base_params)
        print(f"Matrix after {args.mode.upper()} injection with seed {args.seed}, {param_info}:")
        print("Faulty matrix sample:")
        sample_rows = min(5, faulty_matrix.shape[0])
        sample_cols = min(5, faulty_matrix.shape[1])
        print(faulty_matrix[:sample_rows, :sample_cols])
        
        diff_matrix = test_matrix - faulty_matrix
        print("Difference (faults):")
        print(diff_matrix[:sample_rows, :sample_cols])

if __name__=="__main__":
    main()
