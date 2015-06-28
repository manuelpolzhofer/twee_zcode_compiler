// Implementation of RoutineGenerator.cpp
// Created by philip on 10.05.15.
//

#include "RoutineGenerator.h"
#include "ZCodeConverter.h"
#include "exceptions.h"
#include "ZCodeObjects/ZCodeJump.h"
#include "ZCodeObjects/ZCodeCallAdress.h"
#include <iostream>
#include <algorithm>
#include <plog/Log.h>

using namespace std;

map<string,ZCodeLabel*> RoutineGenerator::labels = map<string,ZCodeLabel*>();
map<string, size_t> RoutineGenerator::routines = map<string, size_t>();
map<size_t, string> RoutineGenerator::callTo = map<size_t, string>();

void checkParamCount(vector<unique_ptr<ZParam>> &params, unsigned int paramCount1, unsigned int paramCount2 = 0,
                     unsigned int paramCount3 = 0, unsigned int paramCount4 = 0) {
    if (params.size() != paramCount1 && (paramCount2 != 0 && params.size() != paramCount2)
        && (paramCount3 != 0 && params.size() != paramCount3) && (paramCount4 != 0 && params.size() != paramCount4)) {

        LOG_ERROR << "Wrong parameter count";
        throw AssemblyException(AssemblyException::ErrorType::INVALID_ROUTINE);
    }
}

bool sameType(ZParamType paramType, ZParamType neededType) {
    if (neededType == VARIABLE_OR_VALUE) {
        return paramType == VARIABLE || paramType == VALUE;
    } else if (neededType == EMPTY || paramType == EMPTY) {
        return false;
    } else {
        return paramType == neededType;
    }
}

void checkParamType(vector<unique_ptr<ZParam>> &params, ZParamType type1, ZParamType type2 = EMPTY,
                    ZParamType type3 = EMPTY,
                    ZParamType type4 = EMPTY, ZParamType type5 = EMPTY) {
    for (size_t i = 0; i < params.size(); i++) {
        switch (i) {
            case 0:
                if (!sameType((*params.at(0)).getParamType(), type1)) {
                    LOG_ERROR << "Wrong param type for parameter 1";
                    throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
                }
                break;
            case 1:
                if (!sameType((*params.at(1)).getParamType(), type2)) {
                    LOG_ERROR << "Wrong param type for parameter 2";
                    throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
                }
                break;
            case 2:
                if (!sameType((*params.at(2)).getParamType(), type3)) {
                    LOG_ERROR << "Wrong param type for parameter 3";
                    throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
                }
                break;
            case 3:
                if (!sameType((*params.at(3)).getParamType(), type4)) {
                    LOG_ERROR << "Wrong param type for parameter 4";
                    throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
                }
                break;
            case 4:
                if (!sameType((*params.at(4)).getParamType(), type5)) {
                    LOG_ERROR << "Wrong param type for parameter 5";
                    throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
                }
                break;
            default:
                LOG_ERROR << "Too many arguments!";
                throw AssemblyException(AssemblyException::ErrorType::INVALID_INSTRUCTION);
        }
    }

}

ZCodeRoutine *RoutineGenerator::getRoutine() {
   return this->routine;
}

void RoutineGenerator::addBitset(vector<bitset<8>> bitsets, std::string name = "UNKNOWN") {
    ZCodeInstruction *instruction =new ZCodeInstruction(bitsets,name);
    routine->add(instruction);
}

void RoutineGenerator::setTextStyle(bool roman, bool reverseVideo, bool bold, bool italic, bool fixedPitch) {
    debug("textStyle");
    vector<uint16_t> param;
    param.push_back(roman ? 0 : (reverseVideo * 1) + (bold * 2) + (italic * 4) + (fixedPitch * 8));

    vector<bool> paramBools;
    paramBools.push_back(false);

    vector<bitset<8>> command = opcodeGenerator.generateVarOPInstruction(SET_TEXT_STYLE, param, paramBools);
    addBitset(command, "text style");
}

// params: stringToPrint
void RoutineGenerator::printString(vector<unique_ptr<ZParam>> params) {
    debug("print");
    checkParamCount(params, 1);
    checkParamType(params, NAME);

    ZCodeConverter converter = ZCodeConverter();
    vector<bitset<8>> zsciiString = converter.convertStringToZSCII((*params.at(0)).name);
    vector<bitset<8>> instruction = vector<bitset<8>>();
    unsigned long len = zsciiString.size();
    for (size_t i = 0; i < len; i++) {
        if (i % 96 == 0) {
            instruction.push_back(numberToBitset(PRINT));
        }
        if (i % 96 == 94) {
            zsciiString[i].set(7, true);
        }
        instruction.push_back(zsciiString[i]);
    }
    ZCodeInstruction *zinstruction = new ZCodeInstruction(instruction,"print");
    routine->add(zinstruction);
}

// params: stringToPrint
void RoutineGenerator::printRet(vector<unique_ptr<ZParam>> params) {
    checkParamCount(params, 1);
    checkParamType(params, NAME);
    debug("print ret")
    ZCodeConverter converter = ZCodeConverter();
    vector<bitset<8>> zsciiString = converter.convertStringToZSCII((*params.at(0)).name);
    auto instructions = vector<bitset<8>>();
    unsigned long len = zsciiString.size();
    for (size_t i = 0; i < len; i++) {
        if (i % 96 == 0) {
            instructions.push_back(numberToBitset(PRINT_RET));      // TODO: Check if this opcode needs same workaround as "print" for Frotz
        }
        if (i % 96 == 94) {
            zsciiString[i].set(7, true);
        }
        instructions.push_back(zsciiString[i]);
    }
    this->addBitset(instructions,"print_ret");
}

// params: storeAddress
void RoutineGenerator::readChar(vector<unique_ptr<ZParam>> params) {
    debug("read_char");
    // TODO: decide on whether to allow no args for read_char
    checkParamCount(params, 1, 2);
    if (params.size() == 1) {
        checkParamType(params, STORE_ADDRESS);
    } else {
        checkParamType(params, VALUE, STORE_ADDRESS);
    }

    // Read char needs '1' as first parameter (cannot be handled by OpcodeParameterGenerator)
    vector<bitset<8>> instructions = vector<bitset<8>>();
    instructions.push_back(numberToBitset(READ_CHAR));
    instructions.push_back(numberToBitset(0xbf));
    instructions.push_back(numberToBitset(1));
    instructions.push_back(numberToBitset((*params.at(params.size() - 1)).getZCodeValue()));
    ZCodeInstruction *zinstruction = new ZCodeInstruction(instructions, "read char");
    routine->add(zinstruction);
}

// params: variable
void RoutineGenerator::printChar(vector<unique_ptr<ZParam>> params) {
    debug("print_char");
    checkParamCount(params, 1);
    checkParamType(params, VARIABLE);

    vector<bitset<8>> instructions = opcodeGenerator.generateVarOPInstruction(PRINT_CHAR, params);
    addBitset(instructions, "print char");
}

// params: routineName, (arg1, (arg2, (arg3))) resultAddress
void RoutineGenerator::callVS(vector<unique_ptr<ZParam>> params) {
    debug("call_vs");
    checkParamCount(params, 2, 3, 4, 5);
    if (params.size() == 2) {
        checkParamType(params, NAME, STORE_ADDRESS);
    } else if (params.size() == 3) {
        checkParamType(params, NAME, VARIABLE_OR_VALUE, STORE_ADDRESS);
    }
    if (params.size() == 4) {
        checkParamType(params, NAME, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, STORE_ADDRESS);
    }
    if (params.size() == 5) {
        checkParamType(params, NAME, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, STORE_ADDRESS);
    }

    string routineName = (*params.at(0)).name;
    uint8_t storeAddress = (*params.at(params.size() - 1)).getZCodeValue();

    params.erase(params.begin() + params.size() - 1);
    params[0] = unique_ptr<ZValueParam>(new ZValueParam(3000));     // placeholder for call offset

    auto generated = opcodeGenerator.generateVarOPInstruction(CALL_VS, params);
    vector<bitset<8>> zinstructions;
    zinstructions.push_back(generated.at(0));
    zinstructions.push_back(generated.at(1));
    ZCodeInstruction *instructionObjects = new ZCodeInstruction(zinstructions);
    routine->add(instructionObjects);
    ZCodeCallAdress *callAdress = new ZCodeCallAdress(ZCodeRoutine::getOrCreateRoutine(routineName,0));
    routine->add(callAdress);
    vector<bitset<8>> instructionsAfterJump;
    for(size_t i = 4; i < generated.size();i++){
        instructionsAfterJump.push_back(generated.at(i));
    }
    ZCodeInstruction *instructionObjectsAfterJump = new ZCodeInstruction(instructionsAfterJump);
    routine->add(instructionObjectsAfterJump);
}

// params: variableOrConstant
void RoutineGenerator::call1n(vector<unique_ptr<ZParam>> params) {
    debug("call_1n");
    checkParamCount(params, 1);
    checkParamType(params, NAME);

    vector<bitset<8>> generated = opcodeGenerator.generate1OPInstruction(CALL_1N, (u_int16_t) 3000, false);
    vector<bitset<8>> zinstructions;
    zinstructions.push_back(generated.at(0));
    ZCodeInstruction *instructionObjects = new ZCodeInstruction(zinstructions);
    routine->add(instructionObjects);
    ZCodeCallAdress *adress = new ZCodeCallAdress(ZCodeRoutine::getOrCreateRoutine((*params.at(0)).name,0));
    routine->add(adress);
    LOG_DEBUG << "Call Routine at:::" << offsetOfRoutine + routineZcode.size() - 2;
}

bitset<8> RoutineGenerator::numberToBitset(unsigned int number) {
    return bitset<8>(number);
}

void RoutineGenerator::newLine() {
    debug("new_line");
    ZCodeInstruction *instruction = new ZCodeInstruction(NEW_LINE);
    routine->add(instruction);
}

void RoutineGenerator::quitRoutine() {
    debug("quit");
    ZCodeInstruction *instruction = new ZCodeInstruction(QUIT);
    routine->add(instruction);
}

void RoutineGenerator::restart() {
    ZCodeInstruction *instruction = new ZCodeInstruction(RESTART);
    routine->add(instruction);
}

// adds label and next instruction address to 'branches' map
void RoutineGenerator::newLabel(string label) {
    routine->add(getOrCreateLabel(label));
}

void setLabelValues(ZParam &labelParam, string &label, bool &jumpIfTrue) {
    label = labelParam.name;
    jumpIfTrue = true;
    if (label.at(0) == '~') {
        jumpIfTrue = false;
        label = label.substr(1);
    }
}

// params: label
void RoutineGenerator::verify(vector<unique_ptr<ZParam>> params) {
    checkParamCount(params, 1);
    checkParamType(params, NAME);

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(0), label, jumpIfTrue);
    auto *instruction = new ZCodeInstruction(VERIFY);
    auto *jump = new ZCodeJump(getOrCreateLabel(label));
    routine->add(instruction);
    routine->add(jump);
}

// params: label
void RoutineGenerator::jump(vector<unique_ptr<ZParam>> params) {
    checkParamCount(params, 1);
    checkParamType(params, NAME);
    debug("jump");

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(0), label, jumpIfTrue);
    vector<bitset<8>> instructions = vector<bitset<8>>();
    instructions.push_back(numberToBitset(JUMP));
    ZCodeInstruction  *instruction = new ZCodeInstruction(instructions);
    routine->add(instruction);
    ZCodeJump *jump = new ZCodeJump(getOrCreateLabel(label));
    routine->add(jump);
}

// params: param1, label
void RoutineGenerator::jumpZero(vector<unique_ptr<ZParam>> params) {
    debug("jump_zero");
    checkParamCount(params, 2);
    checkParamType(params, VARIABLE_OR_VALUE, NAME);

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(params.size() - 1), label, jumpIfTrue);

    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(JZ, *params.at(0));
    addBitset(instructions);

    ZCodeJump *jump = new ZCodeJump(getOrCreateLabel(label));
    jump->jumpIfCondTrue = jumpIfTrue;
    routine->add(jump);
}

// params: param1, param2, (param3, (param4,)) label
void RoutineGenerator::jumpEquals(vector<unique_ptr<ZParam>> params) {
    debug("je");
    checkParamCount(params, 3, 4, 5);
    if (params.size() == 3) {
        checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, NAME);
    } else if (params.size() == 4) {
        checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, NAME);
    } else {
        checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, NAME);
    }

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(params.size() - 1), label, jumpIfTrue);

    if (params.size() == 3) {
        conditionalJump(JE, label, jumpIfTrue, (*params.at(0)), (*params.at(1)));
    } else {
        params.erase(params.end()); // erase label param
        auto instructions = opcodeGenerator.generateVarOPInstruction(JE, params);
        addBitset(instructions);

        ZCodeJump *jump = new ZCodeJump(getOrCreateLabel(label));
        jump->jumpIfCondTrue = jumpIfTrue;
        routine->add(jump);
    }
}

// params: param1, param2, label
void RoutineGenerator::jumpLessThan(vector<unique_ptr<ZParam>> params) {
    debug("jlt");
    checkParamCount(params, 3);
    checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, NAME);

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(params.size() - 1), label, jumpIfTrue);

    conditionalJump(JL, label, jumpIfTrue, *params.at(0), *params.at(1));
}

// params: param1, param2, label
void RoutineGenerator::jumpGreaterThan(vector<unique_ptr<ZParam>> params) {
    debug("jgt");
    checkParamCount(params, 3);
    checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, NAME);

    string label;
    bool jumpIfTrue;
    setLabelValues(*params.at(params.size() - 1), label, jumpIfTrue);

    conditionalJump(JG, label, jumpIfTrue, *params.at(0), *params.at(1));
}

void RoutineGenerator::conditionalJump(unsigned int opcode, string toLabel, bool jumpIfTrue, ZParam &param1,
                                       ZParam &param2) {
    vector<bitset<8>> instructions = opcodeGenerator.generate2OPInstruction(opcode, param1, param2);
    addBitset(instructions);

    ZCodeJump *jump = new ZCodeJump(getOrCreateLabel(toLabel));
    jump->jumpIfCondTrue = jumpIfTrue;
    routine->add(jump);
}

// params: address, value
// both parameters need to be constansts!!! (first argument is not an addressParameter!)
void RoutineGenerator::store(vector<unique_ptr<ZParam>> params) {
    debug("store");
    checkParamCount(params, 2);
    checkParamType(params, VARIABLE_OR_VALUE, VALUE);

    unique_ptr<ZParam> address(new ZValueParam((*params.at(0)).getZCodeValue()));

    vector<bitset<8>> instructions = opcodeGenerator.generate2OPInstruction(STORE, *address, *params.at(1));
    addBitset(instructions);
}

void RoutineGenerator::base2OpOperation(unsigned int opcode, vector<unique_ptr<ZParam>> &params) {
    checkParamCount(params, 3);
    checkParamType(params, VARIABLE_OR_VALUE, VARIABLE_OR_VALUE, STORE_ADDRESS);

    vector<bitset<8>> instructions = opcodeGenerator.generate2OPInstruction(opcode, *params.at(0), *params.at(1));
    instructions.push_back(bitset<8>(params.at(2)->getZCodeValue()));
    addBitset(instructions);
}

void RoutineGenerator::add(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(ADD, params);
}

void RoutineGenerator::sub(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(SUB, params);
}

void RoutineGenerator::mul(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(MUL, params);
}

void RoutineGenerator::div(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(DIV, params);
}

void RoutineGenerator::doAND(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(AND, params);
}

void RoutineGenerator::doOR(vector<unique_ptr<ZParam>> params) {
    base2OpOperation(OR, params);
}

// params: address, resultAddress
void RoutineGenerator::load(vector<unique_ptr<ZParam>> params) {
    debug("load");
    checkParamCount(params, 2);
    checkParamType(params, VARIABLE, STORE_ADDRESS);

    // TODO: Test again

    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(LOAD, *params.at(0));
    instructions.push_back(numberToBitset((*params.at(1)).getZCodeValue()));
    addBitset(instructions);
}

// params: address
void RoutineGenerator::printAddress(vector<unique_ptr<ZParam>> params) {
    debug("print_addr");
    checkParamCount(params, 1);
    checkParamType(params, VARIABLE_OR_VALUE);

    // TODO: Test this opcode with extra string table

    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(PRINT_ADDR, *params.at(0));
    addBitset(instructions);
}

void RoutineGenerator::setLocalVariable(string name, int16_t value) {
    if (++addedLocalVariables > maxLocalVariables) {
        LOG_ERROR << "Added " << addedLocalVariables << " local variables to routine but only "
                  << maxLocalVariables << " specified at routine start!";
        throw AssemblyException(AssemblyException::ErrorType::INVALID_DIRECTIVE);
    }

    size_t size = locVariables.size() + 1;
    locVariables[name] = size;   // first local variable is at address 1 in stack

    vector<unique_ptr<ZParam>> params;
    params.push_back(unique_ptr<ZParam>(new ZVariableParam(locVariables[name])));
    params.push_back(unique_ptr<ZParam>(new ZValueParam(value)));

    store(move(params));
}

// params: address
void RoutineGenerator::printNum(vector<unique_ptr<ZParam>> params) {
    debug("print_num");
    checkParamCount(params, 1);
    checkParamType(params, VARIABLE_OR_VALUE);

    vector<bitset<8>> instructions = opcodeGenerator.generateVarOPInstruction(PRINT_SIGNED_NUM, params);
    addBitset(instructions);
}

u_int8_t RoutineGenerator::getAddressOfVariable(string name) {
    if (locVariables.count(name) == 0) {
        LOG_ERROR << "Undefined local variable used: " << name;
        throw AssemblyException(AssemblyException::ErrorType::INVALID_LOCAL);
    } else {
        return locVariables[name];
    }
}


bool RoutineGenerator::containsLocalVariable(string name) {
    return locVariables.count(name) != 0;
}

// params: variableOrConstant
void RoutineGenerator::returnValue(vector<unique_ptr<ZParam>> params) {
    debug("ret_value");
    checkParamCount(params, 1);
    checkParamType(params, VARIABLE_OR_VALUE);

    vector<bitset<8>> instructions = opcodeGenerator.generate1OPInstruction(RET_VALUE, *params.at(0));
    addBitset(instructions);
}

void RoutineGenerator::returnTrue() {
    ZCodeInstruction *instruction = new ZCodeInstruction(RETURN_TRUE);
    routine->add(instruction);
}

void RoutineGenerator::returnFalse() {
    ZCodeInstruction *instruction =new  ZCodeInstruction(RETURN_FALSE);
    routine->add(instruction);
}

void RoutineGenerator::retPopped() {
    ZCodeInstruction *instruction = new ZCodeInstruction(RET_POPPED);
    routine->add(instruction);
}

void RoutineGenerator::resolveCallInstructions(vector<bitset<8>> &zCode) {
    typedef map<size_t, string>::iterator it_type;
    for (it_type it = RoutineGenerator::callTo.begin(); it != RoutineGenerator::callTo.end(); it++) {
        size_t calledRoutineOffset = RoutineGenerator::routines[it->second];
        size_t callOffset = it->first;
        vector<bitset<8>> callAdress = vector<bitset<8>>();
        Utils::addTwoBytes(calledRoutineOffset / 8, callAdress);
        zCode[callOffset] = callAdress[0];
        zCode[callOffset + 1] = callAdress[1];
    }
}
ZCodeLabel *RoutineGenerator::getOrCreateLabel(std::string name) {
    if (labels.count(name) == 0) {
        ZCodeLabel *label = new ZCodeLabel();
        label ->displayName = "["+name+"]";
        labels.insert(std::pair<std::string, ZCodeLabel*>(name,label));
        return label;
    } else {
        ZCodeLabel *label = labels.at(name);
        return label;
    }
}

void RoutineGenerator::debug(std::string message) {
    ZCodeConverter converter = ZCodeConverter();
    vector<bitset<8>> zsciiString = converter.convertStringToZSCII("["+message+"]");
    vector<bitset<8>> instruction = vector<bitset<8>>();
    unsigned long len = zsciiString.size();
    for (size_t i = 0; i < len; i++) {
        if (i % 96 == 0) {
            instruction.push_back(numberToBitset(PRINT));
        }
        if (i % 96 == 94) {
            zsciiString[i].set(7, true);
        }
        instruction.push_back(zsciiString[i]);
    }
    instruction.push_back(numberToBitset(NEW_LINE));
    ZCodeInstruction *zinstruction = new ZCodeInstruction(instruction, "**debug message**");
    routine->add(zinstruction);
}