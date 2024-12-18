#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

struct BasicBlock
{
    std::string label;
    std::vector<std::pair<int, std::string>> edges;
    std::vector<std::string> instructions; // for testing parsing only
};

std::map<std::string, std::string> dataFlow; // record the dataflow, trace origin...

// void createDotFile(const std::string& funcName, const std::vector<BasicBlock>& blocks, const std::map<std::string, int>& blockMap);

std::string getOrigin(const std::string &var, const std::map<std::string, std::string> &dataFlow)
{
    // recursively trace the source in dataflow
    if (dataFlow.find(var) != dataFlow.end())
    {
        return getOrigin(dataFlow.at(var), dataFlow);
    }
    // If there is no source, return the variable itself directly
    return var;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./graph <input_file.ll>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::ifstream inputFileStream(inputFile);
    std::string line;
    std::string currentBlock;
    std::vector<BasicBlock> blocks;
    std::map<std::string, int> blockMap;
    int blockCounter = 0;
    bool hasEntryBlock = false;
    bool hasFlow = false;
    bool inTrueBlock = false;
    bool inFalseBlock = false;
    std::string currentFuncName; // Track the current function name

    if (!inputFileStream.is_open())
    {
        std::cerr << "Failed to open input file: " << inputFile << std::endl;
        return 1;
    }

    // std::cout << "Creating blocks" << std::endl;
    while (std::getline(inputFileStream, line))
    {
        std::istringstream iss(line);
        if (line.find("define") != std::string::npos)
        {
            // if (!blocks.empty() && !currentFuncName.empty()) {
            //     // Create a .dot file for the each function
            //     createDotFile(currentFuncName, blocks, blockMap);
            // }
            blocks.clear();
            blockMap.clear();
            blockCounter = 0;

            std::string defineKeyword, returnType, funcName;
            iss >> defineKeyword >> returnType >> funcName;
            currentFuncName = funcName.substr(1, funcName.find('(') - 1); // Get the function name
            currentBlock = currentFuncName;

            std::string nextLine;
            blockMap[currentBlock] = blockCounter++;
            blocks.push_back({currentBlock, {}, {}});
            hasEntryBlock = false;
            // This line will cause the parsing skip the first instruction
            // std::getline(inputFileStream, nextLine);

            if (nextLine.find(':') != std::string::npos)
            {
                // If next line is a label, treat it as the entry block
                currentBlock = nextLine.substr(0, nextLine.find(':'));
                blockMap[currentBlock] = blockCounter++;
                blocks.push_back({currentBlock, {}, {}});
                hasEntryBlock = true;
            }
            else
            {
                // Otherwise, assume the function name as Node0 and push the next line for processing
                blockMap[currentBlock] = blockCounter++;
                blocks.push_back({currentBlock, {}, {}});
                hasEntryBlock = false; // No "entry" block found yet
            }

            // Put back the next line for further processing if it's not a label
            if (!hasEntryBlock)
            {
                std::istringstream putBackLine(nextLine);
                line = nextLine;
            }
        }
        else if (line.find(':') != std::string::npos)
        {
            currentBlock = line.substr(0, line.find(':'));
            blockMap[currentBlock] = blockCounter++;
            blocks.push_back({currentBlock, {}, {}});
        }
        else if (line.find("phi ") != std::string::npos)
        {
            // handles phi instructions
            std::string token;
            std::string dest;
            std::string src1;
            std::string src2;
            bool destFound = false;
            while (iss >> token)
            {
                if (token.find('%') != std::string::npos && !destFound)
                {
                    dest = token;
                    destFound = true;
                }
                if (token.find('[') != std::string::npos || std::regex_match(token, std::regex("[0-9]+")))
                {
                    if (dest.empty())
                    {
                        dest = token;
                    }
                    else if (src1.empty())
                    {
                        src1 = token;
                    }
                    else
                    {
                        src2 = token;
                    }
                }
            }

            if (!src1.empty() && src1.front() == '[')
                src1 = src1.substr(src1.find('[') + 1);
            if (!src1.empty() && (src1.back() == ',' || src1.back() == ']'))
                src1.pop_back();
            if (!src1.empty() && (src1.back() == ',' || src1.back() == ']'))
                src1.pop_back();
            if (!src2.empty() && src2.front() == '[')
                src2 = src2.substr(src2.find('[') + 1);
            if (!src2.empty() && (src2.back() == ',' || src2.back() == ']'))
                src2.pop_back();
            if (!src2.empty() && (src2.back() == ',' || src2.back() == ']'))
                src2.pop_back();

            // std::cout << "Detected phi instruction: " << line << std::endl;
            // std::cout << "Destination: " << dest << ", Source1: " << src1 << ", Source2: " << src2 << std::endl;

            // Test parse
            if (!dest.empty() && !src1.empty() && !src2.empty())
            {
                std::string instruction = "phi " + dest + " <- " + src1 + ", " + src2;
                if (!currentBlock.empty() && blockMap.find(currentBlock) != blockMap.end())
                {
                    blocks[blockMap[currentBlock]].instructions.push_back(instruction);
                    // std::cout << "Parsed phi: " << instruction << std::endl;
                }
            }

            // Test dataflow
            if (!dest.empty() && !src1.empty() && !src2.empty())
            {
                std::string origin1 = getOrigin(src1, dataFlow);
                std::string origin2 = getOrigin(src2, dataFlow);

                // If any source has "SOURCE" as origin, the destination inherits it
                if (origin1 == "SOURCE" || origin2 == "SOURCE")
                {
                    dataFlow[dest] = "SOURCE";
                    // std::cout << "Data flow updated: " << dest << " <- SOURCE" << std::endl;
                }
                else
                {
                    // If no SOURCE involved, mark origin be src1
                    dataFlow[dest] = src1;
                    // std::cout << "Data flow updated: " << dest << " <- " << origin1 << std::endl;
                }
            }
        }
        else if (line.find("store") != std::string::npos)
        {
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            size_t typePos = line.find("i32");
            size_t commaPos = line.find(',');
            size_t ptrPos = line.find("ptr", commaPos);

            std::string src, dest;

            if (typePos != std::string::npos && commaPos != std::string::npos && ptrPos != std::string::npos)
            {
                src = line.substr(typePos + 4, commaPos - typePos - 4);
                src.erase(0, src.find_first_not_of(" \t"));
                if (!src.empty() && src.back() == ',')
                    src.pop_back();

                dest = line.substr(ptrPos + 4);
                dest.erase(0, dest.find_first_not_of(" \t"));
            }

            // std::cout << "Detected store instruction: " << line << std::endl;
            // std::cout << "Source: " << src << ", Destination: " << dest << "\n";

            // Test parse
            if (!src.empty() && !dest.empty())
            {
                std::string instruction = "store " + src + " -> " + dest;
                if (!currentBlock.empty() && blockMap.find(currentBlock) != blockMap.end())
                {
                    blocks[blockMap[currentBlock]].instructions.push_back(instruction);
                    // std::cout << "Parsed store: " << instruction << std::endl;
                }
            }

            // Test dataflow
            if (!src.empty() && !dest.empty())
            {
                std::string origin = getOrigin(src, dataFlow);
                // std::cout << "Origin: " << origin << " Dest: " << dest << std::endl;
                if (inTrueBlock || inFalseBlock)
                {
                    // std::cout << "In true or false block" << std::endl;
                    std::string destOrigin = getOrigin(dest, dataFlow);
                    // std::cout << "Dest Origin: " << destOrigin << std::endl;
                    if (destOrigin == "SOURCE")
                    {
                        // std::cout << "Data flow not updated." << std::endl;
                    }
                    else
                    {
                        dataFlow[dest] = origin;
                        // std::cout << "Data flow updated: " << dest << " <- " << origin << std::endl;
                    }
                }
                else
                {
                    std::string origin = getOrigin(src, dataFlow);
                    dataFlow[dest] = origin;
                    // std::cout << "Data flow updated: " << dest << " <- " << origin << std::endl;
                }
            }
        }
        else if (line.find("load") != std::string::npos)
        {
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            size_t typePos = line.find("i32");
            size_t ptrPos = line.find("ptr");
            size_t eqPos = line.find('=');

            std::string src, dest;

            if (typePos != std::string::npos && ptrPos != std::string::npos && eqPos != std::string::npos)
            {
                dest = line.substr(0, eqPos);
                dest.erase(0, dest.find_first_not_of(" \t"));
                dest.erase(dest.find_last_not_of(" \t") + 1);

                src = line.substr(ptrPos + 4);
                src.erase(0, src.find_first_not_of(" \t"));
            }

            // std::cout << "Detected load instruction: " << line << std::endl;
            // std::cout << "Source: " << src << ", Destination: " << dest << "\n";

            // Test parse
            if (!src.empty() && !dest.empty())
            {
                std::string instruction = "load " + src + " -> " + dest;
                if (!currentBlock.empty() && blockMap.find(currentBlock) != blockMap.end())
                {
                    blocks[blockMap[currentBlock]].instructions.push_back(instruction);
                    // std::cout << "Parsed load: " << instruction << std::endl;
                }
            }

            // Test dataflow
            if (!src.empty() && !dest.empty())
            {
                std::string origin = getOrigin(src, dataFlow);
                if (inTrueBlock || inFalseBlock)
                {
                    std::string destOrigin = getOrigin(dest, dataFlow);
                    if (destOrigin == "SOURCE")
                    {
                        // std::cout << "Data flow not updated." << std::endl;
                    }
                    else
                    {
                        dataFlow[dest] = origin;
                        // std::cout << "Data flow updated: " << dest << " <- " << origin << std::endl;
                    }
                }
                else
                {
                    dataFlow[dest] = origin;
                    // std::cout << "Data flow updated: " << dest << " <- " << origin << std::endl;
                }
            }
        }
        else if (line.find("alloca") != std::string::npos)
        {
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            size_t equalPos = line.find('=');
            size_t allocaPos = line.find("alloca", equalPos);

            std::string varName, type;

            if (equalPos != std::string::npos && allocaPos != std::string::npos)
            {
                varName = line.substr(0, equalPos);
                varName.erase(varName.find_last_not_of(" \t") + 1);
                varName.erase(0, varName.find_first_not_of(" \t"));

                type = line.substr(allocaPos + 7);
                type.erase(0, type.find_first_not_of(" \t"));
            }

            // std::cout << "Detected alloca instruction: " << line << std::endl;
            // std::cout << "Variable: " << varName << ", Type: " << type << std::endl;

            if (!varName.empty() && !type.empty())
            {
                std::string instruction = "alloca " + varName + " (" + type + ")";
                if (!currentBlock.empty() && blockMap.find(currentBlock) != blockMap.end())
                {
                    blocks[blockMap[currentBlock]].instructions.push_back(instruction);
                    // std::cout << "Parsed alloca: " << instruction << std::endl;
                }
            }
        }
        else if (line.find("add") != std::string::npos ||
                 line.find("sub") != std::string::npos ||
                 line.find("div") != std::string::npos ||
                 line.find("mul") != std::string::npos)
        {

            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            size_t eqPos = line.find('=');
            size_t opPos = line.find("add");
            if (opPos == std::string::npos)
                opPos = line.find("sub");
            if (opPos == std::string::npos)
                opPos = line.find("div");
            if (opPos == std::string::npos)
                opPos = line.find("mul");

            std::string dest, src1, src2;

            if (eqPos != std::string::npos && opPos != std::string::npos)
            {
                dest = line.substr(0, eqPos);
                dest.erase(0, dest.find_first_not_of(" \t"));
                dest.erase(dest.find_last_not_of(" \t") + 1);

                std::istringstream operands(line.substr(opPos));
                std::string op, temp;
                operands >> op >> temp >> src1 >> src2;

                if (!src1.empty() && src1.back() == ',')
                    src1.pop_back();
                if (!src2.empty() && src2.back() == ',')
                    src2.pop_back();

                // std::cout << dest << "-" << src1 << "-" << src2 << std::endl;

                std::string origin1 = getOrigin(src1, dataFlow);
                std::string origin2 = getOrigin(src2, dataFlow);

                // If any source has "SOURCE" as origin, the destination inherits it
                if (origin1 == "SOURCE" || origin2 == "SOURCE")
                {
                    dataFlow[dest] = "SOURCE";
                    // std::cout << "Data flow updated: " << dest << " <- SOURCE" << std::endl;
                }
                else
                {
                    // If no SOURCE involved, mark origin be src1
                    dataFlow[dest] = src1;
                    // std::cout << "Data flow updated: " << dest << " <- " << origin1 << std::endl;
                }
            }
        }
        else if (line.find("call") != std::string::npos)
        {
            std::string token, calledFunc, argument, firstToken;
            bool normalCall = true;
            // should cause no issues in regular call functions, formatted like: call void @function
            // since the call is ignored anyway past finding it
            iss >> token;
            if (token.find('%') != std::string::npos)
            {
                normalCall = false;
                firstToken = token;
            }
            bool inArgumentSection = false;
            while (iss >> token)
            {
                if (token.find('@') != std::string::npos)
                {
                    size_t startPos = token.find('@') + 1;
                    size_t endPos = token.find('(');
                    if (endPos != std::string::npos)
                    {
                        calledFunc = token.substr(startPos, endPos - startPos);
                        inArgumentSection = true;
                        argument = token.substr(endPos + 1);
                    }
                    else
                    {
                        calledFunc = token.substr(startPos);
                    }
                }
                else if (inArgumentSection)
                {
                    size_t endPos = token.find(')');
                    if (endPos != std::string::npos)
                    {
                        argument += " " + token.substr(0, endPos);
                        inArgumentSection = false;
                    }
                    else
                    {
                        argument += " " + token;
                    }
                }
            }

            if (!argument.empty())
            {
                argument.erase(0, argument.find_first_not_of(" \t"));
                argument.erase(argument.find_last_not_of(" \t") + 1);
            }

            if (argument == ")")
            {
                argument.clear();
            }
            // remove type from argument
            // example:
            // i32 %b1 -> %b1
            if (argument.find('%') != std::string::npos)
            {
                std::size_t percentLoc = argument.find('%');
                std::string newArg = argument.substr(percentLoc);
                argument = newArg;
            }
            // std::cout << "Detected call instruction: " << line << std::endl;
            // std::cout << "Function: " << calledFunc << ", Argument: " << argument << std::endl;

            // Create a new block after the call
            std::string afterCallBlock = calledFunc + "_call_ret";
            blockMap[afterCallBlock] = blockCounter++;
            blocks.push_back({afterCallBlock, {}, {}});

            // Add an edge to the ret block and switch to it
            blocks[blockMap[currentBlock]].edges.push_back({0, afterCallBlock});
            currentBlock = afterCallBlock;

            // Test parse
            if (!calledFunc.empty())
            {
                std::string instruction = "call " + calledFunc + " (" + argument + ")";
                blocks[blockMap[currentBlock]].instructions.push_back(instruction);
                // std::cout << "Parsed call: " << instruction << std::endl;
            }

            if (calledFunc == "SOURCE")
            {
                // if the call is in the format of %var = call
                // this will set the source of %var to the function called
                // ASSUMING NO ARGUMENTS IN SOURCE FUNCTION
                if (!normalCall)
                {
                    argument = firstToken;
                }
                if (!argument.empty())
                {
                    dataFlow[argument] = "SOURCE";
                    // std::cout << "Data flow updated: " << argument << " <- SOURCE" << std::endl;
                }
            }
            else if (calledFunc == "SINK")
            {
                // std::cout << "SINK FLOW:" << argument << std::endl;

                std::string origin = getOrigin(argument, dataFlow);
                if (origin == "SOURCE")
                {
                    // std::cout << "Detected sensitive data flow to SINK: " << argument << std::endl;
                    hasFlow = true;
                }
            }
        }
        else if (line.find("br label") != std::string::npos)
        {
            // handles "br label..." aka just jumps to another block
            std::string token;
            std::string label;
            int index = 0;
            while (iss >> token)
            {
                if (token == "label")
                {
                    iss >> label;
                    label = label.substr(1);
                    if (label.back() == ',')
                    {
                        label.pop_back();
                    }
                    blocks[blockMap[currentBlock]].edges.push_back({index++, label});
                }
            }
            if (inTrueBlock)
            {
                inTrueBlock = false;
                inFalseBlock = true;
            }
            else if (inFalseBlock)
            {
                inFalseBlock = false;
            }
        }
        else if (line.find("br") != std::string::npos)
        {
            // handles br with a compare value
            std::string token;
            std::string trueLabel;
            std::string falseLabel;
            while (iss >> token)
            {
                if (token == "label")
                {
                    if (trueLabel.empty())
                    {
                        // handle %label,
                        iss >> trueLabel;
                        trueLabel = trueLabel.substr(1);
                        if (trueLabel.back() == ',')
                        {
                            trueLabel.pop_back();
                        }
                    }
                    else
                    {
                        // handle %label
                        iss >> falseLabel;
                        falseLabel = falseLabel.substr(1);
                    }
                }
            }
            blocks[blockMap[currentBlock]].edges.push_back({0, trueLabel});
            blocks[blockMap[currentBlock]].edges.push_back({1, falseLabel});
            inTrueBlock = true;
        }
    }

    inputFileStream.close();
    if (hasFlow)
    {
        std::cout << "FLOW"
                  << std::endl;
    }
    else
    {
        std::cout << "NO FLOW"
                  << std::endl;
    }
    return 0;
}
