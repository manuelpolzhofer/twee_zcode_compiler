//
// Created by Danni on 09.06.15.
//

#include "TweeCompiler.h"
#include "ZAssemblyGenerator.h"
#include "exceptions.h"
#include "Utils.h"

#include <sstream>
#include <iostream>
#include <set>

#include <Passage/Body/Link.h>
#include <Passage/Body/Text.h>

#include <Passage/Body/Expressions/BinaryOperation.h>
#include <Passage/Body/Expressions/Const.h>
#include <Passage/Body/Expressions/UnaryOperation.h>
#include <Passage/Body/Expressions/Variable.h>

#include <Passage/Body/Macros/Print.h>
#include <Passage/Body/Macros/SetMacro.h>

using namespace std;

static const string PASSAGE_GLOB = "PASSAGE_PTR",
        JUMP_TABLE_LABEL = "JUMP_TABLE_START",
        JUMP_TABLE_END_LABEL = "JUMP_TABLE_END",
        MAIN_ROUTINE = "main",
        USER_INPUT = "USER_INPUT",
        READ_BEGIN = "READ_BEGIN";

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


void TweeCompiler::compile(TweeFile &tweeFile, std::ostream &out) {
    _assgen = unique_ptr<ZAssemblyGenerator>(new ZAssemblyGenerator(out));

    vector<Passage> passages = tweeFile.getPassages();

    globalVariables = std::set<std::string>();

    {
        int i = 0;
        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            passageName2id[passage->getHead().getName()] = i;
            i++;
        }
    }

    // main routine
    {
        // globals
        ASSGEN.addGlobal(PASSAGE_GLOB)
                .addGlobal(USER_INPUT);

        // call start routine first
        ASSGEN.addRoutine(MAIN_ROUTINE)
                .markStart()
                .call(routineNameForPassageName("start"), PASSAGE_GLOB)
                .addLabel(JUMP_TABLE_LABEL);

        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            int passageId = passageName2id.at(passage->getHead().getName());

            ASSGEN.jumpEquals(ZAssemblyGenerator::makeArgs({std::to_string(passageId), PASSAGE_GLOB}),
                              labelForPassage(*passage));
        }

        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            ASSGEN.addLabel(labelForPassage(*passage))
                    .call(routineNameForPassage(*passage), PASSAGE_GLOB)
                    .jump(JUMP_TABLE_LABEL);
        }

        ASSGEN.addLabel(JUMP_TABLE_END_LABEL);

        ASSGEN.quit();
    }

    // passage routines
    {
        for (auto passage = passages.begin(); passage != passages.end(); ++passage) {
            const vector<unique_ptr<BodyPart>> &bodyParts = passage->getBody().getBodyParts();

            // declare passage routine
            ASSGEN.addRoutine(routineNameForPassage(*passage));

            ASSGEN.println(string("***** ") + passage->getHead().getName() + string(" *****"));

            //  print passage contents
            for (auto it = bodyParts.begin(); it != bodyParts.end(); it++) {
                BodyPart *bodyPart = it->get();
                if (Text *text = dynamic_cast<Text *>(bodyPart)) {
                    ASSGEN.print(text->getContent());
                }
                if (Print *print = dynamic_cast<Print *>(bodyPart)) {
                    evalExpression(print->getExpression().get());
                    ASSGEN.print_num("sp");
                }

            }

            ASSGEN.newline();

            vector<Link *> links;
            // get links from passage
            for (auto it = bodyParts.begin(); it != bodyParts.end(); ++it) {
                BodyPart *bodyPart = it->get();
                if (Link *link = dynamic_cast<Link *>(bodyPart)) {
                    links.push_back(link);
                }
            }

            // present choices to user
            ASSGEN.println("Select one of the following options");
            int i = 1;
            for (auto link = links.begin(); link != links.end(); link++) {
                ASSGEN.println(string("    ") + to_string(i) + string(") ") + (*link)->getTarget());
                i++;
            }

            ASSGEN.addLabel(READ_BEGIN);

            // read user input
            ASSGEN.read_char(USER_INPUT);

            // jump to according link selection
            i = 0;
            for (auto link = links.begin(); link != links.end(); link++) {
                string label = string("L") + to_string(i);
                ASSGEN.jumpEquals(ZAssemblyGenerator::makeArgs({to_string(ZSCII_NUM_OFFSET + i), USER_INPUT}), label);

                i++;
            }

            // no proper selection was made
            ASSGEN.jump(READ_BEGIN);

            i = 0;
            for (auto link = links.begin(); link != links.end(); link++) {
                string label = string("L") + to_string(i);
                try {
                    int targetPassageId = passageName2id.at((*link)->getTarget());
                    ASSGEN.addLabel(label);
#ifdef ZAS_DEBUG
                    ASSGEN.print(string("selected ") + to_string(targetPassageId) );
                    #endif
                    ASSGEN.ret(to_string(targetPassageId));
                } catch (const out_of_range &err) {
                    cerr << "could not find passage for link target \"" << (*link)->getTarget() << "\"" << endl;
                    throw TweeDocumentException();
                }
                i++;
            }
        }
    }
}

void TweeCompiler::evalExpression(Expression *expression) {

    if (Const<int> *constant = dynamic_cast<Const<int> *>(expression)) {

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

    } else if (BinaryOperation *binOp = dynamic_cast<BinaryOperation *>(expression)) {

        TweeCompiler::evalExpression(binOp->getLeftSide().get());
        TweeCompiler::evalExpression(binOp->getRightSide().get());

        switch (binOp->getOperator()) {
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
                ASSGEN.store("sp", "sp");
                break;

        }
    } else if (UnaryOperation *unOp = dynamic_cast<UnaryOperation *>(expression)) {
        TweeCompiler::evalExpression(unOp->getExpression().get());
        switch (unOp->getOperator()) {
            case UnOps::NOT:
                break;
            case UnOps::PLUS:
                ASSGEN.push("0");
                ASSGEN.add("sp", "sp", "sp");
                break;
            case UnOps::MINUS:
                break;
                ASSGEN.push("0");
                ASSGEN.sub("sp", "sp", "sp");
            default:
                break;
        }
    } /*else if (Function *func = dynamic_cast<Function *>(expression)) {
        for(auto iter = func->getArgs().begin; iter != func->getArgs().begin; ++iter) {
            TweeCompiler::evalExpression()
        }


    }*/
}
