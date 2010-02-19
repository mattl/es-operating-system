/*
 * Copyright 2008-2010 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ESIDL_CPLUSPLUS_TEMPLATE_H_INCLUDED
#define ESIDL_CPLUSPLUS_TEMPLATE_H_INCLUDED

#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <set>
#include <vector>
#include "cplusplus.h"
#include "info.h"

class CPlusPlusTemplate : public CPlusPlus
{
    unsigned methodNumber;

    int writeInvoke(const Node* node, Node* spec, SequenceType* seq, const Node* nameNode)
    {
        bool hasBuffer = false;
        bool hasLength = false;
        int hasCast = 0;

        writetab();
        if (seq)
        {
            write("return static_cast<int>(invoke(this");
            hasBuffer = true;
            if (!seq->getMax())
            {
                hasLength = true;
            }
            hasCast = 1;
        }
        else if (spec->isVoid(node))
        {
            write("invoke(this");
        }
        else if (spec->isAny(node))
        {
            write("return invoke(this");
            hasBuffer = hasLength = true;
        }
        else if (spec->isInterface(node))
        {
            write("return dynamic_cast<");
            spec->accept(this);
            write("*>(static_cast<Object*>(invoke(this");
            hasCast = 2;
        }
        else
        {
            write("return static_cast<");
            spec->accept(this);
            write(" >(invoke(this");
            hasCast = 1;
        }

        write(", I, %u", methodNumber);

        if (!hasCustomStringType() && spec->isString(node->getParent()))
        {
            hasBuffer = hasLength = true;
        }
        if (hasBuffer)
        {
            write(", %s", getBufferName(nameNode).c_str());
        }
        if (hasLength)
        {
            write(", %sLength", getBufferName(nameNode).c_str());
        }

        ++methodNumber;

        return hasCast;
    }

public:
    CPlusPlusTemplate(const char* source, FILE* file, const char* indent = "es") :
        CPlusPlus(source, file, "std::string", "Object", true, indent)
    {
        currentNode = 0;
    }

    CPlusPlusTemplate(const char* source, const Formatter* formattter) :
        CPlusPlus(source, formattter, "std::string", "Object", true)
    {
        currentNode = 0;
    }

    virtual void at(const Member* node)
    {
        // To avoid constants
    }

    virtual void at(const Interface* node)
    {
        Interface* constructor = node->getConstructor();

        if (!currentNode)
        {
            currentNode = node->getParent();
            qualifiedModuleName = currentNode->getQualifiedModuleName();
        }
        assert(!(node->getAttr() & Interface::Supplemental) && !node->isLeaf());

        const std::list<const Interface*>* implementList = node->getImplements();

        // Mixin interface
        if (!implementList->empty())
        {
            write("class %s_Mixin : public %s",
                  getEscapedName(node->getName()).c_str(),
                  getEscapedName(node->getName()).c_str());
            for (std::list<const Interface*>::const_iterator i = implementList->begin();
                i != implementList->end();
                ++i)
            {
                std::string name = (*i)->getQualifiedName();
                name = getInterfaceName(name);
                name = getScopedName(qualifiedModuleName, name);
                write(", public %s", name.c_str());
            }
            writeln("{");
            writeln("};");
            writeln("");
        }

        // Bridge template
        writeln("template<Any (*invoke)(Object*, unsigned, unsigned, ...), class B = %s%s, unsigned I = 0>",
                getEscapedName(node->getName()).c_str(),
                implementList->empty() ? "" : "_Mixin");
        write("class %s_Bridge : ", getEscapedName(node->getName()).c_str());
        int baseCount = 0;
        
        std::list<const Interface*> bridgeList;
        for (std::list<const Interface*>::const_reverse_iterator i = implementList->rbegin();
             i != implementList->rend();
             ++i)
        {
            (*i)->collectMixins(&bridgeList);
        }
        node->collectMixins(&bridgeList);
        
        write("public ");
        for (std::list<const Interface*>::const_iterator i = bridgeList.begin();
             i != bridgeList.end();
             ++i)
        {
            if (*i == node || ((*i)->getAttr() & Interface::Supplemental))
            {
                continue;
            }
            std::string name = (*i)->getQualifiedName();
            name = getInterfaceName(name);
            name = getScopedName(qualifiedModuleName, name);
            write("%s_Bridge<invoke, ", name.c_str());
            ++baseCount;
        }
        write("B");
        for (int i = 0; i < baseCount; ++i)
        {
            write(", I + %u>", baseCount - i - 1);
        }

        write(" {\n");
        unindent();
        writeln("public:");
        indent();

        std::list<const Interface*> interfaceList;
        node->collectMixins(&interfaceList, node);
        for (std::list<const Interface*>::const_iterator i = interfaceList.begin();
             i != interfaceList.end();
             ++i)
        {
            methodNumber = 0;
            writeln("// %s", (*i)->getName().c_str());
            const Node* saved = currentNode;
            currentNode = *i;
            for (NodeList::iterator j = (*i)->begin(); j != (*i)->end(); ++j)
            {
                visitInterfaceElement(*i, *j);
            }
            currentNode = saved;
        }

        writeln("};");
    }

    virtual void at(const Attribute* node)
    {
        // getter
        static Type replaceable("any");
        Node* spec = node->getSpec();
        if (node->isReplaceable())
        {
            spec = &replaceable;
        }
        SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));
        std::string name = getBufferName(node);

        writetab();
        CPlusPlus::getter(node);
        writeln("{");
            int hasCast = writeInvoke(node, spec, seq, node);
            while (0 < hasCast--)
            {
                write(")");
            }
            write(");\n");
        writeln("}");

        if (node->isReadonly() && !node->isPutForwards() && !node->isReplaceable())
        {
            return;
        }

        // setter
        if (node->isPutForwards())
        {
            Interface* target = dynamic_cast<Interface*>(dynamic_cast<ScopedName*>(spec)->search(node->getParent()));
            assert(target);
            Attribute* forwards = dynamic_cast<Attribute*>(target->search(node->getPutForwards()));
            assert(forwards);
            spec = forwards->getSpec();
        }
        seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));

        writetab();
        CPlusPlus::setter(node);
        writeln("{");
            writetab();
            if (seq)
            {
                write("return ");
            }
            write("invoke(this, I, %u, %s", methodNumber, name.c_str());
            if (seq && !seq->getMax())
            {
                write(", %sLength)", name.c_str());
            }
            write(");\n");
        writeln("}");

        ++methodNumber;
    }

    virtual void at(const OpDcl* node)
    {
        Interface* interface = dynamic_cast<Interface*>(node->getParent());
        assert(interface);

        writetab();
        CPlusPlus::at(node);
        writeln("{");

            // Invoke
            Node* spec = node->getSpec();
            SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));

            int hasCast = writeInvoke(node, spec, seq, spec);

            NodeList::iterator it = node->begin();
            for (int i = 0; i < getParamCount(); ++i, ++it)
            {
                ParamDcl* param = static_cast<ParamDcl*>(*it);
                write(", %s", getEscapedName(param->getName()).c_str());
                SequenceType* seq = const_cast<SequenceType*>(param->getSpec()->isSequence(node));
                if (param->isVariadic() || seq && !seq->getMax())
                {
                    write(", %sLength", getEscapedName(param->getName()).c_str());
                }
            }

            while (0 < hasCast--)
            {
                write(")");
            }
            write(");\n");

        writeln("}");
    }
};

#endif  // ESIDL_CPLUSPLUS_TEMPLATE_H_INCLUDED