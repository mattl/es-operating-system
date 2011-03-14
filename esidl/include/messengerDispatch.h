/*
 * Copyright 2010, 2011 Esrille Inc.
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

#ifndef ESIDL_MESSENGER_DISPATCH_H_INCLUDED
#define ESIDL_MESSENGER_DISPATCH_H_INCLUDED

#include "messenger.h"

#include <map>

class MessengerDispatch : public Messenger
{
    bool overloaded;
public:
    MessengerDispatch(Formatter* formatter, const std::string& stringTypeName, const std::string& objectTypeName, bool useExceptions) :
        Messenger(formatter, stringTypeName, objectTypeName, useExceptions),
        overloaded(false)
    {
        currentNode = 0;
    }

    virtual void at(const Member* node)
    {
    }

    virtual void at(const ConstDcl* node)
    {
        writeln("case 0x%x:", node->getHash());
        indent();
            writeln("if (argumentCount == GETTER_)");
            indent();
                writeln("return %s;", node->getName().c_str());
            unindent();
            writeln("if (argumentCount == HAS_PROPERTY_)");
            indent();
                writeln("return true;");
            unindent();
        unindent();
    }

    virtual void at(const Interface* node)
    {
        if (!currentNode)
        {
            currentNode = node->getParent();
            prefixedModuleName = currentNode->getPrefixedModuleName();
        }
        assert(!(node->getAttr() & Interface::Supplemental) && !node->isLeaf());

        unsigned interfaceNumber;
        std::list<const Interface*> list;
        node->getInterfaceList(&list);
        std::list<const Interface*> mixinList;
        node->collectMixins(&mixinList);

        std::multimap<uint32_t, OpDcl*> operations;

        writeln("template <class IMP>");
        writeln("static Any dispatch(IMP* self, unsigned selector, const char* id, int argumentCount, Any* arguments) {");
        {
            writeln("switch (selector) {");
            unindent();
            std::list<const Interface*> interfaceList;
            node->collectSupplementals(&interfaceList);
            for (std::list<const Interface*>::const_iterator i = interfaceList.begin();
                i != interfaceList.end();
                ++i)
            {
                const Node* saved = currentNode;
                currentNode = *i;
                for (NodeList::iterator j = (*i)->begin(); j != (*i)->end(); ++j)
                {
                    if (OpDcl* op = dynamic_cast<OpDcl*>(*j))
                    {
                        operations.insert(std::pair<uint32_t, OpDcl*>(op->getHash(), op));
                        continue;
                    }
                    if (Attribute* attr = dynamic_cast<Attribute*>(*j))
                    {
                         attr->accept(this);
                    }
                    if (ConstDcl* decl = dynamic_cast<ConstDcl*>(*j))
                    {
                         decl->accept(this);
                    }
                }
                currentNode = saved;
            }
            for (std::multimap<uint32_t, OpDcl*>::iterator i = operations.begin(); i != operations.end(); ++i)
            {
                writeln("case 0x%x:", i->first);
                indent();
                int count = operations.count(i->first);
                if (count == 1)
                {
                    // Process non-overloaded operations
                    overloaded = false;
                    const Node* saved = currentNode;
                    currentNode = i->second->getParent();
                    visitInterfaceElement(dynamic_cast<const Interface*>(currentNode), i->second);
                    currentNode = saved;
                }
                else
                {
                    // Process overloaded operations
                    overloaded = true;
                    for (; 0 < count; --count, ++i) {
                        OpDcl* op = i->second;
                        const Node* saved = currentNode;
                        currentNode = op->getParent();
                        visitInterfaceElement(dynamic_cast<const Interface*>(currentNode), op);
                        currentNode = saved;
                    };
                    --i;
                }
                writeln("if (argumentCount == HAS_OPERATION_)");
                indent();
                    writeln("return true;");
                unindent();
                writeln("return Any();");
                unindent();
            }

            writeln("default:");
            indent();
                if (node->getExtends())
                {
                    NodeList::iterator i = node->getExtends()->begin();
                    if ((*i)->isBaseObject())
                    {
                        writeln("return Any();");  // TODO:  should raise an exception?
                    }
                    else
                    {
                        writetab();
                        write("return ");
                        (*i)->accept(this);
                        write("::dispatch(self, selector, id, argumentCount, arguments);\n");
                    }
                }
                else
                {
                    writeln("return Any();");  // TODO:  should raise an exception?
                }
            writeln("}");
        }
        writeln("}");

        if (!mixinList.empty())
        {
            writeln("");
            writeln("Any %s_Mixin::call(unsigned interfaceNumber, unsigned selector, unsigned argumentCount, Any* arguments) {",
                    getEscapedName(getClassName(node)).c_str());

                writeln("switch (interfaceNumber) {");
                interfaceNumber = 0;
                for (std::list<const Interface*>::const_iterator i = list.begin();
                    i != list.end();
                    ++i, ++interfaceNumber)
                {
                    unindent();
                    writeln("case %u:", interfaceNumber);
                    indent();
                    writeln("return static_cast<%s*>(this)->call(selector, argumentCount, arguments);",
                            getScopedName(prefixedModuleName, (*i)->getPrefixedName()).c_str());
                }
                unindent();
                writeln("default:");
                indent();
                    writeln("return Any();");  // TODO:  should raise an exception?
                writeln("}");
            writeln("}");
        }
    }

    virtual void at(const Attribute* node)
    {
        std::string cap = node->getName().c_str();
        cap[0] = toupper(cap[0]);   // XXX

        static Type replaceable("any");
        Node* spec = node->getSpec();
        if (node->isReplaceable())
        {
            spec = &replaceable;
        }
        std::string name = getBufferName(node);

        // getter
        writeln("case 0x%x:", node->getHash());
        indent();
            writeln("if (argumentCount == GETTER_)");
            indent();
                writeln("return self->get%s();", cap.c_str());
            unindent();


        // setter
        if (!node->isReadonly() || node->isPutForwards() || node->isReplaceable()) {
            if (node->isPutForwards())
            {
                Interface* target = dynamic_cast<Interface*>(dynamic_cast<ScopedName*>(spec)->search(node->getParent()));
                assert(target);
                Attribute* forwards = dynamic_cast<Attribute*>(target->search(node->getPutForwards()));
                assert(forwards);
                spec = forwards->getSpec();
            }
            writeln("if (argumentCount == SETTER_) {");
                writetab();
                write("self->set%s(", cap.c_str());
                if (spec->isAny(node->getParent()))
                {
                    write("arguments[0]");
                }
                else if (spec->isString(node->getParent()))
                {
                    write("arguments[0].toString()");
                }
                else if (spec->isInterface(node->getParent()))
                {
                    write("arguments[0].toObject()");
                }
                else
                {
                    write("static_cast<");
                    spec->accept(this);
                    write(" >(arguments[0])");
                }
                write(");\n");
                writeln("return Any();");
            writeln("}");
        }

        writeln("if (argumentCount == HAS_PROPERTY_)");
        indent();
            writeln("return true;");
        unindent();

        writeln("return Any();");
        unindent();
    }

    virtual void at(const OpDcl* node)
    {
        Interface* interface = dynamic_cast<Interface*>(node->getParent());
        assert(interface);
        int paramCount = getParamCount(node);

        writetab();
        switch (node->getAttr() & OpDcl::IndexMask)
        {
        case OpDcl::IndexCreator:
            write("if (argumentCount == SPECIAL_CREATOR_");
            break;
        case OpDcl::IndexDeleter:
            write("if (argumentCount == SPECIAL_DELETER_");
            break;
        case OpDcl::IndexGetter:
            write("if (argumentCount == SPECIAL_GETTER_");
            break;
        case OpDcl::IndexSetter:
            write("if (argumentCount == SPECIAL_SETTER_");
            break;
        case OpDcl::IndexSetter | OpDcl::IndexCreator:
            write("if (argumentCount == SPECIAL_SETTER_CREATOR_");
            break;
        case 0:
            if (!getVariadic())
            {
                write("if (argumentCount == %d", paramCount);
            }
            else
            {
                --paramCount;
                write("if (%d <= argumentCount", paramCount);
            }
            break;
        default:
            node->check(node->getAttr(), "%s has invalid specials", node->getName().c_str(), node->getAttr());
            break;
        }

        if (overloaded)
        {
            unsigned n = 0;
            for (NodeList::iterator i = node->begin(); n < paramCount && i != node->end(); ++i, ++n)
            {
                ParamDcl* param = dynamic_cast<ParamDcl*>(*i);
                assert(param);
                if (param->getSpec()->isInterface(node->getParent()))
                {
                    write(" &&\n");
                    writetab();
                    write("    arguments[%u].isObject()", n);
                }
                else
                {
                    // TODO:
                    write(" &&\n");
                    writetab();
                    write("    !arguments[%u].isObject()", n);
                }
            }
        }
        write(") {\n");

            writetab();
            Node* spec = node->getSpec();
            if (!spec->isVoid(interface))
            {
                write("return ");
            }
            write("self->%s(", getEscapedName(node->getName()).c_str());
            writeParameters(node);
            write(");\n");
            if (spec->isVoid(interface))
            {
                writeln("return Any();");
            }

        writeln("}");
    }

    virtual void at(const ParamDcl* node)
    {
        Node* spec = node->getSpec();
        if (node->isVariadic())
        {
            write("Variadic<");
            spec->accept(this);
            write(" >(arguments + %d, argumentCount - %d)", getParamCount() - 1, getParamCount() - 1);
        }
        else if (spec->isAny(node->getParent()))
        {
            write("arguments[%u]", getParamCount() - 1);
        }
        else if (spec->isString(node->getParent()))
        {
            write("arguments[%u].toString()", getParamCount() - 1);
        }
        else if (spec->isInterface(node->getParent()))
        {
            write("arguments[%u].toObject()", getParamCount() - 1);
        }
        else
        {
            write("static_cast<");
            spec->accept(this);
            write(" >(arguments[%u])", getParamCount() - 1);
        }
     }
 };

#endif  // ESIDL_MESSENGER_DISPATCH_H_INCLUDEDx
