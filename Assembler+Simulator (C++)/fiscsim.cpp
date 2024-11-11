/** 
 * FISC Simulator
 * 
 * Reads an assembly file with hex values, simulates the instructions with register values.
 * Disassmbles if prompted with [-d] flag - prints out the instructions as string onto the command line.
*/

// Imports
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <vector>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <cmath>

/**
 * Instruction split into components
 * address - line number
 * opCode - code for instruction
 * operand[1,2,3] - register values
 */
struct Instruction{
    int address;
    std::uint8_t unsignedInstruction;
    std::string disassembledInstruction;
    int opCode;
    int operand1;
    int operand2;
    int operand3;

    // Initializes each value
    Instruction(std::uint8_t unsignedIns):
        address(-1), 
        unsignedInstruction(unsignedIns), 
        disassembledInstruction(""), 
        opCode(-1), 
        operand1(-1), 
        operand2(-1), 
        operand3(-1){}
};

/**
 * Vector of instruction objects
 */
struct InstructionMemory{
    std::vector<Instruction> instructions;
    void insert(Instruction i){
        instructions.push_back(i);
    }
};

// Decodes each instruction into vvalues for the operation and registers
class Decoder{
    private:
    const int ADD = 0;
    const int AND = 1;
    const int NOT = 2;
    const int BNZ = 3;
    
    public:
    /**
     * Read the input file and create instruction objects for each line, add to instruction memory
     * 
     * @param filename - name of input file
     * @param im - reference to instruction  memory object
     */
    void readFile(std::string filename, InstructionMemory &im){
        std::vector<std::string> hexInstructions;
        std::string lineFromFile;
        std::ifstream myfile;
        myfile.open(filename);
        std::getline(myfile, lineFromFile);
        if(lineFromFile != "v2.0 raw"){
            throw ("ERR: Unable to read file.");
        }
        while(std::getline(myfile, lineFromFile)){
            Instruction i = createInstruction(lineFromFile);
            im.insert(i);
        }
    }

    /**
     * Convert to hex instruction to unisgned binary
     * 
     * @return Unsigned binary instruction
     */
    Instruction createInstruction(std::string instruction){
            int hexInstruction;
            std::stringstream stream;
            stream << instruction;
            stream >> std::hex >> hexInstruction;
            std::uint8_t unsignedInstruction = (uint8_t)hexInstruction;
            Instruction i(unsignedInstruction);
        return i;
    }

    /**
     * For each instruction, decode the instruction parts into integers and store into instruction object
     * Decoded differently depending on instruction structure:
     *      ADD [destination] [source1] [source2]
     *      AND [destination] [source1] [source2]
     *      NOT [destination] [source]
     *      BNZ [branch address]
     */
    void decode(InstructionMemory &im){
        int address = 0;
        for(Instruction &i: im.instructions){
            std::uint8_t instruction = i.unsignedInstruction;
            i.address = address;
            address += 1;

            i.opCode = getBits(i.unsignedInstruction, 6, 8);
            if(i.opCode == ADD || i.opCode == AND || i.opCode == NOT){
                i.operand1 = getBits(i.unsignedInstruction, 0, 2);
                i.operand2 = getBits(i.unsignedInstruction, 4, 6);
            }
            if(i.opCode == ADD || i.opCode == AND){
                i.operand3 = getBits(i.unsignedInstruction, 2, 4);
            }
            if(i.opCode == BNZ){
                i.operand1 = getBits(i.unsignedInstruction, 0, 6);
            }
        }
    }

    /**
     * Get the specified bits in the binary
     * 
     * @param num - the binary value of the instruction
     * @param start - start of the bit range to get
     * @param end - end of the bit range to bit
     */
    std::int8_t getBits(std::int8_t num, int start, int end){
        // start = inclusive, end = exclusive
        num = num >> start;
        int mask = (1 << end - start) - 1;
        std::int8_t result = num & mask;
        return result;
    }
};

// Reconstructs each instructor with the register and operation integer values
class Diassembler{
    public:
    /**
     * Convert the instruction back into the string instruction (ex: not r0 r1)
     * 
     * Decodes each operation/register into words
     * Combine the words base on the instruction type
     * 
     * @param im - Reference to instruction memory
     */
    void disassemble(InstructionMemory &im){
        for(Instruction &i: im.instructions){
            std::string strInstruction;
            std::string operation = decodeOperation(i.opCode);
            std::string operand1 = decodeRegister(i.operand1);
            std::string strOperand1 = std::to_string(i.operand1);
            std::string operand2 = decodeRegister(i.operand2);
            std::string operand3 = decodeRegister(i.operand3);
            if(i.opCode == 0 || i.opCode == 1){
                strInstruction = operation + operand1 + operand2 + operand3;
            }
            else if(i.opCode == 2){
                strInstruction = operation + operand1 + operand2;
            }
            else if(i.opCode == 3){
                strInstruction = operation + strOperand1;
            }
            i.disassembledInstruction = strInstruction;
        }
    }

    /**
     * Decodes each operation value to the corresponding instruction name
     * 
     * @param opCode - operation code (corresponding to an instruction)
     * @return Name of the instruction as string
     */
    std::string decodeOperation(int opCode){
        switch(opCode){
            case 0:
                return "add ";
            case 1:
                return "and ";
            case 2:
                return "not ";
            case 3:
                return "bnz ";
            default:
                return "";
        }
    }

    /**
     * Decodes each register value to the corresponding register name
     * 
     * @param regCode - register value
     * @return Full name of register as string
     */
    std::string decodeRegister(int regCode){
        switch(regCode){
            case 0:
                return "r0 ";
            case 1:
                return "r1 ";
            case 2:
                return "r2 ";
            case 3:
                return "r3 ";
            default:
                return "";
        }
    }
};

/**
 * Register memory
 * ZFlag - if the previous instruction resulted in 0 (used for bnz (branch if not zero))
 */
struct Memory{
    uint8_t registerMemory[4] = {0};
    int zFlag;
    int programCounter;
    Memory():
        zFlag(0),
        programCounter(0){};
};

// Simulates the program using the register memory
class Execute{
    private:
    Memory m;
    public:

    /**
     * Simulates the program
     * 
     * End the program if cycle stops or end of program
     * Run each instruction
     * Return disassembly (the string instruction if [-d] flag is set)
     * 
     * @param im - The parameter of the program
     * @param cycles - The number of cycles to run the program
     * @param disassembly - Boolean whether or not to disassemble
     */
    void runProgram(InstructionMemory im, int cycles, bool disassembly){
        for(int i = 1; i <= cycles; i++){
            if(m.programCounter >= im.instructions.size()){
                throw ("ERR: Cycle stopped, reached end of program.");
            }
            Instruction instruction = im.instructions[m.programCounter];
            runCycle(instruction, disassembly);
            outputState(i);
            if(disassembly){
                outputDisassembly(instruction);
            }
        }
    }

    /**
     * Outputs the disassmebled instruction
     * Ex: not r0 r1
     * 
     * @param i - instruction to disassemble
     */
    void outputDisassembly(Instruction i){
        std::cout << "Disassembly: ";
        std::cout << i.disassembledInstruction << std::endl;
        std::cout << std::endl;
    }

    /**
     * Outputs the current state of the program (after each instruction)
     * Returns the current cycle and values in the registers
     * @param curCycle - The cycle of the simulation or how many have been instructions executed
     */
    void outputState(int curCycle){
        std::cout << std::dec;
        std::cout << "Cycle:" << curCycle;
        std::cout << " State:";
        std::cout << "PC:" << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << m.programCounter;
        std::cout << " " << "Z:" << m.zFlag;
        std::cout << " R0:" << " " << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << (int)m.registerMemory[0];
        std::cout << " R1:" << " " << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << (int)m.registerMemory[1];
        std::cout << " R2:" << " " << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << (int)m.registerMemory[2];
        std::cout << " R3:" << " " << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << (int)m.registerMemory[3] << std::endl;
    }

    /**
     * Calls the corresponding operation's functions based on opCode
     * 
     * @param i - instruction to execute
     * @param disassembly - whether or not to disassemble
     */
    void runCycle(Instruction i, bool disassembly){
        if(i.opCode == 0){
            addOperation(i.operand1, i.operand2, i.operand3);
        }
        else if(i.opCode == 1){
            andOperation(i.operand1, i.operand2, i.operand3);
        }
        else if(i.opCode == 2){
            notOperation(i.operand1, i.operand2);
        }
        else if(i.opCode == 3){
            branch(i.operand1);
        }
        if(m.programCounter == 63){
            m.programCounter = 0;
        }
    }

    /**
     * zFlag = 1 if result of operation is 0
     * zFlag = 0 if result is nonzero
     * 
     * @param regD - the register value of the destination register
     */
    void setZFlag(int regD){
        if(m.registerMemory[regD] == 0){
            m.zFlag = 1;
        }
        else{
            m.zFlag = 0;
        }
    }

    /**
     * Adds the register values in regN and regM and stores it into regD
     * Increases the program counter and sets zFlag as necessary
     * 
     * @param regD - destination register
     * @param regN - source 1 register
     * @param regM - source 2 register
     */
    void addOperation(int regD, int regN, int regM){
        m.registerMemory[regD] = m.registerMemory[regN] + m.registerMemory[regM];
        setZFlag(regD);
        m.programCounter += 1;
    }

    /**
     * Ands the register values in regN and regM and stores it into regD
     * Increases the program counter and sets zFlag as necessary
     * 
     * @param regD - destination register
     * @param regN - source 1 register
     * @param regM - source 2 register
     */
    void andOperation(int regD, int regN, int regM){
        m.registerMemory[regD] = 
            m.registerMemory[regN] & m.registerMemory[regM];
        setZFlag(regD);
        m.programCounter += 1;
    }

    /**
     * Performs not operation on regN and stores into regD
     * Increases the program counter and sets zFlag as necessary
     * 
     * @param regD - destination register
     * @param regN - source 1 register
     */
    void notOperation(int regD, int regN){
        m.registerMemory[regD] = ~m.registerMemory[regN];
        setZFlag(regD);
        m.programCounter += 1;
    }

    /**
     * Branches/jumps to the specified address if the zFlag is set (zFlag == 1)
     * 
     * @param address - address to jump to
     */
    void branch(int address){
        if(!m.zFlag){
            m.programCounter = address;
        }else{
            m.programCounter +=1;
        }
    }
};

// Runs the entire process (decoding, simulating, disassmebling, etc)
class Simulator{
    private:
    std::string filename;
    int cycles = 20;
    bool disassembly = false;
    InstructionMemory im;

    public:
    void printUsageInfo(){
        std::cout << "USAGE:\tfiscsim  <object file> [cycles] [-d]";
        std::cout << "\n\t-d : print disassembly listing with each cycle\n\t";
        std::cout << "if cycles are unspecified the CPU will run for 20 cycles";
    }

    /**
     * Stores the arguments from the command line
     */
    void initialize(int argc, char *argv[]){
        if(argc == 1){
            printUsageInfo();
            throw("");
        }
        else if(argc > 4){
            throw ("ERR: Too many arguments");
        }
        else if(argc >= 2){
            filename = argv[1];
        }
        for(int i = 2; i < argc; i++){
            if(strcmp(argv[i], "-d") == 0){
                disassembly = true;
            }
            else{
                if(isNum(argv[i])){
                    cycles = atoi(argv[i]);
                }
                else{
                    throw ("ERR: Unknown parameter");
                }
            }
        }
    }

    /**
     * General function: checks if string is a number
     * @param num - the string to check
     * @return Boolean whether the string was a number
     */
    bool isNum(std::string num){
        bool isNum = true;
        for(char c: num){
            if(!isdigit(c)){
                isNum = false;
            }
        }
        return isNum;
    }

    /**
     * Runs the decoder
     */
    void decode(){
        Decoder decoder;
        Diassembler disassmebler;
        decoder.readFile(filename, im);
        decoder.decode(im);
        if(disassembly){
            disassmebler.disassemble(im);
        }
    }

    /**
     * Executes the program
     */
    void execute(){
        Execute executor;
        executor.runProgram(im, cycles, disassembly);
    }
};

int main(int argc, char *argv[]){
    Simulator sim;
    try{
        sim.initialize(argc, argv);  
        sim.decode();
        sim.execute();
    }
    catch(const char* err){
        std::cerr << err << std::endl;
    }
    return 0;
}