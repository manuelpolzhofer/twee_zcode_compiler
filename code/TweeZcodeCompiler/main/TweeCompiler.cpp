//
// Created by Danni on 09.06.15.
//

#include "TweeCompiler.h"
#include "ZAssemblyGenerator.h"
#include "exceptions.h"
#include "Utils.h"

#include <plog/Log.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <set>
#include <array>

#include <Passage/Body/Link.h>
#include <Passage/Body/Text.h>
#include <Passage/Body/Newline.h>

#include <Passage/Body/Expressions/BinaryOperation.h>
#include <Passage/Body/Expressions/UnaryOperation.h>
#include <algorithm>

#include <plog/Log.h>
#include <Passage/Body/Expressions/Const.h>
#include <Passage/Body/Expressions/Variable.h>

#include <Passage/Body/Macros/Print.h>
#include <Passage/Body/Macros/SetMacro.h>

#include <Passage/Body/Expressions/Variable.h>
#include <Passage/Body/Macros/IfMacro.h>
#include <Passage/Body/Macros/ElseIfMacro.h>
#include <Passage/Body/Macros/ElseMacro.h>
#include <Passage/Body/Macros/EndIfMacro.h>



using namespace std;
using namespace std::experimental;


static const string GLOB_PASSAGE = "PASSAGE_PTR",
        GLOB_PASSAGES_COUNT = "PASSAGES_COUNT",
        LABEL_MAIN_LOOP = "MAIN_LOOP",
        ROUTINE_MAIN = "main",
        GLOBAL_USER_INPUT = "USER_INPUT",
        TABLE_LINKED_PASSAGES = "LINKED_PASSAGES",
        TABLE_USERINPUT_LOOKUP = "USERINPUT_LOOKUP",
        ROUTINE_PASSAGE_BY_ID = "passage_by_id",
        ROUTINE_NAME_FOR_PASSAGE = "print_name_for_passage",
        ROUTINE_DISPLAY_LINKS = "display_links",
        ELSE_LABEL_PREFIX = "ELSE",
        ENDIF_LABEL_PREFIX = "ENDIF",
        ROUTINE_CLEAR_TABLES = "reset_tables";

static const unsigned int ZSCII_NUM_OFFSET = 49;

//#define ZAS_DEBUG

#ifndef ASSGEN
#define ASSGEN (*_assgen)
#endif

void maskString(std::string &string) {
    std::replace(string.begin(), string.end(), ' ', '_');
}

string routineNameForPassageName(std::string passageName) {
    stringstream ss;
    maskString(passageName);
    ss << "R_" << passageName;
    return ss.str();
}


string routineNameForPassage(Passage &passage) {
    return routineNameForPassageName(passage.getHead().getName());
}

string labelForPassage(Passage &passage) {
    stringstream ss;
    string passageName = passage.getHead().getName();
    maskString(passageName);
    ss << "L_" << passageName;
    return ss.str();
}

string makeIfCaseLabel(const IfContext& context) {
    return ELSE_LABEL_PREFIX + to_string(context.number) + "_" + to_string(context.caseCount);
}

string makeIfEndLabel(const IfContext& context) {
    return ENDIF_LABEL_PREFIX + to_string(context.number);
}


string makeUserInputRoutine() {
    return ".FUNCT display_links i, selectcount\n"
            "loop_start:\n"
            "loadb LINKED_PASSAGES i -> sp\n"
            "jz sp ?passage_not_set\n"
            "storeb USERINPUT_LOOKUP selectcount i\n"
            "add selectcount 1 -> sp\n"
            "add selectcount 49 -> sp\n"
            "print_char sp\n"
            "print \": \"\n"
            "call_vs print_name_for_passage i -> sp\n"
            "new_line\n"
            "add selectcount 1 -> selectcount\n"
            "\n"
            "passage_not_set:\n"
            "\n"
            "add i 1 -> i\n"
            "jl i PASSAGES_COUNT ?loop_start\n"
            "\n"
            "read_char 1 -> USER_INPUT\n"
            "sub USER_INPUT 48 -> sp\n"
            "sub sp 1 -> sp\n"
            "loadb USERINPUT_LOOKUP sp -> sp \n"
            "\n"
            "ret sp"
            "\n";
}

string makeClearTablesRoutine() {
    return ".FUNCT reset_tables i\n"
            "loop_start:\n"
            "storeb LINKED_PASSAGES i 0\n"
            "storeb USERINPUT_LOOKUP i 0\n"
            "add i 1 -> i\n"
            "jl i PASSAGES_COUNT ?loop_start\n"
            "ret 0"
            "\n";
}


void TweeCompiler::compile(TweeFile &tweeFile, std::ostream &out) {
    _assgen = unique_ptr<ZAssemblyGenerator>(new ZAssemblyGenerator(out));
    vector<Passage> passages = tweeFile.getPassages();
    globalVariables = std::set<std::string>();
    labelCount = 0;
    ifContexts = stack<IfContext>();

    {
        int i = 0;
        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            passageName2id[passage->getHead().getName()] = i;
            i++;
        }
    }

    // tables needed for routine linking
    ASSGEN.addByteArray(TABLE_LINKED_PASSAGES, (unsigned)passages.size());
    ASSGEN.addByteArray(TABLE_USERINPUT_LOOKUP, (unsigned)passages.size());


    // globals
    ASSGEN.addGlobal(GLOB_PASSAGE)
            .addGlobal(GLOBAL_USER_INPUT)
            .addGlobal(GLOB_PASSAGES_COUNT);


    // main routine
    {
        // call start routine first
        ASSGEN.addRoutine(ROUTINE_MAIN)
                .markStart()
                .store(GLOB_PASSAGES_COUNT, to_string(passages.size()))
                .call_1n(ROUTINE_CLEAR_TABLES)
                .call_vs(ROUTINE_PASSAGE_BY_ID, to_string(passageName2id.at(string("Start"))), "sp");

        ASSGEN.addLabel(LABEL_MAIN_LOOP)
                .call_vs(ROUTINE_DISPLAY_LINKS, nullopt, "sp")
                .call_vs(ROUTINE_CLEAR_TABLES, nullopt, "sp")
                .call_vs(ROUTINE_PASSAGE_BY_ID, string("sp"), "sp")
                .jump(LABEL_MAIN_LOOP);

        ASSGEN.quit();
    }

    out << makeUserInputRoutine() << makeClearTablesRoutine();

    // call passage by id routine
    {
        const string idLocal = "id";
        ASSGEN.addRoutine(ROUTINE_PASSAGE_BY_ID, vector<ZRoutineArgument>({ZRoutineArgument(idLocal)}));
        for(auto it = passages.begin(); it != passages.end(); ++it) {
            string passageName = it->getHead().getName();
            unsigned id = passageName2id.at(passageName);
            ASSGEN.jumpEquals(ASSGEN.makeArgs({idLocal, to_string(id)}), labelForPassage(*it));
        }

        // error case
        ASSGEN.print("invalid id for passage "); // TODO: print offending argument as soon as print_num is available
        ASSGEN.quit();

        for(auto it = passages.begin(); it != passages.end(); ++it) {
            ASSGEN.addLabel(labelForPassage(*it)).call_vs(routineNameForPassage(*it), nullopt, "sp").ret("0");
        }
    }


    // print passage name by id routine
    {
        const string idLocal = "id";
        ASSGEN.addRoutine(ROUTINE_NAME_FOR_PASSAGE, vector<ZRoutineArgument>({ZRoutineArgument(idLocal)}));
        for(auto it = passages.begin(); it != passages.end(); ++it) {
            string passageName = it->getHead().getName();
            unsigned id = passageName2id.at(passageName);
            ASSGEN.jumpEquals(ASSGEN.makeArgs({idLocal, to_string(id)}), labelForPassage(*it));
        }

        // error case
        ASSGEN.print("invalid id for passage "); // TODO: print offending argument as soon as print_num is available
        ASSGEN.quit();

        for(auto it = passages.begin(); it != passages.end(); ++it) {
            ASSGEN.addLabel(labelForPassage(*it)).print(it->getHead().getName()).ret("0");
        }
    }

    // passage routines
    {
        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            const vector<unique_ptr<BodyPart>> &bodyParts = passage->getBody().getBodyParts();

            // declare passage routine
            ASSGEN.addRoutine(routineNameForPassage(*passage));

            ASSGEN.println(string("***** ") + passage->getHead().getName() + string(" *****"));

            //  print passage contents
            for(auto it = bodyParts.begin(); it != bodyParts.end(); it++) {
                size_t ifDepth = ifContexts.size();

                BodyPart* bodyPart = it->get();
                if(Text* text = dynamic_cast<Text*>(bodyPart)) {
                    ASSGEN.print(text->getContent());
                } else if (Variable* variable = dynamic_cast<Variable*>(bodyPart)) {
                    ASSGEN.variable(variable->getName());
                } else if (Link* link = dynamic_cast<Link*>(bodyPart)) {
                    // TODO: catch invalid link
                    ASSGEN.storeb(TABLE_LINKED_PASSAGES, passageName2id.at(link->getTarget()), 1);
                } else if (Newline* text = dynamic_cast<Newline*>(bodyPart)) {
                    ASSGEN.newline();
                } else if (Print *print = dynamic_cast<Print *>(bodyPart)) {
                    evalExpression(print->getExpression().get());
                    ASSGEN.print_num("sp");
                } else if (IfMacro * ifmacro = dynamic_cast<IfMacro *>(bodyPart)) {
                    //check preceding ifmacro
                    ifContexts.push(makeNextIfContext());
                    //set label for jump to after if block (else, else if, endif)
                    //evaluate expression
                    //make jump to set label if expression is true
                    evalExpression(ifmacro->getExpression().get());
                    ASSGEN.jumpNotEquals(ZAssemblyGenerator::makeArgs({"sp", "1"}), makeIfCaseLabel(ifContexts.top()));
                } else if (ElseIfMacro * elseifmacro = dynamic_cast<ElseIfMacro *>(bodyPart)) {
                    //check preceding ifmacro
                    if(ifContexts.empty()) {
                        throw TweeDocumentException("else if macro encountered without preceding if macro");
                    }

                    //save label for jump to after if/else if block , in this case else
                    //make jump to set label if expression is true
                    //set label for jump to after if block (else, else if, endif)
                    //evaluate expression
                    //make jump to set label if expression is true
                    //else part
                    ASSGEN.jump(makeIfEndLabel(ifContexts.top()));
                    ASSGEN.addLabel(makeIfCaseLabel(ifContexts.top()));
                    ifContexts.top().caseCount++;
                    evalExpression(elseifmacro->getExpression().get());
                    ASSGEN.jumpNotEquals(ZAssemblyGenerator::makeArgs({"sp", "1"}), makeIfCaseLabel(ifContexts.top()));
                } else if (ElseMacro * elsemacro = dynamic_cast<ElseMacro *>(bodyPart)) {
                    //check preceding ifmacro
                    if(ifContexts.empty()) {
                        throw TweeDocumentException("else macro encountered without preceding if macro");
                    }
                    //save label for jump to after if/else if block , in this case else
                    //make jump to set label if expression is true
                    ASSGEN.jump(makeIfEndLabel(ifContexts.top()));
                    ASSGEN.addLabel(makeIfCaseLabel(ifContexts.top()));
                    ifContexts.top().caseCount++;
                } else if (EndIfMacro * endifemacro = dynamic_cast<EndIfMacro *>(bodyPart)) {
                    //check preceding ifmacro
                    if(ifContexts.empty()) {
                        throw TweeDocumentException("endif macro encountered without preceding if macro");
                    }
                    //make jump to set label if expression is trueJumpLabels[ifDepth];

                    ASSGEN.addLabel(makeIfCaseLabel(ifContexts.top()));
                    ASSGEN.addLabel(makeIfEndLabel(ifContexts.top()));

                    ifContexts.pop();
                } else if (SetMacro *op = dynamic_cast<SetMacro *>(bodyPart)) {
                    LOG_DEBUG << "generate SetMacro assembly code";

                    if ( BinaryOperation *binaryOperation = (BinaryOperation *) (op->getExpression().get()) ) {
                        bool isAssignment = binaryOperation->getOperator() == BinOps::TO;
                        if (isAssignment) {
                            evalAssignment(binaryOperation);
                            ASSGEN.pop();
                        }
                    } else {
                        throw TweeDocumentException("set macro didn't contain an assignment");
                    }
                }
            }

            // unclosed if-macro
            if(ifContexts.size() > 0) {
                throw TweeDocumentException("unclosed if macro");
            }

            ASSGEN.ret("0");
        }
    }
}

IfContext TweeCompiler::makeNextIfContext() {
    IfContext context;
    context.number = ifCount++;
    return context;
}

void TweeCompiler::evalAssignment(BinaryOperation *expression) {
    if (expression->getOperator() == BinOps::TO) {
        if (Variable *variable = (Variable *) (expression->getLeftSide().get())) {
            std::string variableName = variable->getName();
            variableName = variableName.erase(0, 1); //remove the $ symbol

            if (!Utils::contains<std::string>(globalVariables, variableName)) {
                globalVariables.insert(variableName);
                ASSGEN.addGlobal(variableName);
            }

            evalExpression(expression->getRightSide().get());

            ASSGEN.load("sp", variableName);
            ASSGEN.push(variableName);
        } else {
            throw TweeDocumentException("left side of set assignment was not a variable");
        }
    }
}

void TweeCompiler::evalExpression(Expression *expression) {

    if (Const<int> *constant = dynamic_cast<Const<int> *>(expression)) {
        ASSGEN.push(std::to_string(constant->getValue()));
    } else if (Const<bool> *constant = dynamic_cast<Const<bool> *>(expression)) {
        ASSGEN.push(std::to_string(constant->getValue()));
    } else if (Variable *variable = dynamic_cast<Variable *>(expression)) {

        std::string prunedVarName = variable->getName().substr(1);

        if (Utils::contains<std::string>(globalVariables, prunedVarName)) {
            ASSGEN.push(prunedVarName);
        } else {
            globalVariables.insert(prunedVarName);
            ASSGEN.addGlobal(prunedVarName);
            ASSGEN.push(prunedVarName);
        }

    } else if (BinaryOperation *binaryOperation = dynamic_cast<BinaryOperation *>(expression)) {
        std::pair<std::string, std::string> labels;

        if (binaryOperation->getOperator() == BinOps::TO) {
            evalAssignment(binaryOperation);
        }

        TweeCompiler::evalExpression(binaryOperation->getLeftSide().get());
        TweeCompiler::evalExpression(binaryOperation->getRightSide().get());

        switch (binaryOperation->getOperator()) {
            case BinOps::ADD:
                ASSGEN.add("sp", "sp", "sp");
                break;
            case BinOps::SUB:
                ASSGEN.sub("sp", "sp", "sp");
                break;
            case BinOps::MUL:
                ASSGEN.mul("sp", "sp", "sp");
                break;
            case BinOps::DIV:
                ASSGEN.div("sp", "sp", "sp");
                break;
            case BinOps::MOD:
                ASSGEN.mod("sp", "sp", "sp");
                break;
            case BinOps::AND:
                ASSGEN.land("sp", "sp", "sp");
                break;
            case BinOps::OR:
                ASSGEN.lor("sp", "sp", "sp");
                break;
            case BinOps::TO:
                // TODO: check if this is right
                ASSGEN.load("sp", "sp");
                break;
            case BinOps::LT:
                labels = makeLabels("lower");
                ASSGEN.jumpLower(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;
            case BinOps::LTE:
                labels = makeLabels("lowerEquals");
                ASSGEN.jumpLowerEquals(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;
            case BinOps::GT:
                labels = makeLabels("greater");
                ASSGEN.jumpGreater(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;
            case BinOps::GTE:
                labels = makeLabels("greaterEquals");
                ASSGEN.jumpGreaterEquals(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;
            case BinOps::IS:
                labels = makeLabels("is");
                ASSGEN.jumpEquals(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;
            case BinOps::NEQ:
                labels = makeLabels("neq");
                ASSGEN.jumpNotEquals(std::string("sp") + " " + std::string("sp"), labels.first);
                ASSGEN.push("0");
                ASSGEN.jump(labels.second);
                ASSGEN.addLabel(labels.first);
                ASSGEN.push("1");
                ASSGEN.addLabel(labels.second);
                break;

        }
    } else if (UnaryOperation *unOp = dynamic_cast<UnaryOperation *>(expression)) {
        TweeCompiler::evalExpression(unOp->getExpression().get());
        switch (unOp->getOperator()) {
            case UnOps::NOT:
                ASSGEN.lnot("sp", "sp");
                break;
            case UnOps::PLUS:
                ASSGEN.push("0");
                ASSGEN.add("sp", "sp", "sp");
                break;
            case UnOps::MINUS:
                ASSGEN.push("0");
                ASSGEN.sub("sp", "sp", "sp");
                break;
            default:
                break;
        }
    }
}

std::pair<std::string, std::string> TweeCompiler::makeLabels(std::string prefix) {

    std::string resultLabel = prefix.append("_");
    std::string afterLabel = "after_";
    resultLabel.append(std::to_string(labelCount));
    afterLabel.append(std::to_string(labelCount));
    ++labelCount;

    return std::make_pair(resultLabel, afterLabel);

}