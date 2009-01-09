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

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "esidl.h"

class Print : public Visitor
{
    std::string indent;

    void printChildren(const Node* node)
    {
        if (node->isLeaf())
        {
            return;
        }

        std::string separator;
        bool br;
        int count = 0;
        for (NodeList::iterator i = node->begin(); i != node->end(); ++i)
        {
            if (0 < count)
            {
                printf("%s", separator.c_str());
            }
            separator = (*i)->getSeparator();
            br = (separator[separator.size() - 1] == '\n') ? true : false;
            if (br)
            {
                printf("%s", indent.c_str());
            }
            (*i)->accept(this);
            ++count;
        }
        if (0 < count && br)
        {
            printf("%s", separator.c_str());
        }
    }

    void printExtendedAttributes(const Node* node)
    {
        if (!node->hasExtendedAttributes())
        {
            return;
        }

        const NodeList* attrlist = node->getExtendedAttributes();
        std::string separator;
        int count = 0;
        printf("[");
        for (NodeList::const_iterator i = attrlist->begin();
             i != attrlist->end();
             ++i)
        {
            if (0 < count)
            {
                printf("%s", separator.c_str());
            }
            separator = (*i)->getSeparator();
            (*i)->accept(this);
            ++count;
        }
        printf("] ", count);
    }
public:
    virtual void at(const Node* node)
    {
        if (0 < node->getName().size())
        {
            printf("%s", node->getName().c_str());
        }
        else
        {
            printChildren(node);
        }
    }

    virtual void at(const Module* node)
    {
        if (node->hasExtendedAttributes())
        {
            printExtendedAttributes(node);
        }
        if (0 < node->getName().size())
        {
            printf("module %s // countCount: %d, moduleCount: %d\n",
                   node->getName().c_str(), node->getConstCount(), node->getModuleCount());
            printf("%s{\n", indent.c_str());
            indent += "    ";
            printChildren(node);
            indent.erase(indent.length() - 4);
            printf("%s};", indent.c_str());
        }
        else
        {
            printChildren(node);
        }
    }

    virtual void at(const EnumType* node)
    {
        printf("enum %s", node->getName().c_str());
        printf("\n%s{\n", indent.c_str());
        indent += "    ";
        printChildren(node);
        indent.erase(indent.length() - 4);
        printf("%s}", indent.c_str());
    }

    virtual void at(const StructType* node)
    {
        printf("struct %s", node->getName().c_str());
        if (!node->isLeaf())
        {
            printf("\n%s{  // memberCount: %d\n", indent.c_str(), node->getMemberCount());
            indent += "    ";
            printChildren(node);
            indent.erase(indent.length() - 4);
            printf("%s}", indent.c_str());
        }
    }

    virtual void at(const ExceptDcl* node)
    {
        printf("exception %s", node->getName().c_str());
        printf("\n%s{  // memberCount: %d\n", indent.c_str(), node->getMemberCount());
        indent += "    ";
        printChildren(node);
        indent.erase(indent.length() - 4);
        printf("%s}", indent.c_str());
    }

    virtual void at(const Interface* node)
    {
        if (node->hasExtendedAttributes())
        {
            printExtendedAttributes(node);
        }
        printf("interface %s", node->getName().c_str());
        if (node->getExtends())
        {
            printf(" : ");
            node->getExtends()->accept(this);
        }

        printf(" // countCount: %d, methodCount %d, rank %d",
               node->getConstCount(), node->getMethodCount(), node->getRank());

        if (!node->isLeaf())
        {
            printf("\n%s{\n", indent.c_str());
            indent += "    ";
            printChildren(node);
            indent.erase(indent.length() - 4);
            printf("%s}", indent.c_str());
        }
    }

    virtual void at(const SequenceType* node)
    {
        printf("sequence<");
        node->getSpec()->accept(this);
        if (node->getMax())
        {
            printf(", ");
            node->getMax()->accept(this);
        }
        else if (dynamic_cast<SequenceType*>(node->getSpec()))  // avoid >>
        {
            printf(" ");
        }
        printf(">");
    }

    virtual void at(const BinaryExpr* node)
    {
        node->getLeft()->accept(this);
        printf(" %s ", node->getName().c_str());
        node->getRight()->accept(this);
    }

    virtual void at(const UnaryExpr* node)
    {
        printf("%s", node->getName().c_str());
        NodeList::iterator elem = node->begin();
        (*elem)->accept(this);
    }

    virtual void at(const GroupingExpression* node)
    {
        printf("(");
        NodeList::iterator elem = node->begin();
        (*elem)->accept(this);
        printf(")");
    }

    virtual void at(const Literal* node)
    {
        at(static_cast<const Node*>(node));
    }

    virtual void at(const PragmaID* node)
    {
        printf("#pragma ID ");
        node->getScopedName()->accept(this);
        printf(" %s", node->getID().c_str());
    }

    virtual void at(const Member* node)
    {
        if (node->isTypedef())
        {
            printf("typedef ");
        }
        node->getSpec()->accept(this);
        printf(" %s", node->getName().c_str());
    }

    virtual void at(const ArrayDcl* node)
    {
        assert(!node->isLeaf());

        at(static_cast<const Member*>(node));
        for (NodeList::iterator i = node->begin(); i != node->end(); ++i)
        {
            printf("[");
            (*i)->accept(this);
            printf("]");
        }
    }

    virtual void at(const Attribute* node)
    {
        if (node->hasExtendedAttributes())
        {
            printExtendedAttributes(node);
        }
        if (node->isReadonly())
        {
            printf("readonly ");
        }
        printf("attribute ");
        at(static_cast<const Member*>(node));
    }

    virtual void at(const ConstDcl* node)
    {
        printf("const ");
        at(static_cast<const Member*>(node));
        printf(" = ");
        node->getExp()->accept(this);
    }

    virtual void at(const OpDcl* node)
    {
        if (node->hasExtendedAttributes())
        {
            printExtendedAttributes(node);
        }
        at(static_cast<const Member*>(node));
        printf("(");
        printChildren(node);
        printf(")");
        if (node->getRaises())
        {
            printf(" raises(");
            node->getRaises()->accept(this);
            printf(")");
        }
        printf(" /* paramCount: %d */", node->getParamCount());
    }

    virtual void at(const ParamDcl* node)
    {
        switch (node->getAttr())
        {
        case ParamDcl::In:
            printf("in ");
            break;
        case ParamDcl::Out:
            printf("out ");
            break;
        case ParamDcl::InOut:
            printf("inout ");
            break;
        }
        at(static_cast<const Member*>(node));
    }

    virtual void at(const Include* node)
    {
        printf("# 1 \"%s\" 1", node->getName().c_str());
    }

    virtual void at(const ExtendedAttribute* node)
    {
        printf("%s", node->getName().c_str());
        // TODO:
    }
};

void print()
{
    Print p;

    getSpecification()->accept(&p);
}
