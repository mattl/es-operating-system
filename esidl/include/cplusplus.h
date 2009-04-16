/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef NINTENDO_ESIDL_CPLUSPLUS_H_INCLUDED
#define NINTENDO_ESIDL_CPLUSPLUS_H_INCLUDED

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include "esidl.h"

class CPlusPlus : public Visitor
{
protected:
    static const int TabWidth = 4;

    std::string indentString;
    std::string prefix;
    FILE* file;
    bool constructorMode;
    std::string asParam;
    int callbackStage;
    int callbackCount;
    int optionalStage;
    int optionalCount;

    std::string moduleName;
    const Node* currentNode;

    int paramCount;  // The number of parameters of the previously evaluated operation
    const ParamDcl* variadicParam;  // Non-NULL if the last parameter of the previously evaluated operation is variadic
    std::map<const ParamDcl*, const OpDcl*> callbacks;

    // param mode saved context.
    // XXX doesn't work if callback takes callbacks as arguments...
    struct
    {
        int callbackStage;
        int callbackCount;
        int optionalStage;
        int optionalCount;
        int paramCount;
        const ParamDcl* variadicParam;
        std::map<const ParamDcl*, const OpDcl*> callbacks;
    } savedContext;

    int getParamCount() const
    {
        return paramCount;
    }

    const ParamDcl* getVariadic() const
    {
        return variadicParam;
    }

    void indent()
    {
        indentString += std::string(TabWidth, ' ');
    }

    void unindent()
    {
        indentString.erase(indentString.length() - TabWidth);
    }

    void write(const char* format, ...)
    {
        va_list ap;
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
    }

    void writetab()
    {
        write("%s", indentString.c_str());
    }

    void writeln(const char* format, ...)
    {
        if (*format)
        {
            writetab();
        }
        va_list ap;
        va_start(ap, format);
        vfprintf(file, format, ap);
        va_end(ap);
        write("\n");
    }

    void printChildren(const Node* node)
    {
        if (node->isLeaf())
        {
            return;
        }

        const Node* saved = currentNode;
        std::string separator;
        bool br;
        int count = 0;
        for (NodeList::iterator i = node->begin(); i != node->end(); ++i)
        {
            if (1 < (*i)->getRank())
            {
                continue;
            }
            if ((*i)->isNative(node->getParent()))
            {
                continue;
            }
            if (0 < count)
            {
                write("%s", separator.c_str());
            }
            separator = (*i)->getSeparator();
            br = (separator[separator.size() - 1] == '\n') ? true : false;
            if (br)
            {
                writetab();
            }
            if (0 < prefix.size())
            {
                write("%s", prefix.c_str());
            }
            currentNode = (*i);
            (*i)->accept(this);
            ++count;
        }
        if (br && 0 < count)
        {
            write("%s", separator.c_str());
        }
        currentNode = saved;
    }

public:
    CPlusPlus(FILE* file) :
        file(file),
        constructorMode(false),
        currentNode(getSpecification()),
        callbackStage(0),
        callbackCount(0),
        paramCount(0),
        variadicParam(0)
    {
    }

    virtual void at(const Node* node)
    {
        if (0 < node->getName().size())
        {
            std::string name = node->getName();
            Node* resolved = resolve(currentNode, name);
            if (resolved)
            {
                name = resolved->getQualifiedName();
                name = getScopedName(moduleName, name);
            }
            name = getInterfaceName(name);
            write("%s", name.c_str());
        }
        else
        {
            printChildren(node);
        }
    }

    virtual void at(const Module* node)
    {
        if (0 < node->getName().size())
        {
            if (node->getJavadoc().size())
            {
                write("%s\n", node->getJavadoc().c_str());
                writetab();
            }
            write("namespace %s\n", node->getName().c_str());
            writeln("{");
            indent();
                moduleName += "::";
                moduleName += node->getName();
                printChildren(node);
                moduleName.erase(moduleName.size() - node->getName().size() - 2);
            unindent();
            writetab();
            write("}");
        }
        else
        {
            printChildren(node);
        }
    }

    virtual void at(const Type* node)
    {
        if (node->getName() == "boolean")
        {
            write("bool");
        }
        else if (node->getName() == "octet")
        {
            write("unsigned char");
        }
        else if (node->getName() == "long")
        {
            write("int");
        }
        else if (node->getName() == "unsigned long")
        {
            write("unsigned int");
        }
        else if (node->getName() == "any")
        {
            write("Any");
        }
        else if (node->getName() == "wchar")
        {
            write("wchar_t");
        }
        else if (node->getName() == "string")
        {
            write("char*");
        }
        else if (node->getName() == "wstring")
        {
            write("wchar_t*");
        }
        else if (node->getName() == "Object")
        {
            if (const char* base = Node::getBaseObjectName())
            {
                write("%s", getScopedName(moduleName, base).c_str());
            }
            else
            {
                write("void");
            }
        }
        else if (node->getName() == "uuid")
        {
            write("Guid&");
        }
        else
        {
            write("%s", node->getName().c_str());
        }
    }

    virtual void at(const SequenceType* node)
    {
        Node* spec = node->getSpec();
        if (spec->isOctet(node->getParent()))
        {
            write("void*");
        }
        else
        {
            spec->accept(this);
            write("*");
        }
    }

    void getter(const Attribute* node)
    {
        static Type replaceable("any");
        std::string cap = node->getName().c_str();
        cap[0] = toupper(cap[0]);   // XXX
        Node* spec = node->getSpec();
        if (node->isReplaceable())
        {
            spec = &replaceable;
        }
        SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));
        std::string name = node->getName();
        size_t pos = name.rfind("::");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 2);
        }
        name[0] = tolower(name[0]); // XXX

        write("virtual ");
        if (seq)
        {
            write("int get%s(", cap.c_str());
            seq->accept(this);
            write(" %s, int %sLength)", name.c_str(), name.c_str());
        }
        else if (spec->isString(node->getParent()) || spec->isWString(node->getParent()))
        {
            write("const ", cap.c_str());
            spec->accept(this);
            write(" get%s(", cap.c_str());
            spec->accept(this);
            write(" %s, int %sLength)", name.c_str(), name.c_str());
        }
        else if (spec->isStruct(node->getParent()))
        {
            write("void get%s(", cap.c_str());
            spec->accept(this);
            write("* %s)", name.c_str());
        }
        else if (spec->isArray(node->getParent()))
        {
            write("void get%s(", cap.c_str());
            spec->accept(this);
            write(" %s)", name.c_str());
        }
        else if (spec->isAny(node->getParent()))
        {
            spec->accept(this);
            write(" get%s(", cap.c_str());
            write("void* %s, int %sLength)", name.c_str(), name.c_str());
        }
        else
        {
            if (spec->isInterface(node->getParent()))
            {
                spec->accept(this);
                write("*");
            }
            else if (NativeType* nativeType = spec->isNative(node->getParent()))
            {
                nativeType->accept(this);
            }
            else
            {
                spec->accept(this);
            }
            write(" get%s()", cap.c_str());
        }
    }

    bool setter(const Attribute* node)
    {
        if (node->isReadonly() && !node->isPutForwards() && !node->isReplaceable())
        {
            return false;
        }

        static Type replaceable("any");
        std::string cap = node->getName().c_str();
        cap[0] = toupper(cap[0]);   // XXX
        Node* spec = node->getSpec();
        if (node->isReplaceable())
        {
            spec = &replaceable;
        }
        else if (node->isPutForwards())
        {
            Interface* target = dynamic_cast<Interface*>(dynamic_cast<ScopedName*>(spec)->search(node->getParent()));
            assert(target);
            Attribute* forwards = dynamic_cast<Attribute*>(target->search(node->getPutForwards()));
            assert(forwards);
            spec = forwards->getSpec();
        }
        SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));
        std::string name = node->getName();
        size_t pos = name.rfind("::");
        if (pos != std::string::npos)
        {
            name = name.substr(pos + 2);
        }
        name[0] = tolower(name[0]); // XXX

        // setter
        write("virtual ");
        if (seq)
        {
            write("int set%s(const ", cap.c_str());
            seq->accept(this);
            write(" %s, int %sLength)", name.c_str(), name.c_str());
        }
        else if (spec->isString(node->getParent()) || spec->isWString(node->getParent()))
        {
            write("void set%s(const ", cap.c_str());
            spec->accept(this);
            write(" %s)", name.c_str());
        }
        else if (spec->isStruct(node->getParent()))
        {
            write("void set%s(const ", cap.c_str());
            spec->accept(this);
            write("* %s)", name.c_str());
        }
        else if (spec->isArray(node->getParent()) || spec->isAny(node->getParent()))
        {
            write("void set%s(const ", cap.c_str());
            spec->accept(this);
            write(" %s)", name.c_str());
        }
        else
        {
            write("void set%s(", cap.c_str());
            if (spec->isInterface(node->getParent()))
            {
                spec->accept(this);
                write("*");
            }
            else if (NativeType* nativeType = spec->isNative(node->getParent()))
            {
                nativeType->accept(this);
            }
            else
            {
                spec->accept(this);
            }
            write(" %s)", name.c_str());
        }
        return true;
    }

    virtual void at(const OpDcl* node)
    {
        callbacks.clear();

        if (asParam == "")
        {
            if (!constructorMode)
            {
                write("virtual ");
            }
            else
            {
                write("static ");
            }
        }

        bool needComma = true;  // true to write "," before the 1st parameter
        Node* spec = node->getSpec();
        SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));
        if (seq)
        {
            std::string name = spec->getName();
            size_t pos = name.rfind("::");
            if (pos != std::string::npos)
            {
                name = name.substr(pos + 2);
            }
            name[0] = tolower(name[0]); // XXX

            write("int");
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", node->getName().c_str());
            }
            seq->accept(this);
            write(" %s, int %sLength", name.c_str(), name.c_str());
        }
        else if (spec->isString(node->getParent()) || spec->isWString(node->getParent()))
        {
            std::string name = spec->getName();
            size_t pos = name.rfind("::");
            if (pos != std::string::npos)
            {
                name = name.substr(pos + 2);
            }
            name[0] = tolower(name[0]); // XXX

            write("const ");
            spec->accept(this);
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", node->getName().c_str());
            }
            spec->accept(this);
            write(" %s, int %sLength", name.c_str(), name.c_str());
        }
        else if (spec->isStruct(node->getParent()))
        {
            std::string name = spec->getName();
            size_t pos = name.rfind("::");
            if (pos != std::string::npos)
            {
                name = name.substr(pos + 2);
            }
            name[0] = tolower(name[0]); // XXX

            write("void");
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", asParam.c_str());
            }
            spec->accept(this);
            write("* %s", name.c_str());
        }
        else if (spec->isArray(node->getParent()))
        {
            std::string name = spec->getName();
            size_t pos = name.rfind("::");
            if (pos != std::string::npos)
            {
                name = name.substr(pos + 2);
            }
            name[0] = tolower(name[0]); // XXX

            write("void");
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", node->getName().c_str());
            }
            spec->accept(this);
            write(" %s", name.c_str());
        }
        else if (spec->isAny(node->getParent()))
        {
            std::string name = spec->getName();
            size_t pos = name.rfind("::");
            if (pos != std::string::npos)
            {
                name = name.substr(pos + 2);
            }
            name[0] = tolower(name[0]); // XXX

            spec->accept(this);
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", asParam.c_str());
            }
            write("void* %s, int %sLength", name.c_str(), name.c_str());
        }
        else
        {
            if (spec->isInterface(node->getParent()))
            {
                spec->accept(this);
                write("*");
            }
            else if (NativeType* nativeType = spec->isNative(node->getParent()))
            {
                nativeType->accept(this);
            }
            else
            {
                spec->accept(this);
            }
            if (asParam == "")
            {
                write(" %s(", node->getName().c_str());
            }
            else
            {
                write(" (*%s)(", asParam.c_str());
            }
            needComma = false;
        }

        paramCount = 0;
        variadicParam = 0;
        for (NodeList::iterator i = node->begin(); i != node->end(); ++i)
        {
            ParamDcl* param = dynamic_cast<ParamDcl*>(*i);
            assert(param);
            if (asParam == "" &&
                (param->isOptional() || param->isVariadic()))
            {
                ++optionalCount;
                if (optionalStage < optionalCount)
                {
                    break;
                }
            }
            if (needComma || i != node->begin())
            {
                write(", ");
            }
            ++paramCount;
            param->accept(this);
        }

        write(")");
        if (node->getRaises())
        {
            write(" throw(");
            node->getRaises()->accept(this);
            write(")");
        }
    }

    virtual void at(const ParamDcl* node)
    {
        static SequenceType variadicSequence(0);

        Node* spec = node->getSpec();
        SequenceType* seq = const_cast<SequenceType*>(spec->isSequence(node->getParent()));
        if (node->isVariadic())
        {
            variadicParam = node;
            variadicSequence.setSpec(spec);
            seq = &variadicSequence;
        }

        if (node->isInput())
        {
            if (seq && !seq->getSpec()->isInterface(currentNode) ||
                spec->isGuid(node->getParent()) ||
                spec->isString(node->getParent()) ||
                spec->isWString(node->getParent()) ||
                spec->isStruct(node->getParent()) ||
                spec->isArray(node->getParent()))
            {
                write("const ");
            }
        }

        if (seq)
        {
            seq->accept(this);
            write(" %s, int %sLength", node->getName().c_str() , node->getName().c_str());
        }
        else if (spec->isStruct(node->getParent()))
        {
            spec->accept(this);
            write("* %s", node->getName().c_str());
        }
        else if (spec->isArray(node->getParent()))
        {
            spec->accept(this);
            write(" %s", node->getName().c_str());
        }
        else
        {
            if (spec->isInterface(node->getParent()))
            {
                Interface* callback = dynamic_cast<Interface*>(dynamic_cast<ScopedName*>(spec)->search(node->getParent()));
                uint32_t attr;
                if (callback && (attr = callback->isCallback()) != 0)
                {
                    bool function;
                    // In C++, acccept callback interface pointer even if [Callback=FunctionOnly].
                    if (attr == Interface::Callback ||
                        attr == Interface::CallbackIsFunctionOnly)
                    {
                        function = (1u << callbackCount) & callbackStage;
                        ++callbackCount;
                    }
                    if (function)
                    {
                        OpDcl* op = 0;
                        paramMode(node->getName());
                        for (NodeList::iterator i = callback->begin(); i != callback->end(); ++i)
                        {
                            if (op = dynamic_cast<OpDcl*>(*i))
                            {
                                CPlusPlus::at(op);
                                break;
                            }
                        }
                        paramMode();
                        if (op)
                        {
                            callbacks.insert(std::pair<const ParamDcl*, const OpDcl*>(node, op));
                        }
                        return;
                    }
                }
                spec->accept(this);
                write("*");
            }
            else if (NativeType* nativeType = spec->isNative(node->getParent()))
            {
                nativeType->accept(this);
            }
            else
            {
                spec->accept(this);
            }
            if (!spec->isString(node->getParent()) &&
                !spec->isWString(node->getParent()))
            {
                if (!node->isInput())
                {
                    write("*");
                }
                write(" %s", node->getName().c_str());
            }
            else
            {
                write(" %s", node->getName().c_str());
                if (!node->isInput())
                {
                    write(", int %sLength", node->getName().c_str());
                }
            }
        }
    }

    virtual void at(const Include* node)
    {
    }

    static std::string getInterfaceName(std::string qualifiedName)
    {
        if (qualifiedName == "Object")
        {
            if (const char* base = Node::getBaseObjectName())
            {
                qualifiedName = base;
            }
            else
            {
                qualifiedName = "void";
            }
        }
        return qualifiedName;
    }

    const OpDcl* isCallback(const ParamDcl* param) const
    {
        std::map<const ParamDcl*, const OpDcl*>::const_iterator i = callbacks.find(param);
        if (i == callbacks.end())
        {
            return 0;
        }
        return i->second;
    }

    void paramMode(const std::string name)
    {
        asParam = name;
        savedContext.callbackStage = callbackStage;
        savedContext.callbackCount = callbackCount;
        savedContext.optionalStage = optionalStage;
        savedContext.optionalCount = optionalCount;
        savedContext.paramCount = paramCount;
        savedContext.variadicParam = variadicParam;
        savedContext.callbacks = callbacks;
    }

    void paramMode()
    {
        asParam = "";
        callbackStage = savedContext.callbackStage;
        callbackCount = savedContext.callbackCount;
        optionalStage = savedContext.optionalStage;
        optionalCount = savedContext.optionalCount;
        paramCount = savedContext.paramCount;
        variadicParam = savedContext.variadicParam;
        callbacks = savedContext.callbacks;
    }
};

#endif  // NINTENDO_ESIDL_CPLUSPLUS_H_INCLUDED
