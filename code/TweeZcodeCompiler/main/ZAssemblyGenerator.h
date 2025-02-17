//
// Created by Danni on 09.06.15.
//

#ifndef PROJECT_ZASSEMBLYGENERATOR_H
#define PROJECT_ZASSEMBLYGENERATOR_H

#include <ostream>
#include <map>
#include <string>
#include <utility>
#include <initializer_list>
#include <vector>
#include "optional.hpp"

typedef const std::string INST_TYPE;

struct ZRoutineArgument {
    ZRoutineArgument(std::string argName) : argName(argName), value(std::experimental::nullopt) { }

    ZRoutineArgument(std::string argName, std::string value) : argName(argName), value(value) { }

    std::string argName;
    std::experimental::optional<std::string> value;
};

/*
 * This class will not check if generated code is sane.
 */
class ZAssemblyGenerator {
public:
    static const std::string STACK_POINTER;
    // to remedy some compat quirks with ZAPF
    static const bool ZAPF_MODE;

    static std::string makeArgs(std::initializer_list<std::string> args);

    ZAssemblyGenerator(std::ostream &out);

    ZAssemblyGenerator &addLabel(std::string labelName);

    ZAssemblyGenerator &addByteArray(std::string name, unsigned size);

    ZAssemblyGenerator &addString(std::string name, std::string str);

    ZAssemblyGenerator &addWordArray(std::string name, unsigned size);

    ZAssemblyGenerator &call_vs(std::string routineName, std::experimental::optional<std::string> args, std::string storeTarget);

    ZAssemblyGenerator &call_vn(std::string routineName, std::experimental::optional<std::string> args);

    ZAssemblyGenerator &call_1n(std::string routineName);

    ZAssemblyGenerator &markStart();

    ZAssemblyGenerator &addRoutine(std::string routineName,
                                   std::vector<ZRoutineArgument> args = std::vector<ZRoutineArgument>());

    ZAssemblyGenerator &addGlobal(std::string globalName);

    ZAssemblyGenerator &jump(std::string label);

    ZAssemblyGenerator &jumpEquals(std::string args, std::string targetLabel);

    ZAssemblyGenerator &jumpNotEquals(std::string args, std::string targetLabel);
    
    ZAssemblyGenerator &jumpGreater(std::string args, std::string targetLabel);

    ZAssemblyGenerator &jumpLess(std::string args, std::string targetLabel);

    ZAssemblyGenerator &jumpGreaterEquals(std::string args, std::string targetLabel);

    ZAssemblyGenerator &jumpLower(std::string args, std::string targetLabel);

    ZAssemblyGenerator &jumpLowerEquals(std::string args, std::string targetLabel);

    ZAssemblyGenerator &read_char(std::string storeTarget);

    ZAssemblyGenerator &quit();

    ZAssemblyGenerator &random();

    ZAssemblyGenerator &ret(std::string arg);

    ZAssemblyGenerator &newline();

    ZAssemblyGenerator &setTextStyle(std::string values);

    ZAssemblyGenerator &print(std::string str);
    
    ZAssemblyGenerator &println(std::string str);

    ZAssemblyGenerator &print_num(std::string value);

    ZAssemblyGenerator &printAddr(std::string addr);

    ZAssemblyGenerator &push(std::string value);

    ZAssemblyGenerator &pop();

    ZAssemblyGenerator &variable(std::string variable);

    ZAssemblyGenerator &loadb(std::string arrayName, unsigned int index, std::string storeTarget);

    ZAssemblyGenerator &loadb(std::string arrayName, std::string varIndex, std::string storeTarget);

    ZAssemblyGenerator &loadw(std::string arrayName, std::string varIndex, std::string storeTarget);

    ZAssemblyGenerator &storeb(std::string arrayName, unsigned int index, std::string value);

    ZAssemblyGenerator &storeb(std::string arrayName, std::string var, std::string value);

    ZAssemblyGenerator &storew(std::string arrayName, std::string var, std::string value);

    ZAssemblyGenerator &load(std::string source, std::string target);

    ZAssemblyGenerator &store(std::string target, std::string value);

    ZAssemblyGenerator &point(std::string target, std::string address);

    ZAssemblyGenerator &add(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &sub(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &mul(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &div(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &mod(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &land(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &lor(std::string left, std::string right, std::string storeTarget);

    ZAssemblyGenerator &lnot(std::string variable, std::string storeTarget);

    ZAssemblyGenerator &nop();

    ZAssemblyGenerator &random(std::string range, std::string storeTarget);

    ZAssemblyGenerator &pushStack(std::string stack, std::string value, std::string label,
                                                      bool negateJump);
    ZAssemblyGenerator &readMouse(std::string table);

    ZAssemblyGenerator &mouseWindow(std::string variable);

    ZAssemblyGenerator &setCursor(std::string variableY, std::string variableX, std::string window);

    ZAssemblyGenerator &getCursor(std::string array);

    ZAssemblyGenerator &getWindowProperty(std::string window, std::string propertyNumber, std::string target);

    ZAssemblyGenerator &jumpZero(std::string args, std::string targetLabel);

    ZAssemblyGenerator &windowSize(std::string window, std::string y, std::string x);

    ZAssemblyGenerator &setMargins(std::string left, std::string right, std::string window);

    ZAssemblyGenerator &setWindow(std::string window);

    ZAssemblyGenerator &putWindowProperty(std::string window, std::string propertyNumber, std::string value);

    ZAssemblyGenerator &eraseWindow(std::string window);

    ZAssemblyGenerator &windowStyle(std::string window, std::string flags, std::string operation);

    ZAssemblyGenerator &saveUndo(std::string target);

    ZAssemblyGenerator &restoreUndo(std::string target);

private:
    std::ostream &out;
    std::map<std::string, int> passageName2id;

    ZAssemblyGenerator &addInstruction(INST_TYPE instruction,
                                       std::experimental::optional<std::string> args,
                                       std::experimental::optional<std::pair<std::string, bool>> targetLabelAndNeg,
                                       std::experimental::optional<std::string> storeTarget);

    ZAssemblyGenerator &addDirective(std::string directiveName, std::experimental::optional<std::string> args);
};


#endif //PROJECT_ZASSEMBLYGENERATOR_H
