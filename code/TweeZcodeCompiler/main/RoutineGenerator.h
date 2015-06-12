//
// Created by philip on 10.05.15.
//

#ifndef TWEEZCODECOMPILER_ROUTINEGENERATOR_H
#define TWEEZCODECOMPILER_ROUTINEGENERATOR_H


#include <vector>
#include <string>
#include <bitset>
#include <map>
#include <iostream>
#include "Jumps.h"
#include "OpcodeParameterGenerator.h"
#include "Utils.h"

class RoutineGenerator {

private:
    bool printLogs = true;

    std::map<int, std::bitset<8>> routineZcode;     // keys = offset in routine, bitset = Opcodes etc
    static std::map<std::string, size_t> routines;  //keys = name of routine, value = offset.

    size_t offsetOfRoutine = 0;
    Jumps jumps;
    OpcodeParameterGenerator opcodeGenerator;

    std::bitset<8> numberToBitset(unsigned int number);

    void addBitset(std::vector<std::bitset<8>> bitsets);

    void addTwoBytes(int16_t number, int pos = -1);     // splits 16 bit value up to 2 bytes and adds them to routineZcode

    void addOneByte(std::bitset<8> byte, int pos = -1);  // add one byte to routineZcode

    void conditionalJump(unsigned int opcode, std::string toLabel, bool jumpIfTrue, int16_t param1, u_int16_t param2,
                         bool param1IsVariable, bool param2IsVariable);

public:
    // constructor needed to create first jump to main call
    RoutineGenerator(size_t routineOffset) {
        jumps.setRoutineBitsetMap(routineZcode);

        size_t pkgAdrr = Utils::calculateNextPackageAddress((size_t) (routineOffset + 3));

        std::vector<std::bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(CALL_1N, (u_int16_t) (pkgAdrr / 8), false);
        addBitset(instructions);

        size_t padding = Utils::paddingToNextPackageAddress(routineZcode.size(), routineOffset);

        for (size_t i = 0; i < padding; i++) {
            addOneByte(numberToBitset(0));
        }
    }

    // this constructor padding zCode to the next package adress and initialize this routine with the name 'name'
    RoutineGenerator(std::string name,unsigned int locVar, std::vector<std::bitset<8>> &zCode, size_t offsetOfZCode){
        size_t padding = Utils::paddingToNextPackageAddress(zCode.size(), offsetOfZCode);
        Utils::fillWithBytes(zCode, 0, padding);

        RoutineGenerator::routines[name] = zCode.size() + offsetOfZCode;
        this->offsetOfRoutine = zCode.size() + offsetOfZCode;
        std::cout << padding << "/" << this->offsetOfRoutine << "\n";
        jumps.setRoutineBitsetMap(routineZcode);
        jumps.routineOffset = this->offsetOfRoutine;
        addOneByte(numberToBitset(locVar));
    }

    // returns complete zcode of Routine as a bitset vector
    std::vector<std::bitset<8>> getRoutine();

    /*
     *      methods to handle call routine offsets:
     */

    static void resolveCallInstructions(std::vector<std::bitset<8>> &zCode);

    static std::map<size_t, std::string> callTo;    //keys = offset of call, value = name of routine

    /*
     *      methods to add intermediate code instructions to routine
     */

    void newLine();

    void newLabel(std::string label);

    void jump(std::string toLabel);

    void jumpZero(std::string toLabel, bool jumpIfTrue, int16_t variable, bool parameterIsVariable);

    void jumpLessThan(std::string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2, bool param1IsVariable,
                      bool param2IsVariable);

    void jumpGreaterThan(std::string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2,
                         bool param1IsVariable, bool param2IsVariable);

    void jumpEquals(std::string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2, bool param1IsVariable,
                    bool param2IsVariable);

    void jumpEquals(std::string toLabel, bool jumpIfTrue, u_int16_t param, bool paramIsVariable);

    void readChar(uint8_t var);

    void printChar(uint8_t var);

    void printString(std::string stringToPrint);

    void printStringAtAddress(u_int8_t address);

    //Call to a routine with spezific name
    void callRoutine(std::string nameOfRoutine);

    void store(u_int8_t address, u_int16_t value);

    void load(u_int8_t address, u_int8_t result_address);

    void quitRoutine();


    /*
     *      Enumerations
     */

    enum Opcode : unsigned int {
        //Opcode: VAR:246 16 4 read_char 1 time routine -> (result)
                READ_CHAR = 246,
        //Opcode: VAR:229 5 print_char output-character-code
                PRINT_CHAR = 229,
        //Opcode: 1OP:143 F 5 call_1n routine
                CALL_1N = 143,
        // Print new line
                NEW_LINE = 187,
        // Opcodes for jump instructions
                JE = 1,
        JL = 2,
        JG = 3,
        JZ = 128,
        JUMP = 140,
        // Opcode for print operation; following by Z-character String
                PRINT = 178,
        // Opcode: quit the main; no arguments.
                QUIT = 186,
        // Opcode: store variable
                STORE = 13,
        // Opcode: load a variable
                LOAD = 142,
        // Opcode: print zscii encoded string at address
                PRINT_ADDR = 135
    };

    enum BranchOffset {
        // Offset placeholder used for unconditional jumps
                JUMP_UNCOND_OFFSET_PLACEHOLDER = 1,
        // Offset placeholder used for conditional jumps
                JUMP_COND_OFFSET_PLACEHOLDER = 2,
        // Conditional jump: bit 7 of first byte of offset determines whether to jump if condition is true (1) or false (0).
        // If set to true, value of first byte is at least 128 (2^7) in decimal
                JUMP_COND_TRUE = 7,
        // Conditional jump: bit 6 of first byte of offset determines whether the offset is only between 0 and 63 over the
        // bit 5 to 0 or signed offset over 14 bits (set to 1 -> offset only 6 bits)
                JUMP_OFFSET_5_BIT = 6
    };
};


#endif //TWEEZCODECOMPILER_ROUTINEGENERATOR_H
