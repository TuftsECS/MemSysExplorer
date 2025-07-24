import os
import subprocess
import sys
import shlex
import torch
import torch.nn as nn
from model import LeNet
from train import get_mnist_dataloaders

def evaluate_model(model, dataloader, device):
    model.eval()
    correct = 0
    total = 0
    with torch.no_grad():
        for images, labels in dataloader:
            images, labels = images.to(device), labels.to(device)
            outputs = model(images)
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()
    accuracy = 100 * correct / total
    return accuracy

def main():
    # --- Configuration ---
    MODE = "rram_mlc"  # Specific memory model: "rram_mlc", "fefet_200d", "dram3t", "dram1t", etc.
    SEED = 0
    INT_BITS = 2
    FRAC_BITS = 4
    Q_TYPE = "afloat"
    REP_CONF = [8, 8]

    MODEL_PATH = "msxFI/example_nn/lenet/checkpoints/lenet.pth"
    MODEL_DIR = "msxFI/example_nn/lenet"  # Directory containing model.py

    # DRAM specific parameters (only used for DRAM models)
    REFRESH_T = 70  # in microseconds
    SIGMA = 0.05

    SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
    WORKSPACE_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../.."))

    model_basename, model_ext = os.path.splitext(os.path.basename(MODEL_PATH))
    model_dir_path = os.path.dirname(MODEL_PATH)

    # --- Step 1: Generate Faulty Model ---
    print("--- Step 1: Generating Faulty Model ---")

    # Generate filename based on memory model type
    if 'dram' in MODE:
        if REFRESH_T is None:
            print("Error: For DRAM modes, refresh_t must be set.", file=sys.stderr)
            sys.exit(1)
        faulty_model_name = f"{model_basename}_{MODE}_s{SEED}_q{Q_TYPE}_i{INT_BITS}_f{FRAC_BITS}_rt{float(REFRESH_T)}{model_ext}"
    else:
        faulty_model_name = f"{model_basename}_{MODE}_s{SEED}_q{Q_TYPE}_i{INT_BITS}_f{FRAC_BITS}{model_ext}"

    faulty_model_rel_path = os.path.join(model_dir_path, faulty_model_name)
    faulty_model_path = os.path.join(WORKSPACE_ROOT, faulty_model_rel_path)

    msxfi_script_path = os.path.join(WORKSPACE_ROOT, "msxFI", "run_msxfi.py")

    print(f"Changing directory to WORKSPACE_ROOT: {WORKSPACE_ROOT}")

    print("Running fault injection script (run_msxfi.py)...")
    
    cmd_list = [
        sys.executable,
        msxfi_script_path,
        "--mode", MODE,
        "--eval_dnn",
        "--model", MODEL_PATH,
        "--model_def", os.path.join(MODEL_DIR, "model.py"),
        "--seed", str(SEED),
        "--int_bits", str(INT_BITS),
        "--frac_bits", str(FRAC_BITS),
        "--q_type", Q_TYPE
    ]

    # Add rep_conf for NVM models (not for DRAM models)
    if 'dram' not in MODE:
        cmd_list.extend(["--rep_conf"] + [str(x) for x in REP_CONF])

    if 'dram' in MODE:
        cmd_list.extend([
            "--refresh_t", str(REFRESH_T),
            "--vth_sigma", str(SIGMA)
        ])

    current_env = os.environ.copy()
    model_module_dir = os.path.join(WORKSPACE_ROOT, MODEL_DIR)
    
    existing_pythonpath = current_env.get("PYTHONPATH", "")
    if existing_pythonpath:
        current_env["PYTHONPATH"] = f"{model_module_dir}{os.pathsep}{existing_pythonpath}"
    else:
        current_env["PYTHONPATH"] = model_module_dir

    print(f"Executing: {shlex.join(cmd_list)}")
    subprocess.run(cmd_list, cwd=WORKSPACE_ROOT, env=current_env, check=True, text=True)

    if not os.path.exists(faulty_model_path):
        print(f"Error: Faulty model was not created at {faulty_model_path}", file=sys.stderr)
        sys.exit(1)
    
    print("")

    # --- Step 2: Evaluating Models (Integrated) ---
    print("--- Step 2: Evaluating Models ---")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Using device: {device}")

    data_root = os.path.join(SCRIPT_DIR, "data")
    print(f"Loading MNIST dataset from: {data_root}")
    
    if not os.path.isdir(data_root):
        print(f"Warning: Data directory {data_root} not found. MNIST download might be attempted by dataloader.")

    _, test_loader = get_mnist_dataloaders(batch_size=1000, train_root=data_root, download=True)

    # Evaluate original model
    print("\nEvaluating original model...")
    original_model_path = os.path.join(WORKSPACE_ROOT, MODEL_PATH)
    
    original_model = torch.load(original_model_path, map_location=device, weights_only=False)
    original_model = original_model.to(device)
    
    original_accuracy = evaluate_model(original_model, test_loader, device)
    print(f"Accuracy of the original model: {original_accuracy:.2f}%")

    # Evaluate faulty model
    print("\nEvaluating faulty model...")
    faulty_model = torch.load(faulty_model_path, map_location=device, weights_only=False)
    faulty_model = faulty_model.to(device)
    
    faulty_accuracy = evaluate_model(faulty_model, test_loader, device)
    print(f"Accuracy of the faulty model: {faulty_accuracy:.2f}%")

    print("")
    print("--- Script Finished ---")

if __name__ == "__main__":
    main() 