import os
import re
import pickle
import math

TEMPERATURES = [300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400]
DEFAULT_ACCESS_CMOS_WIDTH_1T = 1.0
DEFAULT_ACCESS_CMOS_WIDTH_3T = 1.0
DEFAULT_CELL_CAP_1T = 18e-15
DEFAULT_CELL_CAP_3T = 5e-15
DEFAULT_RETENTION_TIME_1T_US = 100.0
DEFAULT_RETENTION_TIME_3T_US = 100.0
TECHNOLOGY_CPP_FILEPATH = "../../ArrayCharacterization/Technology.cpp"
CELL_FILES_DIRECTORY = "../../ArrayCharacterization/sample_cells/"
OUTPUT_DIRECTORY = "../mem_data/"

CELL_PARAMETER_CONFIG = {
    'AccessCMOSWidth': {
        'pattern': r'-AccessCMOSWidth \(F\)\s*:\s*([\d.]+)',
        'internal_key': 'access_width',
        'type': float
    },
    'DRAMCellCapacitance': {
        'pattern': r'-DRAMCellCapacitance \(F\)\s*:\s*([\d.eE+-]+)',
        'internal_key': 'cell_capacitance',
        'type': float
    },
    'RetentionTime': {
        'pattern': r'-RetentionTime \(us\)\s*:\s*([\d.eE+-]+)',
        'internal_key': 'retention_time',
        'type': float
    }
}

TECH_PARAMS_IOFF_CALC = {
    14: {'eff_w': 4.2e-8*2+8.0e-9, 'max_s': None, 'max_f': None},
    10: {'eff_w': 4.5e-8*2+8.0e-9, 'max_s': None, 'max_f': None},
    7: {'eff_w': 5.0e-8*2+7e-9, 'max_s': None, 'max_f': None},
    5: {'eff_w': 106.0*1e-9, 'max_s': None, 'max_f': None},
    3: {'eff_w': 101.0*1e-9, 'max_s': None, 'max_f': None},
    2: {'eff_w': (6e-9+15e-9)*2, 'max_s': 3, 'max_f': 1},
    1: {'eff_w': (6e-9+10e-9)*2, 'max_s': 4, 'max_f': 1}
}

def parse_technology_cpp(filepath):
    """
    Parse the Technology.cpp file to extract the technology data.
    """
    tech_data = {}
    try:
        with open(filepath, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: Technology file not found at {filepath}")
        return tech_data

    all_feature_conditions = []
    
    pattern1 = r'(?:if|else if) \(_featureSizeInNano >= (\d+)\)\s*\{'
    matches1 = re.finditer(pattern1, content)
    for match in matches1:
        feature_size = int(match.group(1))
        start_pos = match.start()
        all_feature_conditions.append((feature_size, start_pos))
    
    pattern2 = r'(?:if|else if) \(featureSizeInNano >= (\d+)\)\s*\{'
    matches2 = re.finditer(pattern2, content)
    for match in matches2:
        feature_size = int(match.group(1))
        start_pos = match.start()
        all_feature_conditions.append((feature_size, start_pos))
    
    all_feature_conditions.sort(key=lambda x: x[1])
    
    for i, (feature_size, start_pos) in enumerate(all_feature_conditions):
        if i + 1 < len(all_feature_conditions):
            end_pos = all_feature_conditions[i + 1][1]
            block_content = content[start_pos:end_pos]
        else:
            end_markers = [
                content.find('initialized = true;', start_pos),
                content.find('void Technology::PrintProperty', start_pos),
                content.find('void Technology::', start_pos + 100)
            ]
            end_pos = min([pos for pos in end_markers if pos != -1] + [len(content)])
            block_content = content[start_pos:end_pos]
        
        lop_match = re.search(r'(?:if|else if) \(_deviceRoadmap == LOP\)\s*\{([\s\S]*?)(?=\n\s*\}[^}]*(?:else|$|\n\s*\}))', block_content)
        
        if lop_match:
            lop_content = lop_match.group(1)
            ioff_values = [0.0] * 11
            all_ioff_found = True
            
            for temp_idx in range(11):
                temp_idx_val = temp_idx * 10
                ioff_re = re.search(rf"currentOffNmos\[{temp_idx_val}\]\s*=\s*([\d.eE+-]+)\s*;", lop_content)
                if ioff_re:
                    ioff_values[temp_idx] = float(ioff_re.group(1))
                else:
                    all_ioff_found = False
                    break
            
            vdd_re = re.search(r"vdd\s*=\s*([\d.eE+-]+)\s*;", lop_content)
            vdd_value = float(vdd_re.group(1)) if vdd_re else None
            
            if all_ioff_found and vdd_value is not None:
                tech_data[feature_size] = {
                    'ioff_values': ioff_values,
                    'vdd': vdd_value
                }
            else:
                missing_fields = []
                if not all_ioff_found:
                    missing_fields.append("Ioff values")
                if vdd_value is None:
                    missing_fields.append("vdd")
                print(f"Warning: Missing {', '.join(missing_fields)} for {feature_size}nm LOP block")
        else:
            print(f"Warning: No LOP block found for {feature_size}nm")
    
    return tech_data

def extract_parameter_from_cell(content, pattern, data_type):
    """Extract a single parameter value from cell file content using regex pattern."""
    match = re.search(pattern, content)
    return data_type(match.group(1)) if match else None

def parse_cell_files(cell_directory_path, technology_nodes):
    """
    Parse the cell files to extract the cell parameters.
    """
    technology_node_mapping = {} 
    if not os.path.isdir(cell_directory_path):
        print(f"Error: Cell directory not found at {cell_directory_path}")
        return {}

    sorted_tech_nodes = sorted(list(technology_nodes))

    for filename in sorted(os.listdir(cell_directory_path)):
        if filename.endswith(".cell"):
            node_match = re.search(r"_(\d+)nm\.cell", filename)
            if node_match:
                cell_node_size = int(node_match.group(1))
                try:
                    with open(os.path.join(cell_directory_path, filename), 'r') as f:
                        content = f.read()
                    extracted_parameters = {}
                    for parameter_name, config in CELL_PARAMETER_CONFIG.items():
                        extracted_parameters[config['internal_key']] = extract_parameter_from_cell(
                            content, config['pattern'], config['type']
                        )
                except FileNotFoundError:
                    print(f"Warning: Cell file {filename} disappeared during read.")
                    continue
                if extracted_parameters['access_width'] is not None and extracted_parameters['cell_capacitance'] is not None:
                    matching_tech_node = -1
                    for tech_node in reversed(sorted_tech_nodes):
                        if cell_node_size >= tech_node:
                            matching_tech_node = tech_node
                            break
                    
                    if matching_tech_node != -1:
                        node_size_distance = abs(cell_node_size - matching_tech_node)
                        cell_candidate = {**extracted_parameters, 'source_cell_node': cell_node_size, 'distance': node_size_distance}
                        
                        existing_candidate = technology_node_mapping.get(matching_tech_node)
                        if existing_candidate is None or node_size_distance < existing_candidate['distance']:
                            technology_node_mapping[matching_tech_node] = cell_candidate
    
    final_parameters = {}
    for tech_node, cell_data in technology_node_mapping.items():
        final_parameters[tech_node] = {
            config['internal_key']: cell_data.get(config['internal_key'])
            for _, config in CELL_PARAMETER_CONFIG.items()
        }
    return final_parameters

def calculate_ioff(mean_ioff, access_cmos_width, feature_size_nm, tech_params_dict):
    """
    Calculate the Ioff value for a given feature size and technology node.
    """
    feature_size_m = feature_size_nm * 1e-9
    width_factor = access_cmos_width 
    actual_width = 0.0
    node_params = tech_params_dict.get(feature_size_nm)

    if feature_size_nm >= 22:
        actual_width = width_factor * feature_size_m
    elif 3 <= feature_size_nm < 22:
        if node_params and 'eff_w' in node_params and node_params['eff_w'] is not None:
            effective_width = node_params['eff_w']
            actual_width = math.ceil(width_factor) * effective_width
        else:
            print(f"Warning: Missing or None eff_w for {feature_size_nm}nm in TECH_PARAMS_IOFF_CALC. Using actual_width=0.")
    elif feature_size_nm < 3:
        if node_params and 'eff_w' in node_params and node_params['eff_w'] is not None and \
           'max_s' in node_params and node_params['max_s'] is not None and \
           'max_f' in node_params and node_params['max_f'] is not None:
            effective_width = node_params['eff_w']
            max_sheet = node_params['max_s']
            max_fin = node_params['max_f']
            if max_fin == 0:
                print(f"Warning: max_f is zero for {feature_size_nm}nm in TECH_PARAMS_IOFF_CALC. Using actual_width=0.")
            else:
                actual_width = math.ceil(width_factor) * effective_width * max_sheet / max_fin
        else:
            print(f"Warning: Missing or None GAA params for {feature_size_nm}nm in TECH_PARAMS_IOFF_CALC. Using actual_width=0.")
            
    return mean_ioff * actual_width

def process_dram_type(dram_type_str, tech_data, technology_nodes, cell_files_dir_path, default_access_width, default_cell_capacitance, default_retention_time, output_filename):
    """
    Process DRAM type data and save to pickle file.
    
    Args:
        dram_type_str: DRAM type identifier ("1T" or "3T")
        tech_data: Dictionary of {node: {'ioff_values': [values], 'vdd': value}}
        technology_nodes: List of technology nodes from cpp file
        cell_files_dir_path: Path to cell files directory
        default_access_width: Default AccessCMOSWidth
        default_cell_capacitance: Default cell capacitance
        default_retention_time: Default retention time in microseconds
        output_filename: Output pickle filename
    """
    final_data_payload = {}
    parsed_cell_parameters = parse_cell_files(cell_files_dir_path, technology_nodes)
    print(f"\nProcessing {dram_type_str} DRAM with {len(tech_data)} technology nodes")

    for feature_size_nm, tech_node_data in tech_data.items():
        cell_parameters = parsed_cell_parameters.get(feature_size_nm)
        
        current_access_width = default_access_width
        if cell_parameters and cell_parameters.get('access_width') is not None:
            current_access_width = cell_parameters['access_width']

        current_cell_capacitance = default_cell_capacitance
        if cell_parameters and cell_parameters.get('cell_capacitance') is not None:
            current_cell_capacitance = cell_parameters['cell_capacitance']

        current_retention_time = default_retention_time
        if cell_parameters and cell_parameters.get('retention_time') is not None:
            current_retention_time = cell_parameters['retention_time']
        
        mean_ioff_list = tech_node_data['ioff_values']
        vdd_value = tech_node_data['vdd']
        
        ioff_at_temperatures = {}
        for temp_idx, temp_kelvin in enumerate(TEMPERATURES):
            if temp_idx < len(mean_ioff_list):
                mean_ioff_value = mean_ioff_list[temp_idx]
                calculated_ioff_value = calculate_ioff(mean_ioff_value, current_access_width, feature_size_nm, TECH_PARAMS_IOFF_CALC)
                ioff_at_temperatures[temp_kelvin] = calculated_ioff_value
            else:
                ioff_at_temperatures[temp_kelvin] = 0.0
        
        final_data_payload[feature_size_nm] = {
            'CellCap': current_cell_capacitance,
            'Ioff': ioff_at_temperatures,
            'vdd': vdd_value,
            'RetentionTime': current_retention_time
        }
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(script_dir, OUTPUT_DIRECTORY)
    output_filepath = os.path.join(output_dir, output_filename)
    
    try:
        os.makedirs(output_dir, exist_ok=True)
        with open(output_filepath, 'wb') as f_out:
            pickle.dump(final_data_payload, f_out)
        
        with open(output_filepath, 'rb') as f_verify:
            saved_data = pickle.load(f_verify)
            print(f"Successfully saved {len(saved_data)} nodes to {output_filepath}")
            
    except Exception as e:
        print(f"Error saving data to {output_filepath}: {e}")
        raise

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    tech_cpp_abs_path = os.path.join(script_dir, TECHNOLOGY_CPP_FILEPATH)
    cell_dir_abs_path = os.path.join(script_dir, CELL_FILES_DIRECTORY)

    if not os.path.exists(tech_cpp_abs_path):
        print(f"Error: {TECHNOLOGY_CPP_FILEPATH} (resolved to {tech_cpp_abs_path}) not found.")
        return
    if not os.path.isdir(cell_dir_abs_path):
        print(f"Error: {CELL_FILES_DIRECTORY} (resolved to {cell_dir_abs_path}) not found.")
        return

    tech_data_map = parse_technology_cpp(tech_cpp_abs_path)
    if not tech_data_map:
        print("Error: Failed to parse Technology.cpp or no LOP data found.")
        return
        
    available_technology_nodes = list(tech_data_map.keys())

    process_dram_type(
        dram_type_str="1T",
        tech_data=tech_data_map,
        technology_nodes=available_technology_nodes,
        cell_files_dir_path=os.path.join(cell_dir_abs_path, "sample_edram1ts/"),
        default_access_width=DEFAULT_ACCESS_CMOS_WIDTH_1T,
        default_cell_capacitance=DEFAULT_CELL_CAP_1T,
        default_retention_time=DEFAULT_RETENTION_TIME_1T_US,
        output_filename="dram1t_args.p"
    )

    process_dram_type(
        dram_type_str="3T",
        tech_data=tech_data_map,
        technology_nodes=available_technology_nodes,
        cell_files_dir_path=os.path.join(cell_dir_abs_path, "sample_edram3ts/"),
        default_access_width=DEFAULT_ACCESS_CMOS_WIDTH_3T,
        default_cell_capacitance=DEFAULT_CELL_CAP_3T,
        default_retention_time=DEFAULT_RETENTION_TIME_3T_US,
        output_filename="dram3t_args.p"
    )

if __name__ == "__main__":
    main()
