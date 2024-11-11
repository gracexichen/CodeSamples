/** 
 * FISC Assmebler
 * 
 * Two pass assmebler that reads an assembly file and converts it to hexadecimal machine code.
 * Pass one of the assembler 
*/

// Imports
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <iomanip>

/**
 * Instruction struct seperating components of one instruction line
 * 
 * Ex: 
 *      loop:   and r3 r0 r0    ; r3 now has zero
 * 
 *      label: "loop"
 *      cleanInstruction: "and r3 r0 r0"
 *      comment: "r3 now has zero"
 *      decimalInstruction: 67 (43 in hex)
 */
struct Instruction{
    int address;
    std::string label;
    std::string cleanInstruction;
    std::string comment;
    int decimalInstruction;
};

// Stores labels for specific instructions
struct LabelAddressMap{
    std::vector<std::pair<std::string, int>> labelAddressMap;

    /**
     * Inserts label name and line number
     * 
     * @param label - Label name as a string
     * @param address - The address that the label points to
     */
    void insert(std::string label, int address){
        labelAddressMap.push_back(std::make_pair(label, address));
    }

    /**
     * Returns address corresponding to label
     * 
     * @param label - Label string
     * @return Address of label or error
     */
    int find(std::string label){
        for(auto l: labelAddressMap){
            if(l.first == label){
                return l.second;
            }
        }
        throw ("ERR: Label not found");
    }

    /**
     * Check if label exists in the map
     * @param label - Label string
     * @return True/False if label exists in map
     */
    bool labelExists(std::string label){
        for(auto l: labelAddressMap){
            if(l.first == label){
                return true;
            }
        }
        return false;
    }
};

// Parses the instruction into Instruction objects
class Parser{
    private:
    int address = 0;

    public:
    /**
     * Reads the file and seperates it by lines
     * 
     * @param filename - name of file to be read
     * @return Lines of file stored in a vector of strings
     */
    std::vector<std::string> readFileByLines(std::string filename){
        std::vector<std::string> fileLines;
        std::string lineFromFile;
        std::ifstream myfile;
        myfile.open(filename);
        if (!myfile.good()){
            throw ("ERR: Cannot open file.");
        }
        while(std::getline(myfile, lineFromFile)){
            fileLines.push_back(lineFromFile);
        }
        return fileLines;
    }

    /**
     * Trims leading and trailing whitespace characters from input
     * 
     * @param str - string to remove whtiespace from
     * @return String with whitespace removed
     */
    std::string trimWhitespace(std::string str){
        size_t leftBound = str.find_first_not_of(" \t");
        size_t rightBound = str.find_last_not_of(" \t");

        size_t outOfBound = std::string::npos;
        if (leftBound == outOfBound && rightBound == outOfBound) {
            return "";
        }

        return str.substr(leftBound, rightBound - leftBound + 1);
    }

    /**
     * Parses each line into an instruction object (label, cleaned instruction, and comments)
     * 
     * @param line - The line of assembly to be parsed into instruction
     * @return Instruction object
     */
    Instruction parseLineIntoInstruction(std::string line){
        Instruction instruction;
        // check from comments
        char commentDelimeter = ';';
        char labelDelimeter = ':';

        size_t commentDelimeterPosition = line.find(commentDelimeter);
        if(commentDelimeterPosition != std::string::npos){
            instruction.comment = line.substr(commentDelimeterPosition + 1);
            line = line.substr(0, commentDelimeterPosition);
        }
        else{
            instruction.comment = "";
        }

        size_t labelDelimeterPosition = line.find(labelDelimeter);
        if(labelDelimeterPosition != std::string::npos){
            instruction.label = line.substr(0, labelDelimeterPosition);
            line = line.substr(labelDelimeterPosition + 1);
        }
        else{
            instruction.label = "";    
        }
        std::string trimmedLine = trimWhitespace(line);
        instruction.address = address;
        instruction.cleanInstruction = trimmedLine;
        if(instruction.cleanInstruction.length() != 0){
            address += 1;
        }
        return instruction;
    }
};

// Builds output of converting instruction to hex
class OutputBuilder{
    private:
    LabelAddressMap labelAddressMap;

    public:
    OutputBuilder(LabelAddressMap map){
        labelAddressMap = map;
    }

    /**
     * Splits the instruction into parts: instruction, registers used
     * 
     * Ex:
     * Instruction - not r0 r1
     * Parts - ["not", "r0", "r1"]
     * 
     * @param instruction - instruction as string
     * @return Parts of the instruction
     */
    std::vector<std::string> splitInstruction(std::string instruction){
        int hexInstruction;
        char space = ' ';
        std::vector<std::string> parts;
        size_t splitPosition = instruction.find(space);
        while(splitPosition != std::string::npos){
            parts.push_back(instruction.substr(0, splitPosition));
            instruction = instruction.substr(splitPosition + 1);
            splitPosition = instruction.find(space);
        }
        parts.push_back(instruction);
        return parts;
    }

    /**
     * Converts the instruction to decimal values
     * 
     * Initializes a blank 8-bit binary value
     * Gets the value of each instruction from the function codeFromName
     * Writes each value into corresponding positions using writeTwoBits and WriteSixBits as binary values
     * Converts the binary into decimal values
     * 
     * @param parts - parts of the instruction
     * @return The decimal value that represents the instruction
     */
    int instructionToDecimal(std::vector<std::string> parts){
        std::string binary = "00000000";
        int opCode = codeFromName(parts[0]);
        binary = writeTwoBits(binary, opCode, 1);
        if(opCode == 0 or opCode == 1){
            binary = writeTwoBits(binary, codeFromName(parts[1]), 4);
            binary = writeTwoBits(binary, codeFromName(parts[2]), 2);
            binary = writeTwoBits(binary, codeFromName(parts[3]), 3);
        }
        else if(opCode == 2){
            binary = writeTwoBits(binary, codeFromName(parts[1]), 4);
            binary = writeTwoBits(binary, 0, 3);
            binary = writeTwoBits(binary, codeFromName(parts[2]), 2);
        }
        else if(opCode == 3){
            int address = labelAddressMap.find(parts[1]);
            binary = writeSixBits(binary, address);
        }
        int result = std::stoi(binary, nullptr, 2);
        return result;
    }

    /**
     * Return the corresponding value for each instruction or register value
     * 
     * @param name - instruction or register name
     * @return Value representing instruction/register
     */
    int codeFromName(std::string name){
        int opCode;
        for(char &letter:name){
            letter = tolower(letter);
        }
        if(name == "add" || name == "r0"){
            return 0;
        }
        else if(name == "and" || name == "r1"){
            return 1;
        }
        else if(name == "not" || name == "r2"){
            return 2;
        }
        else if(name == "bnz" || name == "r3"){
            return 3;
        }
        else{
            throw ("ERR: Invalid opCode/register.");
        }
    }

    /**
     * Writes the binary value of the number (representing instruction/register) into the selected position
     * @param binary - The binary value representing the instruction
     * @param num - A decimal value that is written into binary
     * @param position - position/bit to write onto
     * 
     * @return A binary value with the num written in the selected position
     */
    std::string writeTwoBits(std::string binary, int num, int position){
        binary[position*2-1] = (num % 2) + '0';
        num = num / 2;
        binary[position*2-2] = (num % 2) + '0';
        return binary;
    }

    /**
     * Writes the binary value of the number (representing instruction/register) into the selected position
     * @param binary - The binary value representing the instruction
     * @param num - A decimal value that is written into binary
     * 
     * @return A binary value with the num written in the selected position
     */
    std::string writeSixBits(std::string binary, int num){
        for(int i = 7; i >= 2; i--){
            binary[i] = (num % 2) + '0';
            num = num / 2;
        }
        return binary;
    }

    /**
     * Writes each hex instruction to the output file
     * Note decimal is converted to hex with std::hex 
     * 
     * @param file - output file to write to
     * @param instructions - vector of instruction objects
     */
    void writeToFile(std::string file, std::vector<Instruction> instructions){
        std::ofstream outputfile (file);
        outputfile << "v2.0 raw" << std::endl;

        for(auto i: instructions){
            outputfile << std::uppercase << std::setw(2) << std::setfill('0');
            outputfile << std::hex << i.decimalInstruction << std::endl;
        }
        outputfile.close();
    }

    /**
     * If the user uses the [-l] flag, output the information to the command line
     * 
     * Outputs the label list and the address that corresponds to each label
     * Outputs the machine program with the address, hex instruction, and string instructino
     * Note instructions are converted to hex with std::hex
     * 
     * @param instructions - vector of instruction objects
     */
    void printListTable(std::vector<Instruction> instructions){
        std::cout << "*** LABEL LIST ***" << std::endl;
        for (auto label: labelAddressMap.labelAddressMap){
            std::cout << label.first << '\t';
            std::cout << std::uppercase << std::setw(2) << std::setfill('0');
            std::cout << std::hex << label.second << std::endl;
        }
        std::cout << "*** MACHINE PROGRAM ***" << std::endl;
        for(Instruction i: instructions){
            std::cout << std::hex << std::uppercase << std::setw(2) 
                << std::setfill('0') << i.address << ":";
            std::cout << std::hex << std::uppercase << std::setw(2) 
                << std::setfill('0') << i.decimalInstruction << '\t';
            std::cout << i.cleanInstruction << std::endl;
        }
    }
};

class Assembler{
    private:
    std::string filename;
    std::string outputFilename;
    bool listOutput;

    std::vector<std::string> fileLines;
    std::vector<Instruction> instructions;
    LabelAddressMap labelAddressMap;

    public:
    /**
     * Prints usage info for the program
     */
    void printUsageInfo(){
            std::cout << "USAGE:  fiscas <source file> <object file> [-l]";
            std::cout << "\n\t-l : print listing to standard error";
    }

    /**
     * Parses and stores command line arguments
     */
    void initFromCmdLine(int argc, char *argv[]){
        if(argc == 1){
            printUsageInfo();
        }
        else if(argc != 3 && argc != 4){
            printUsageInfo();
        }

        filename = argv[1];
        outputFilename = argv[2];
        listOutput = false;

        if(argc == 4){
            std::string listOption(argv[3]);
            if(!listOption.compare("-l")){
                listOutput = true;
            }
        }
    }

    /**
     * Reads the file, processes the instruction, and extract the label/address pairs
     */
    void passOne(){
        Parser parser;
        fileLines = parser.readFileByLines(filename);
        for (std::string line : fileLines){
            Instruction result = parser.parseLineIntoInstruction(line);
            if(result.cleanInstruction.length() == 0 && 
                result.label.length() != 0){
                    labelAddressMap.insert(result.label, result.address);
            }
            if(result.cleanInstruction.length() != 0){
                if(labelAddressMap.labelExists(result.label)){
                    throw ("ERR: Duplicate labels detected.");
                }
                instructions.push_back(result);
                if(result.label.length()!=0){
                    labelAddressMap.insert(result.label, result.address);
                }
            }
        }
    }

    /**
     * Builds the output with the information extracted from pass one
     * Writes the information to the output file
     * Print the information to command line if [-l] flag is used
     */
    void passTwo(){
        OutputBuilder outputBuilder(labelAddressMap);
        for(Instruction& i: instructions){
            std::vector<std::string> parts;
            parts = outputBuilder.splitInstruction(i.cleanInstruction);
            i.decimalInstruction = outputBuilder.instructionToDecimal(parts);
        }
        outputBuilder.writeToFile(outputFilename, instructions);
        if(listOutput){
            outputBuilder.printListTable(instructions);
        }
    }
};

int main(int argc, char *argv[]) {
    Assembler assembler;
    try{
        assembler.initFromCmdLine(argc, argv);
        assembler.passOne();
        assembler.passTwo();
    }
    catch(const char* err){
        std::cerr << err << std::endl;
    }
    return 0;
}