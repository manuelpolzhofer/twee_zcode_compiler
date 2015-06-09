// Implementation of RoutineGenerator.cpp
// Created by philip on 10.05.15.
//

#include "RoutineGenerator.h"
#include "ZCodeConverter.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>

using namespace std;

vector<bitset<8>> RoutineGenerator::getRoutine() {
    /*if (!quitOpcodePrinted) {
        cout << "Routine has no quit expression!" << endl;
        throw;
    }*/

    jumps.calculateOffsets();

    vector<bitset<8>> zcode;
    for (map<int, bitset<8>>::iterator entry = routineZcode.begin(); entry != routineZcode.end(); ++entry) {
        zcode.push_back(entry->second);
    }

    return zcode;
}

void RoutineGenerator::addBitset(std::bitset<8> byte, int pos) {
    routineZcode[pos < 0 ? routineZcode.size() : pos] = byte;
}

void RoutineGenerator::addBitset(vector<bitset<8>> bitsets) {
    for (bitset<8> bitset : bitsets) {
        routineZcode[routineZcode.size()] = bitset;
    }
}

void RoutineGenerator::printString(std::string stringToPrint) {
    ZCodeConverter converter = ZCodeConverter();
    vector<bitset<8>> zsciiString = converter.convertStringToZSCII(stringToPrint);

    unsigned long len = zsciiString.size();
    for (int i = 0; i < len; i++) {
        if (i % 96 == 0) {
            addBitset(numberToBitset(PRINT));
        }
        if (i % 96 == 94) {
            zsciiString[i].set(7, true);
        }
        addBitset(zsciiString[i]);
    }
}

void RoutineGenerator::readChar(uint8_t var) {
    addBitset(numberToBitset(READ_CHAR));
    addBitset(numberToBitset(0xbf));
    addBitset(numberToBitset(1));
    addBitset(numberToBitset(var));
}

void RoutineGenerator::printChar(uint8_t var) {
    addBitset(numberToBitset(PRINT_CHAR));
    addBitset(numberToBitset(0xbf));
    addBitset(numberToBitset(var));
}

void RoutineGenerator::callToMainRoutine(size_t offset, unsigned int locVar) {
    size_t pkgAdrr = Utils::calculateNextPackageAddress((size_t) (offset + 3));

    //call the main
    addBitset(bitset<8>(CALL_1N));
    addLargeNumber((uint16_t) (pkgAdrr / 8));
    size_t padding = Utils::paddingToNextPackageAddress(routineZcode.size(), offset);

    for (size_t i = 0; i < padding; i++) {
        addBitset(numberToBitset(0));
    }

    addBitset(numberToBitset(locVar));
}

std::bitset<8> RoutineGenerator::numberToBitset(unsigned int number) {
    return bitset<8>(number);
}

void RoutineGenerator::newLine() {
    addBitset(numberToBitset(NEW_LINE));
}

// The destination of the jump opcode is:
// Address after instruction + Offset - 2
void RoutineGenerator::jump(string toLabel) {
    addBitset(numberToBitset(JUMP));
    jumps.newJump(toLabel);
    addLargeNumber(1 << (JUMP_UNCOND_OFFSET_PLACEHOLDER + 7)); // placeholder, will be replaced in getRoutine()
}

void RoutineGenerator::quitRoutine() {
    quitOpcodePrinted = true;
    addBitset(numberToBitset(QUIT));
}

// adds label and next instruction address to 'branches' map
void RoutineGenerator::newLabel(string label) {
    jumps.newLabel(label);
}

void RoutineGenerator::jumpZero(string toLabel, bool jumpIfTrue, int16_t parameter, bool parameterIsVariable) {
    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(JZ, parameter, parameterIsVariable);
    addBitset(instructions);

    jumps.newJump(toLabel);

    bitset<8> offsetFirstBits;
    offsetFirstBits.set(JUMP_COND_TRUE, jumpIfTrue);
    offsetFirstBits.set(JUMP_COND_OFFSET_PLACEHOLDER, true);
    offsetFirstBits.set(JUMP_UNCOND_OFFSET_PLACEHOLDER, false);

    // placeholder, will be replaced in getRoutine()
    addBitset(offsetFirstBits);
    addBitset(numberToBitset(0));
}

void RoutineGenerator::jumpEquals(string toLabel, bool jumpIfTrue, u_int16_t param, bool paramIsVariable) {
    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(JE, param, paramIsVariable);
    addBitset(instructions);

    jumps.newJump(toLabel);

    bitset<8> offsetFirstBits;
    offsetFirstBits.set(JUMP_COND_TRUE, jumpIfTrue);
    offsetFirstBits.set(JUMP_COND_OFFSET_PLACEHOLDER, true);
    offsetFirstBits.set(JUMP_UNCOND_OFFSET_PLACEHOLDER, false);

    // placeholder, will be replaced in getRoutine()
    addBitset(offsetFirstBits);
    addBitset(numberToBitset(0));
}

void RoutineGenerator::jumpEquals(string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2, bool param1IsVariable, bool param2IsVariable) {
    conditionalJump(JE, toLabel, jumpIfTrue, param1, param2, param1IsVariable, param2IsVariable);
}

void RoutineGenerator::jumpLessThan(string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2, bool param1IsVariable, bool param2IsVariable) {
    conditionalJump(JL, toLabel, jumpIfTrue, param1, param2, param1IsVariable, param2IsVariable);
}

void RoutineGenerator::jumpGreaterThan(string toLabel, bool jumpIfTrue, u_int16_t param1, u_int16_t param2, bool param1IsVariable, bool param2IsVariable) {
    conditionalJump(JG, toLabel, jumpIfTrue, param1, param2, param1IsVariable, param2IsVariable);
}

void RoutineGenerator::conditionalJump(unsigned int opcode, std::string toLabel, bool jumpIfTrue, int16_t param1, u_int16_t param2, bool param1IsVariable, bool param2IsVariable) {
    vector<bitset<8>> instructions = opcodeGenerator.generate2OPInstruction(opcode, param1, param2, param1IsVariable, param2IsVariable);
    addBitset(instructions);

    jumps.newJump(toLabel);

    bitset<8> offsetFirstBits;
    offsetFirstBits.set(JUMP_COND_TRUE, jumpIfTrue);
    offsetFirstBits.set(JUMP_COND_OFFSET_PLACEHOLDER, true);
    offsetFirstBits.set(JUMP_UNCOND_OFFSET_PLACEHOLDER, false);

    // placeholder, will be replaced in getRoutine()
    addBitset(offsetFirstBits);
    addBitset(numberToBitset(0));
}

void RoutineGenerator::store(u_int8_t address, u_int16_t value) {
    vector<bitset<8>> instructions = opcodeGenerator.generate2OPInstruction(STORE, address, value, true, false);
    addBitset(instructions);
}

void RoutineGenerator::load(u_int8_t address, u_int8_t result_address) {
    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(LOAD, address, true);
    addBitset(instructions);

    addBitset(numberToBitset(result_address));
}

void RoutineGenerator::printStringAtAddress(u_int8_t address) {
    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(PRINT_ADDR, address, true);
    addBitset(instructions);
}

void RoutineGenerator::addLargeNumber(int16_t number) {
    addLargeNumber(number, -1);
}

void RoutineGenerator::addLargeNumber(int16_t number, int pos) {
    bitset<16> shortVal (number);
    bitset<8> firstHalf, secondHalf;

    for (size_t i = 0; i < 8; i++) {
        secondHalf.set(i, shortVal[i]);
    }

    for (size_t i = 8; i < 16; i++) {
        firstHalf.set(i - 8, shortVal[i]);
    }

    if (pos >= 0) {
        addBitset(firstHalf, pos);
        addBitset(secondHalf, pos + 1);
    } else {
        addBitset(firstHalf);
        addBitset(secondHalf);
    }
}