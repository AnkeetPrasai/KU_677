import sys
import re
from collections import defaultdict

class BasicBlock:
    def __init__(self, label):
        self.label = label
        self.edges = []
        self.instructions = []

def get_origin(var, data_flow):
    if var in data_flow:
        if data_flow[var] == var:
            return var
        origin = get_origin(data_flow[var], data_flow)
        return origin
    return var

def main(input_file):
    data_flow = {}
    blocks = []
    block_map = {}
    block_counter = 0
    has_flow = False
    current_func_name = ""
    current_block = ""
    true_block = ""
    in_true_block = False
    false_block = ""
    in_false_block = False

    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith("define"):
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
                in_true_block = False
                in_false_block = False
                current_block = line[:-1]
                print("Current Block: ",current_block, "True Block: ", true_block, "False Block: ", false_block)
                if (current_block == true_block):
                    in_true_block = True
                if (current_block == false_block):
                    in_false_block = True
                block_map[current_block] = block_counter
                blocks.append(BasicBlock(current_block))
                block_counter += 1
            elif "store" in line:
                tokens = line.split()
                if len(tokens) >= 5:
                    src = tokens[2].rstrip(',')  # e.g., %secret
                    dest = tokens[4]             # e.g., %aVar
                    print(f"Detected store instruction: {line}")
                    print(f"Source: {src}, Destination: {dest}")
                    origin = get_origin(src, data_flow)
                    if(in_true_block or in_false_block):
                        print("IN TRUE OR FALSE BLOCK", dest, data_flow)
                        destOrigin = get_origin(dest, data_flow)
                        print(f"Destination origin: {destOrigin}")
                        if(destOrigin == "SOURCE"):
                            print(f"Data flow unchanged: {dest} <- {origin}")
                        else:
                            data_flow[dest] = origin
                            print(f"Data flow updated: {dest} <- {origin}")
                    else:
                        origin = get_origin(src, data_flow)
                        data_flow[dest] = origin
                        print(f"Data flow updated: {dest} <- {origin}")
            elif "load" in line:
                # Example: %a1 = load i32, ptr %aVar
                match = re.match(r'(%\w+)\s*=\s*load\s+\w+, ptr\s+(%\w+)', line)
                if match:
                    dest, src = match.groups()
                    print(f"Detected load instruction: {line}")
                    print(f"Source: {src}, Destination: {dest}")
                    origin = get_origin(src, data_flow)
                    data_flow[dest] = origin
                    print(f"Data flow updated: {dest} <- {origin}")
            elif "alloca" in line:
                # Example: %aVar = alloca i32
                match = re.match(r'(%\w+)\s*=\s*alloca\s+(\w+)', line)
                if match:
                    var_name, var_type = match.groups()
                    print(f"Detected alloca instruction: {line}")
                    print(f"Variable: {var_name}, Type: {var_type}")
                    data_flow[var_name] = var_name  # Initialize with itself
            elif any(op in line for op in ["add", "sub", "mul", "div"]):
                # Example: %varT = add i32 1, 0
                match = re.match(r'(%\w+)\s*=\s*(add|sub|mul|div)\s+\w+\s+([\w%]+),\s*([\w%]+)', line)
                if match:
                    dest, operation, operand1, operand2 = match.groups()
                    print(f"Detected arithmetic instruction: {line}")
                    
                    # Determine origin based on operands
                    origin1 = get_origin(operand1, data_flow) if operand1.startswith('%') else operand1
                    origin2 = get_origin(operand2, data_flow) if operand2.startswith('%') else operand2
                    
                    # Decide the origin for dest
                    if origin1 == "SOURCE" or origin2 == "SOURCE":
                        data_flow[dest] = "SOURCE"
                        print(f"Data flow updated: {dest} <- SOURCE")
                    else:
                        # If operand1 is a constant, prefer it; else use origin1
                        if operand1.startswith('%'):
                            data_flow[dest] = origin1
                            print(f"Data flow updated: {dest} <- {origin1}")
                        else:
                            data_flow[dest] = operand1
                            print(f"Data flow updated: {dest} <- {operand1}")
            elif "call" in line:
                # Example: %secret = call i32 () @SOURCE()
                match = re.match(r'(%\w+)\s*=\s*call\s+\w+\s+\([^)]*\)\s+@(\w+)\((.*)\)', line)
                if match:
                    dest, called_func, args = match.groups()
                    args = re.findall(r'%\w+', args)
                    argument = args[0] if args else ""
                    print(f"Detected call instruction: {line}")
                    print(f"Function: {called_func}, Argument: {argument}")
                    if called_func == "SOURCE":
                        data_flow[dest] = "SOURCE"
                        print(f"Data flow updated: {dest} <- SOURCE")
                    elif called_func == "SINK":
                        origin = get_origin(argument, data_flow)
                        if origin == "SOURCE":
                            has_flow = True
                else:
                    # Handle call instructions without assignment
                    match_no_assign = re.match(r'call\s+\w+\s+@(\w+)\((.*)\)', line)
                    if match_no_assign:
                        called_func, args = match_no_assign.groups()
                        args = re.findall(r'%\w+', args)
                        argument = args[0] if args else ""
                        print(f"Detected call instruction: {line}")
                        print(f"Function: {called_func}, Argument: {argument}")
                        if called_func == "SINK":
                            origin = get_origin(argument, data_flow)
                            if origin == "SOURCE":
                                has_flow = True
            elif "phi" in line:
                # Example: %var = phi i32 [%varT, %lbl_t], [%varF, %lbl_f]
                # Example: %var = phi i32 [0, %lbl_t], [%varF, %lbl_f]
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
                    print(f"Detected phi instruction: {line}")
                    print(f"Destination: {dest}, Sources: {', '.join(sources)}")
                    if "SOURCE" in origins:
                        data_flow[dest] = "SOURCE"
                    else:
                        data_flow[dest] = origins[0] if origins else dest
                    print(f"Data flow updated: {dest} <- {data_flow[dest]}")
            elif "br" in line and "br label" not in line:
                # Example: br i1 %cmp, label %lbl_t, label %lbl_f
                match = re.match(r'br\s+\w+\s+(%\w+),\s+label\s+(%\w+),\s+label\s+(%\w+)', line)
                if match:
                    condition, label_true, label_false = match.groups()
                    print(f"Detected branch instruction: {line}")
                    print(f"Condition: {condition}, Labels: {label_true}, {label_false}")
                    true_block = label_true[1:]
                    false_block = label_false[1:]

    if has_flow:
        print("Sensitive data flow detected.")
    else:
        print("No sensitive data flow detected.")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python dataflow.py <input_file.ll>")
    else:
        main(sys.argv[1])