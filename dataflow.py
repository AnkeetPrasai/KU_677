import sys
import re
from collections import defaultdict

class BasicBlock:
    def __init__(self, label):
        self.label = label
        self.edges = []
        self.instructions = []

def get_origin(var, data_flow):
    # Recursively find the origin of a variable in the data flow
    if var in data_flow:
        if data_flow[var] == var:
            return var
        origin = get_origin(data_flow[var], data_flow)
        return origin
    return var

def main(input_file):
    data_flow = {}          # Dictionary to track the data flow of variables
    blocks = []             # List to store basic blocks
    block_map = {}          # Map to store block labels and their indices
    block_counter = 0       # Counter for the number of blocks
    has_flow = False        # Flag to indicate if sensitive data flow is detected
    current_func_name = ""  # Name of the current function being processed
    current_block = ""      # Label of the current block being processed
    true_block = ""         # Label of the true branch block
    in_true_block = False   # Flag to indicate if currently in the true branch block
    false_block = ""        # Label of the false branch block
    in_false_block = False  # Flag to indicate if currently in the false branch block

    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("define"):
                # Start of a new function definition
                blocks.clear()
                block_map.clear()
                block_counter = 0
                func_name_search = re.search(r'@(\w+)', line)
                if func_name_search:
                    func_name = func_name_search.group(1)
                    current_func_name = func_name
                    current_block = current_func_name
                    block_map[current_block] = block_counter
                    blocks.append(BasicBlock(current_block))
                    block_counter += 1
            elif line.endswith(":"):
                # Start of a new basic block
                in_true_block = False
                in_false_block = False
                current_block = line[:-1]
                if (current_block == true_block):
                    in_true_block = True
                if (current_block == false_block):
                    in_false_block = True
                block_map[current_block] = block_counter
                blocks.append(BasicBlock(current_block))
                block_counter += 1
            elif "store" in line:
                # Handle store instructions
                tokens = line.split()
                if len(tokens) >= 5:
                    src = tokens[2].rstrip(',')  # Source variable
                    dest = tokens[4]  # Destination variable
                    origin = get_origin(src, data_flow)
                    if(in_true_block or in_false_block):
                        # If in a conditional block, update data flow conditionally
                        destOrigin = get_origin(dest, data_flow)
                        if(destOrigin != "SOURCE"):
                            data_flow[dest] = origin
                    else:
                        # Otherwise, update data flow directly
                        data_flow[dest] = origin
            elif "load" in line:
                # Handle load instructions
                match = re.match(r'(%\w+)\s*=\s*load\s+\w+, ptr\s+(%\w+)', line)
                if match:
                    dest, src = match.groups()
                    origin = get_origin(src, data_flow)
                    data_flow[dest] = origin
            elif "alloca" in line:
                # Handle alloca instructions
                match = re.match(r'(%\w+)\s*=\s*alloca\s+(\w+)', line)
                if match:
                    var_name, var_type = match.groups()
                    data_flow[var_name] = var_name  # Initialize with itself
            elif any(op in line for op in ["add", "sub", "mul", "div"]):
                # Handle arithmetic instructions
                match = re.match(r'(%\w+)\s*=\s*(add|sub|mul|div)\s+\w+\s+([\w%]+),\s*([\w%]+)', line)
                if match:
                    dest, operation, operand1, operand2 = match.groups()
                    origin1 = get_origin(operand1, data_flow) if operand1.startswith('%') else operand1
                    origin2 = get_origin(operand2, data_flow) if operand2.startswith('%') else operand2
                    if origin1 == "SOURCE" or origin2 == "SOURCE":
                        data_flow[dest] = "SOURCE"
                    else:
                        if operand1.startswith('%'):
                            data_flow[dest] = origin1
                        else:
                            data_flow[dest] = operand1
            elif "call" in line:
                # Handle call instructions
                match = re.match(r'(%\w+)\s*=\s*call\s+\w+\s+\([^)]*\)\s+@(\w+)\((.*)\)', line)
                if match:
                    dest, called_func, args = match.groups()
                    args = re.findall(r'%\w+', args)
                    argument = args[0] if args else ""
                    if called_func == "SOURCE":
                        data_flow[dest] = "SOURCE"
                    elif called_func == "SINK":
                        origin = get_origin(argument, data_flow)
                        if origin == "SOURCE":
                            has_flow = True
                else:
                    match_no_assign = re.match(r'call\s+\w+\s+@(\w+)\((.*)\)', line)
                    if match_no_assign:
                        called_func, args = match_no_assign.groups()
                        args = re.findall(r'%\w+', args)
                        argument = args[0] if args else ""
                        if called_func == "SINK":
                            origin = get_origin(argument, data_flow)
                            if origin == "SOURCE":
                                has_flow = True
            elif "phi" in line:
                # Handle phi instructions
                match = re.match(r'(%\w+)\s*=\s*phi\s+\w+\s+\[(%\w+|\d+),\s*%(\w+)\](?:,\s+\[(%\w+|\d+),\s*%(\w+)\])?', line)
                if match:
                    dest = match.group(1)
                    sources = []
                    if match.group(2):
                        sources.append(match.group(2))
                    if match.group(4):
                        sources.append(match.group(4))
                    constants = [src for src in sources if not src.startswith('%')]
                    origins = [get_origin(src, data_flow) for src in sources if src.startswith('%')]
                    origins.append(constants)
                    if "SOURCE" in origins:
                        data_flow[dest] = "SOURCE"
                    else:
                        data_flow[dest] = origins[0] if origins else dest
            elif "br" in line and "br label" not in line:
                # Handle branch instructions
                match = re.match(r'br\s+\w+\s+(%\w+),\s+label\s+(%\w+),\s+label\s+(%\w+)', line)
                if match:
                    condition, label_true, label_false = match.groups()
                    true_block = label_true[1:]
                    false_block = label_false[1:]

    if has_flow:
        print("Flow.")
    else:
        print("No Flow.")

if __name__ == "__main__":
        main(sys.argv[1])