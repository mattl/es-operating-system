/*
 * Copyright 2008, 2009 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ESJS_H_INCLUDED
#define NINTENDO_ESJS_H_INCLUDED

#include <string.h>
#include <es/formatter.h>
#include <es/list.h>
#include <es/utf.h>
#include "value.h"

class SourceElements;

// SourceElement

class SourceElement
{
    Link<SourceElement> link;

public:
    virtual ~SourceElement() {};
    virtual void print() const
    {
        report("%s", toString("").c_str());
    };
    virtual std::string toString(std::string indent) const
    {
        return "";
    }

    // Process for function declarations
    virtual void process() {};

    virtual CompletionType evaluate()
    {
        return CompletionType(CompletionType::Normal, 0, "");
    };

    typedef ::List<SourceElement, &SourceElement::link>   List;
    friend class SourceElements;
};

class Statement : public SourceElement
{
};

// Expression

class Expression
{
    Link<Expression>    link;

public:
    virtual ~Expression() {};

    virtual Value* evaluate()
    {
        return UndefinedValue::getInstance();   // XXX check later
    };

    virtual void print() const
    {
        report("%s", toString("").c_str());
    }

    virtual std::string toString(std::string indent = "") const
    {
        return "";
    }

    typedef ::List<Expression, &Expression::link> List;
    friend class ArgumentList;
    friend class FormalParameterList;
};

class AssignmentExpression : public Expression
{
public:
    enum Operator
    {
        Assign,
        Multiple,
        Divide,
        Remainder,
        Plus,
        Minus,
        LeftShift,
        SignedRightShift,
        UnsignedRightShift,
        And,
        Xor,
        Or
    };

private:
    Expression*   left;       // LeftHandSideExpression
    Expression*   expression; // AssignmentExpression
    enum Operator op;

public:
    AssignmentExpression(Expression* left, Expression* expression, enum Operator op) :
        left(left),
        expression(expression),
        op(op)
    {
    }

    ~AssignmentExpression()
    {
        delete left;
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        switch (op)
        {
        case Assign:
            s += " = ";
            break;
        case Multiple:
            s += " *= ";
            break;
        case Divide:
            s += " /= ";
            break;
        case Remainder:
            s += " %= ";
            break;
        case Plus:
            s += " += ";
            break;
        case Minus:
            s += " -= ";
            break;
        case LeftShift:
            s += " <<= ";
            break;
        case SignedRightShift:
            s += " >>= ";
            break;
        case UnsignedRightShift:
            s += " >>>= ";
            break;
        case And:
            s += " &= ";
            break;
        case Xor:
            s += " ^= ";
            break;
        case Or:
            s += " |= ";
            break;
        }
        s += expression->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> reference = left->evaluate();
        Register<Value> result;

        if (op == Assign)
        {
            result = expression->evaluate()->getValue();
        }
        else if (op == Plus)
        {
            Register<Value> x = reference->getValue();
            Register<Value> y = expression->evaluate()->getValue();
            Register<Value> s = x->toPrimitive();
            Register<Value> t = y->toPrimitive();
            if (s->isString() || t->isString())
            {
                Register<StringValue> result = new StringValue();
                result->concatenate(s->toString());
                result->concatenate(t->toString());
                reference->putValue(result);
                return result;
            }
            else
            {
                result = new NumberValue(s->toNumber() + t->toNumber());
            }
        }
        else
        {
            double x = reference->getValue()->toNumber();
            double y = expression->evaluate()->getValue()->toNumber();
            switch (op)
            {
            case Multiple:
                result = new NumberValue(x * y);
                break;
            case Divide:
                result = new NumberValue(x / y);
                break;
            case Remainder:
                result = new NumberValue(fmod(x, y));
                break;
            case Minus:
                result = new NumberValue(x - y);
                break;
            case LeftShift:
                result = new NumberValue((s32) x << ((u32) y & 0x1f));
                break;
            case SignedRightShift:
                result = new NumberValue((s32) x >> ((u32) y & 0x1f));
                break;
            case UnsignedRightShift:
                result = new NumberValue((u32) x >> ((u32) y & 0x1f));
                break;
            case And:
                result = new NumberValue((s32) x & (s32) y);
                break;
            case Xor:
                result = new NumberValue((s32) x ^ (s32) y);
                break;
            case Or:
                result = new NumberValue((s32) x | (s32) y);
                break;
            }
        }
        reference->putValue(result);
        return result;
    };
};

class ConditionalExpression : public Expression
{
    Expression* first;
    Expression* second;
    Expression* third;

public:
    ConditionalExpression(Expression* first, Expression* second, Expression* third) :
        first(first),
        second(second),
        third(third)
    {
    }

    ~ConditionalExpression()
    {
        delete first;
        delete second;
        delete third;
    }

    std::string toString(std::string indent) const
    {
        return first->toString(indent) + " ? " + second->toString(indent) + " : " + third->toString(indent);
    }

    Value* evaluate()
    {
        if (first->evaluate()->getValue()->toBoolean())
        {
            return second->evaluate()->getValue();
        }
        else
        {
            return third->evaluate()->getValue();
        }
    }
};

class LogicalExpression : public Expression
{
public:
    enum Operator
    {
        And,
        Or
    };

private:
    Expression*   left;
    Expression*   right;
    enum Operator op;

public:
    LogicalExpression(Expression* left, Expression* right, enum Operator op) :
        left(left),
        right(right),
        op(op)
    {
    }

    ~LogicalExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        switch (op)
        {
        case And:
            s += " && ";
            break;
        case Or:
            s += " || ";
            break;
        }
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> value;
        value = left->evaluate()->getValue();
        switch (op)
        {
        case And:
            if (!value->toBoolean())
            {
                return value;
            }
            break;
        case Or:
            if (value->toBoolean())
            {
                return value;
            }
            break;
        }
        return right->evaluate()->getValue();
    };
};

class EqualityExpression : public Expression
{
public:
    enum Operator
    {
        Equal,
        NotEqual,
        StrictEqual,
        StrictNotEqual
    };

private:
    Expression*   left;
    Expression*   right;
    enum Operator op;

public:
    EqualityExpression(Expression* left, Expression* right, enum Operator op) :
        left(left),
        right(right),
        op(op)
    {
    }

    ~EqualityExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        switch (op)
        {
        case Equal:
            s += " == ";
            break;
        case NotEqual:
            s += " != ";
            break;
        case StrictEqual:
            s += " === ";
            break;
        case StrictNotEqual:
            s += " !== ";
            break;
        }
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> x = left->evaluate()->getValue();
        Register<Value> y = right->evaluate()->getValue();
        bool value;
        switch (op)
        {
        case Equal:
            value = areEqual(x, y);
            break;
        case NotEqual:
            value = !areEqual(x, y);
            break;
        case StrictEqual:
            value = areStrictEqual(x, y);
            break;
        case StrictNotEqual:
            value = !areStrictEqual(x, y);
            break;
        }
        return BoolValue::getInstance(value);
    };

    static bool areStrictEqual(Value* x, Value* y)
    {
        if (x->getType() != y->getType())
        {
            return false;
        }
        if (x->isUndefined() || x->isNull())
        {
            return true;
        }
        if (x->isNumber())
        {
            return (x->toNumber() == y->toNumber()) ? true : false;
        }
        else if (x->isString())
        {
            return (x->toString() == y->toString()) ? true : false;
        }
        else if (x->isBoolean())
        {
            return (x->toBoolean() == y->toBoolean()) ? true : false;
        }
        else
        {
            return (x == y) ? true : false;
        }
    }

    static bool areEqual(Value* x, Value* y)
    {
        if (x->getType() == y->getType())
        {
            return areStrictEqual(x, y);
        }

        if (x->isNull() && y->isUndefined() ||
            x->isUndefined() && y->isNull())
        {
            return true;
        }

        if (x->isNumber() && y->isString() ||
            x->isString() && y->isNumber())
        {
            return (x->toNumber() == y->toNumber()) ? true : false;
        }

        if (x->isBoolean())
        {
            Register<Value> v = new NumberValue(x->toNumber());
            return areEqual(v, y);
        }
        if (y->isBoolean())
        {
            Register<Value> v = new NumberValue(y->toNumber());
            return areEqual(x, v);
        }

        // XXX toPrimitive()

        return false;
    }
};

class RelationalExpression : public Expression
{
public:
    enum Operator
    {
        LessThan,
        GreaterThan,
        LessEqual,
        GreaterEqual
    };

private:
    Expression*   left;
    Expression*   right;
    enum Operator op;

    Value* compare(Value* x, Value* y)
    {
        x = x->toPrimitive();
        y = y->toPrimitive();
        bool value;
        if (x->isString() && y->isString())
        {
            switch (op)
            {
            case LessThan:
                value = (x->toString() < y->toString()) ? true : false;
                break;
            case GreaterThan:
                value = (x->toString() > y->toString()) ? true : false;
                break;
            case LessEqual:
                value = (x->toString() <= y->toString()) ? true : false;
                break;
            case GreaterEqual:
                value = (x->toString() >= y->toString()) ? true : false;
                break;
            }
        }
        else
        {
            double s = x->toNumber();
            double t = y->toNumber();
            if (s == NAN || t == NAN)
            {
                return UndefinedValue::getInstance();
            }
            switch (op)
            {
            case LessThan:
                value = (s < t);
                break;
            case GreaterThan:
                value = (s > t);
                break;
            case LessEqual:
                value = (s <= t);
                break;
            case GreaterEqual:
                value = (s >= t);
                break;
            }
        }
        return BoolValue::getInstance(value);
    }

public:
    RelationalExpression(Expression* left, Expression* right, enum Operator op) :
        left(left),
        right(right),
        op(op)
    {
    }

    ~RelationalExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s += left->toString(indent);
        switch (op)
        {
        case LessThan:
            s += " < ";
            break;
        case GreaterThan:
            s += " > ";
            break;
        case LessEqual:
            s += " <= ";
            break;
        case GreaterEqual:
            s += " >= ";
            break;
        }
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> x = left->evaluate()->getValue();
        Register<Value> y = right->evaluate()->getValue();
        return compare(x, y);
    };
};

class InstanceOfExpression : public Expression
{
    Expression* left;
    Expression* right;

public:
    InstanceOfExpression(Expression* left, Expression* right) :
        left(left),
        right(right)
    {
    }

    ~InstanceOfExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        s += " instanceof ";
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> lvalue = left->evaluate()->getValue();
        Register<Value> rvalue = right->evaluate()->getValue();
        if (!rvalue->isObject())
        {
            throw getErrorInstance("TypeError");
        }
        return BoolValue::getInstance(dynamic_cast<ObjectValue*>(static_cast<Value*>(rvalue))->hasInstance(lvalue));
    };
};

class BitwiseExpression : public Expression
{
public:
    enum Operator
    {
        LeftShift,
        SignedRightShift,
        UnsignedRightShift,
        And,
        Xor,
        Or
    };

private:
    Expression*   left;
    Expression*   right;
    enum Operator op;

public:
    BitwiseExpression(Expression* left, Expression* right, enum Operator op) :
        left(left),
        right(right),
        op(op)
    {
    }

    ~BitwiseExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        switch (op)
        {
        case LeftShift:
            s += " << ";
            break;
        case SignedRightShift:
            s += " >> ";
            break;
        case UnsignedRightShift:
            s += " >>> ";
            break;
        case And:
            s += " & ";
            break;
        case Xor:
            s += " ^ ";
            break;
        case Or:
            s += " | ";
            break;
        }
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        double x = left->evaluate()->getValue()->toNumber();
        double y = right->evaluate()->getValue()->toNumber();
        switch (op)
        {
        case LeftShift:
            return new NumberValue((s32) x << ((u32) y & 0x1f));
            break;
        case SignedRightShift:
            return new NumberValue((s32) x >> ((u32) y & 0x1f));
            break;
        case UnsignedRightShift:
            return new NumberValue((u32) x >> ((u32) y & 0x1f));
            break;
        case And:
            return new NumberValue((s32) x & (s32) y);
            break;
        case Xor:
            return new NumberValue((s32) x ^ (s32) y);
            break;
        case Or:
            return new NumberValue((s32) x | (s32) y);
            break;
        }
    };
};

// 11.6.1 The Addition operator
class AdditionExpression : public Expression
{
    Expression* left;
    Expression* right;

public:
    AdditionExpression(Expression* left, Expression* right) :
        left(left),
        right(right)
    {
    }

    ~AdditionExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        return left->toString(indent) + " + " + right->toString(indent);
    }

    Value* evaluate()
    {
        Register<Value> lvalue = left->evaluate()->getValue();
        Register<Value> rvalue = right->evaluate()->getValue();

        Register<Value> lprim = lvalue->toPrimitive();
        Register<Value> rprim = rvalue->toPrimitive();
        if (lprim->isString() || rprim->isString())
        {
            Register<StringValue> result = new StringValue();
            result->concatenate(lprim->toString());
            result->concatenate(rprim->toString());
            return result;
        }
        else
        {
            return new NumberValue(lprim->toNumber() + rprim->toNumber());
        }
    };
};

// 11.6.2 The Subtraction operator
class SubtractionExpression : public Expression
{
    Expression* left;
    Expression* right;

public:
    SubtractionExpression(Expression* left, Expression* right) :
        left(left),
        right(right)
    {
    }

    ~SubtractionExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        return left->toString(indent) + " - " + right->toString(indent);
    }

    Value* evaluate()
    {
        double x = left->evaluate()->getValue()->toNumber();
        double y = right->evaluate()->getValue()->toNumber();
        return new NumberValue(x - y);
    };
};

class MultiplicativeExpression : public Expression
{
public:
    enum Operator
    {
        Multiple,
        Divide,
        Remainder
    };

private:
    Expression*   left;
    Expression*   right;
    enum Operator op;

public:
    MultiplicativeExpression(Expression* left, Expression* right, enum Operator op) :
        left(left),
        right(right),
        op(op)
    {
    }

    ~MultiplicativeExpression()
    {
        delete left;
        delete right;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = left->toString(indent);
        switch (op)
        {
        case Multiple:
            s += " * ";
            break;
        case Divide:
            s += " / ";
            break;
        case Remainder:
            s += " % ";
            break;
        }
        s += right->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        double x = left->evaluate()->getValue()->toNumber();
        double y = right->evaluate()->getValue()->toNumber();
        switch (op)
        {
        case Multiple:
            return new NumberValue(x * y);
            break;
        case Divide:
            return new NumberValue(x / y);
            break;
        case Remainder:
            return new NumberValue(fmod(x, y));
            break;
        }
    };
};

class UnaryExpression : public Expression
{
public:
    enum Operator
    {
        Plus,
        Minus,
        BitwiseNot,
        LogicalNot
    };

private:
    Expression*   expression;
    enum Operator op;

public:
    UnaryExpression(Expression* expression, enum Operator op) :
        expression(expression),
        op(op)
    {
    }

    ~UnaryExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        switch (op)
        {
        case Plus:
            s += "+";
            break;
        case Minus:
            s += "-";
            break;
        case BitwiseNot:
            s += "~";
            break;
        case LogicalNot:
            s += "!";
            break;
        }
        s += expression->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate()->getValue();
        switch (op)
        {
        case Plus:
            return new NumberValue(object->toNumber());
            break;
        case Minus:
            return new NumberValue(-object->toNumber());
            break;
        case BitwiseNot:
            return new NumberValue(~static_cast<u32>(object->toNumber()));
            break;
        case LogicalNot:
            return BoolValue::getInstance(!object->toBoolean());
            break;
        }
    };
};

class DeleteExpression : public Expression
{
    Expression* expression;

public:
    DeleteExpression(Expression* expression) :
        expression(expression)
    {
    }

    ~DeleteExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        return "delete " + expression->toString(indent);
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        ReferenceValue* reference = dynamic_cast<ReferenceValue*>(static_cast<Value*>(object));
        if (!reference)
        {
            return BoolValue::getInstance(true);
        }

        ObjectValue* base = dynamic_cast<ObjectValue*>(reference->getBase());
        if (!base)
        {
            return BoolValue::getInstance(true);
        }

        return BoolValue::getInstance(base->remove(reference->getPropertyName()));
    };
};

class VoidExpression : public Expression
{
    Expression* expression;

public:
    VoidExpression(Expression* expression) :
        expression(expression)
    {
    }

    ~VoidExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        return "void " + expression->toString(indent);
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate()->getValue();
        return UndefinedValue::getInstance();
    };
};

class TypeOfExpression : public Expression
{
    Expression* expression;

public:
    TypeOfExpression(Expression* expression) :
        expression(expression)
    {
    }

    ~TypeOfExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        return "typeof " + expression->toString(indent);
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        if (object->isReference())
        {
            ReferenceValue* reference = dynamic_cast<ReferenceValue*>(static_cast<Value*>(object));
            if (reference->getBase()->isNull())
            {
                return new StringValue("undefined");
            }
        }
        object = object->getValue();
        switch (object->getType())
        {
        case Value::NullType:
            return new StringValue("object");
            break;
        case Value::BoolType:
            return new StringValue("boolean");
            break;
        case Value::StringType:
            return new StringValue("string");
            break;
        case Value::NumberType:
            return new StringValue("number");
            break;
        case Value::ObjectType:
            {
                ObjectValue* function = dynamic_cast<ObjectValue*>(static_cast<Value*>(object));
                return new StringValue(function->getCode() ? "function" : "object");
            }
            break;
        case Value::PropertyType:
        case Value::UndefinedType:
        case Value::ReferenceType:
        default:
            return new StringValue("undefined");
            break;
        }
    };
};

class PrefixExpression : public Expression
{
public:
    enum Operator
    {
        Increment,
        Decrement
    };

private:
    Expression*   expression;
    enum Operator op;

public:
    PrefixExpression(Expression* expression, enum Operator op) :
        expression(expression),
        op(op)
    {
    }

    ~PrefixExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        switch (op)
        {
        case Increment:
            s += "++";
            break;
        case Decrement:
            s += "--";
            break;
        }
        s += expression->toString(indent);
        return s;
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        double value = object->getValue()->toNumber();
        switch (op)
        {
        case Increment:
            value += 1.0;
            break;
        case Decrement:
            value -= 1.0;
            break;
        }
        Register<Value> result = new NumberValue(value);
        object->putValue(result);
        return result;
    };
};

class PostfixExpression : public Expression
{
public:
    enum Operator
    {
        Increment,
        Decrement
    };

private:
    Expression*   expression;
    enum Operator op;

public:
    PostfixExpression(Expression* expression, enum Operator op) :
        expression(expression),
        op(op)
    {
    }

    ~PostfixExpression()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = expression->toString(indent);
        switch (op)
        {
        case Increment:
            s += "++";
            break;
        case Decrement:
            s += "--";
            break;
        }
        return s;
    }

    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        double value = object->getValue()->toNumber();
        Register<Value> result;
        switch (op)
        {
        case Increment:
            result = new NumberValue(value + 1.0);
            break;
        case Decrement:
            result = new NumberValue(value - 1.0);
            break;
        }
        object->putValue(result);
        return new NumberValue(value);
    };
};

// PrimaryExpression

class This : public Expression
{
public:
    std::string toString(std::string indent) const
    {
        return "this";
    }

    Value* evaluate()
    {
        return getThis();
    };
};

class Identifier : public Expression
{
    std::string identifier;

public:
    Identifier(const char* identifier)
    {
        if (identifier)
        {
            this->identifier = identifier;
        }
    }

    // 10.1.4
    Value* evaluate()
    {
        for (ObjectValue* object = getScopeChain(); object; object = object->getNext())
        {
            if (object->hasProperty(identifier))
            {
                return new ReferenceValue(object, identifier);
            }
        }
        return new ReferenceValue(NullValue::getInstance(), identifier);
    };

    std::string toString(std::string indent = "") const
    {
        return identifier;
    }
};

class NullLiteral : public Expression
{
public:
    std::string toString(std::string indent) const
    {
        return "null";
    }
    Value* evaluate()
    {
        return NullValue::getInstance();
    }
};

class BoolLiteral : public Expression
{
    bool value;
public:
    BoolLiteral(bool value) :
        value(value)
    {
    }
    ~BoolLiteral()
    {
    };
    std::string toString(std::string indent) const
    {
        return value ? "true" : "false";
    }
    Value* evaluate()
    {
        return BoolValue::getInstance(value);
    }
};

class NumericLiteral : public Expression
{
    double value;
public:
    NumericLiteral(double value) :
        value(value)
    {
    }

    Value* evaluate()
    {
        return new NumberValue(value);
    }

    std::string toString(std::string indent) const
    {
        std::string s;
        Formatter f(s);
        f.setMode(Formatter::Mode::ECMAScript);
        f.format("%g", value);
        return s;
    }
};

class StringLiteral : public Expression
{
    std::string value;
public:
    StringLiteral(char* s)
    {
        while (char c = *s++)
        {
            if (c != '\\')
            {
                this->value += c;
            }
            else
            {
                // Process escape sequence
                u32   ucode;
                char  utf8[5];
                char* t;
                c = *s++;
                switch (c)
                {
                case '\'':
                case '"':
                case '\\':
                    this->value += c;
                    break;
                case 'b':
                    this->value += '\b';
                    break;
                case 'f':
                    this->value += '\f';
                    break;
                case 'n':
                    this->value += '\n';
                    break;
                case 'r':
                    this->value += '\r';
                    break;
                case 't':
                    this->value += '\t';
                    break;
                case 'v':
                    this->value += '\v';
                    break;
                case '0':
                    this->value += '\0';
                    break;
                case 'x':
                    s = parseHex(s, 2, ucode);
                    t = utf32to8(ucode, utf8);
                    if (t)
                    {
                        *t = 0;
                        this->value += utf8;
                    }
                    break;
                case 'u':
                    s = parseHex(s, 4, ucode);
                    t = utf32to8(ucode, utf8);
                    if (t)
                    {
                        *t = 0;
                        this->value += utf8;
                    }
                    break;
                default:
                    this->value += '\\';
                    this->value += c;
                    break;
                }
            }
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = "\"";
        Formatter f(s);
        f.setMode(Formatter::Mode::ECMAScript);
        for (const char* p = value.c_str(); char c = *p; ++p)
        {
            switch (c)
            {
            case '"':
                s += "\\\"";
                break;
            case '\b':
                s += "\\b";
                break;
            case '\f':
                s += "\\f";
                break;
            case '\n':
                s += "\\n";
                break;
            case '\r':
                s += "\\r";
                break;
            case '\t':
                s += "\\t";
                break;
            case '\v':
                s += "\\v";
                break;
            case '\0':
                s += "\\0";
                break;
            case '\\':
                if (strchr("bfnrtvxu", p[1]))
                {
                    s += "\\\\";
                }
                else
                {
                    s += "\\";
                }
                break;
            default:
                if (c < ' ' || c == 127)
                {
                    f.format("\\x%02x", (u8) c);
                }
                else
                {
                    f.format("%c", c);
                }
                break;
            }
        }
        s += "\"";
        return s;
    }

    Value* evaluate()
    {
        return new StringValue(value);
    }

    std::string toString() const
    {
        return value;
    }
};


class RegularExpressionLiteral : public Expression
{
    std::string value;

public:
    RegularExpressionLiteral(std::string s) :
        value(s)
    {
    }

    Value* evaluate();

    std::string toString(std::string indent) const
    {
        return value;
    }
};

class Elision : public Expression
{
public:
    Value* evaluate()
    {
        return UndefinedValue::getInstance();
    };
};

class ArrayLiteral : public Expression
{
    mutable Expression::List    list;

public:
    ~ArrayLiteral()
    {
        while (!list.isEmpty())
        {
            Expression* element = list.removeLast();
            delete element;
        }
    }

    void add(Expression* element)
    {
        list.addLast(element);
    }

    void add(ArrayLiteral* array)
    {
        while (!array->list.isEmpty())
        {
            Expression* element = array->list.removeFirst();
            list.addLast(element);
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = "[ ";
        Expression::List::Iterator iter = list.begin();
        while (Expression* element = iter.next())
        {
            s += element->toString(indent);
            if (iter.hasNext() || dynamic_cast<Elision*>(element))
            {
                s += ", ";
            }
        }
        s += " ]";
        return s;
    }

    Value* evaluate()
    {
        Register<ArrayValue> object = new ArrayValue;
        Expression::List::Iterator iter = list.begin();
        Expression* element;
        for (u32 i = 0; element = iter.next(); ++i)
        {
            Register<Value> value = element->evaluate();
            char name[12];
            sprintf(name, "%d", i);
            object->put(name, value);
        }
        return object;
    };
};

class PropertyNameAndValue
{
    Link<PropertyNameAndValue>  link;
    Expression*                 name;
    Expression*                 value;

public:
    PropertyNameAndValue(Expression* name, Expression* value) :
        name(name),
        value(value)
    {
    }

    ~PropertyNameAndValue()
    {
        delete name;
        delete value;
    };

    void evaluate(ObjectValue* object)
    {
        Register<Value> v = value->evaluate()->getValue();
        object->put(name->toString(), v);
    }

    std::string toString(std::string indent) const
    {
        return name->toString(indent) + ": " + value->toString(indent);
    }

    friend class ObjectLiteral;
    typedef ::List<PropertyNameAndValue, &PropertyNameAndValue::link> List;
};

class ObjectLiteral : public Expression
{
    mutable PropertyNameAndValue::List list;

public:
    ~ObjectLiteral()
    {
        while (!list.isEmpty())
        {
            PropertyNameAndValue* property = list.removeLast();
            delete property;
        }
    }

    void add(PropertyNameAndValue* property)
    {
        list.addLast(property);
    }

    std::string toString(std::string indent) const
    {
        std::string s = "{ ";
        PropertyNameAndValue::List::Iterator iter = list.begin();
        while (PropertyNameAndValue* property = iter.next())
        {
            s += property->toString(indent);
            if (iter.hasNext())
            {
                s += ", ";
            }
        }
        s += " }";
        return s;
    }

    Value* evaluate()
    {
        Register<ObjectValue> object = new ObjectValue;
        PropertyNameAndValue::List::Iterator iter = list.begin();
        PropertyNameAndValue* property;
        while (property = iter.next())
        {
            property->evaluate(object);
        }
        return object;
    };
};

class GroupingExpression : public Expression
{
    Expression* expression;

public:
    GroupingExpression(Expression* expression) :
        expression(expression)
    {
    }
    ~GroupingExpression()
    {
        delete expression;
    };
    std::string toString(std::string indent) const
    {
        return "(" + expression->toString(indent) + ")";
    }
    Value* evaluate()
    {
        return expression->evaluate();
    }
};

// MemberExpression

class ArgumentList
{
    mutable Expression::List    list;

public:
    ~ArgumentList()
    {
        while (!list.isEmpty())
        {
            Expression* expression = list.removeLast();
            delete expression;
        }
    };

    void add(Expression* expression)
    {
        list.addLast(expression);
    }

    // 11.2.4
    ListValue* evaluate()
    {
        Register<ListValue> arguments = new ListValue;
        Expression::List::Iterator iter = list.begin();
        while (Expression* expression = iter.next())
        {
            Register<Value> value = expression->evaluate()->getValue();
            arguments->push(value);
        }
        return arguments;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        Expression::List::Iterator iter = list.begin();
        while (Expression* expression = iter.next())
        {
            s += expression->toString(indent);
            if (iter.hasNext())
            {
                s += ", ";
            }
        }
        return s;
    };

    bool isEmpty()
    {
        return list.isEmpty();
    }
};

class FunctionCall : public Expression
{
    Expression*     expression;
    ArgumentList*   arguments;

public:
    FunctionCall(Expression* expression, ArgumentList* arguments) :
        expression(expression),
        arguments(arguments)
    {
    }

    ~FunctionCall()
    {
        delete expression;
        delete arguments;
    }

    // 11.2.3
    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        Register<ListValue> list = arguments->evaluate();
        Register<ObjectValue> function = dynamic_cast<ObjectValue*>(object->getValue());
        if (!function || !function->getCode())
        {
            throw getErrorInstance("TypeError");
        }
        ReferenceValue* reference = dynamic_cast<ReferenceValue*>(static_cast<Value*>(object));
        Register<Value> self = reference ? reference->getBase() : NullValue::getInstance();
        if (self == getScopeChain())    // XXX right is activationObject
        {
            self = NullValue::getInstance();
        }
        return function->call(self, list);
    }

    std::string toString(std::string indent) const
    {
        return expression->toString(indent) + "(" + arguments->toString(indent) + ")";
    };
};

class BracketAccessor : public Expression
{
    Expression* expression;
    Expression* identifier;

public:
    BracketAccessor(Expression* expression, Expression* identifier) :
        expression(expression),
        identifier(identifier)
    {
    }

    ~BracketAccessor()
    {
        delete expression;
        delete identifier;
    }

    Value* evaluate()
    {
        Register<Value> value = expression->evaluate()->getValue();
        Register<Value> object = value->toObject();
        Register<Value> name = identifier->evaluate()->getValue();;
        return new ReferenceValue(object, name->toString());
    }

    std::string toString(std::string indent) const
    {
        return expression->toString(indent) + "[" + identifier->toString(indent) + "]";
    };
};

class DotAccessor : public Expression
{
    Expression* expression;
    Identifier* identifier;

public:
    DotAccessor(Expression* expression, Identifier* identifier) :
        expression(expression),
        identifier(identifier)
    {
    }

    ~DotAccessor()
    {
        delete expression;
        delete identifier;
    }

    Value* evaluate()
    {
        Register<Value> value = expression->evaluate()->getValue();
        Register<Value> object = value->toObject();
        return new ReferenceValue(object, identifier->toString());
    }

    std::string toString(std::string indent) const
    {
        return expression->toString(indent) + "." + identifier->toString(indent);
    };
};

class NewExpression : public Expression
{
    Expression*     expression;
    ArgumentList*   arguments;

public:
    NewExpression(Expression* expression, ArgumentList* arguments) :
        expression(expression),
        arguments(arguments)
    {
    }

    ~NewExpression()
    {
        delete expression;
        delete arguments;
    }

    // 11.2.2
    Value* evaluate()
    {
        Register<Value> object = expression->evaluate();
        Register<ObjectValue> function = dynamic_cast<ObjectValue*>(object->getValue());
        Register<ListValue> list = arguments->evaluate();
        if (!function || !function->getCode())
        {
            throw getErrorInstance("TypeError");
        }
        return function->construct(list);
    }

    std::string toString(std::string indent) const
    {
        std::string s = "new ";
        s += expression->toString(indent);
        if (!arguments->isEmpty())
        {
            s += "(";
            s += arguments->toString(indent);
            s += ")";
        }
        return s;
    };
};

class CommaExpression : public Expression
{
    Expression* first;
    Expression* second;

public:
    CommaExpression(Expression* first, Expression* second) :
        first(first),
        second(second)
    {
    }

    ~CommaExpression()
    {
        delete first;
        delete second;
    }

    std::string toString(std::string indent) const
    {
        return first->toString(indent) + ", " + second->toString(indent);
    }

    Value* evaluate()
    {
        first->evaluate()->getValue();
        return second->evaluate()->getValue();
    };
};

// Statement

class StatementList
{
    mutable SourceElement::List list;

public:
    StatementList(Statement* statement)
    {
        add(statement);
    }

    virtual ~StatementList()
    {
        while (!list.isEmpty())
        {
            SourceElement* element = list.removeLast();
            delete element;
        }
    };

    void add(Statement* statement)
    {
        list.addLast(statement);
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        SourceElement::List::Iterator iter = list.begin();
        while (SourceElement* statement = iter.next())
        {
            s += statement->toString(indent);
        }
        return s;
    }

    CompletionType evaluate()
    {
        CompletionType result;
        Register<Value> value;
        SourceElement::List::Iterator iter = list.begin();
        while (SourceElement* element = iter.next())
        {
            result = element->evaluate();
            if (result.getValue())
            {
                value = result.getValue();
            }
            if (result.isThrow())
            {
                return CompletionType(CompletionType::Throw, value, "");
            }
            if (result.isAbrupt())
            {
                break;
            }
        }
        return CompletionType(result.getType(), value, result.getTarget());
    }
};

class Block : public Statement
{
    StatementList*  list;

public:
    Block(StatementList* list = 0) :
        list(list)
    {
    }
    ~Block()
    {
        if (list)
        {
            delete list;
        }
    };

    std::string toString(std::string indent) const
    {
        std::string s;

        s += indent + "{\n";
        if (list)
        {
            s += list->toString(indent + "    ");
        }
        s += indent + "}\n";
        return s;
    };

    CompletionType evaluate()
    {
        if (!list)
        {
            return CompletionType(CompletionType::Normal, 0, "");
        }
        else
        {
            return list->evaluate();
        }
    }
};

class VariableDeclaration
{
    Link<VariableDeclaration> link;

    Identifier* identifier;
    Expression* initialiser;

public:
    VariableDeclaration(Identifier* identifier, Expression* initialiser = 0) :
        identifier(identifier),
        initialiser(initialiser)
    {
    }

    ~VariableDeclaration()
    {
        delete identifier;
        if (initialiser)
        {
            delete initialiser;
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = identifier->toString(indent);
        if (initialiser)
        {
            s += " = ";
            s += initialiser->toString(indent);
        }
        return s;
    }

    Value* evaluate()
    {
        // Note the following is es specific semantics:
        // variables are only valid after the declaration.
        ObjectValue* variableObject = getScopeChain();
        std::string name = identifier->toString();

        Register<Value> value;
        if (initialiser)
        {
            value = initialiser->evaluate()->getValue();
        }
        else
        {
            value = UndefinedValue::getInstance();
        }
        variableObject->put(name, value, ObjectValue::DontDelete);

        return new ReferenceValue(variableObject, name);    // for "for-in" statement
    };

    typedef ::List<VariableDeclaration, &VariableDeclaration::link>   List;
    friend class VariableDeclarationList;
};

class VariableDeclarationList : public Expression
{
    mutable VariableDeclaration::List   list;

public:
    ~VariableDeclarationList()
    {
        while (!list.isEmpty())
        {
            VariableDeclaration* var = list.removeLast();
            delete var;
        }
    }

    void add(VariableDeclaration* var)
    {
        list.addLast(var);
    }

    std::string toString(std::string indent) const
    {
        std::string s = "var ";
        VariableDeclaration::List::Iterator iter = list.begin();
        while (VariableDeclaration* var = iter.next())
        {
            s += var->toString(indent);
            if (iter.hasNext())
            {
                s += ", ";
            }
        }
        return s;
    }

    Value* evaluate()
    {
        Register<Value> value;
        VariableDeclaration::List::Iterator iter = list.begin();
        while (VariableDeclaration* var = iter.next())
        {
            value = var->evaluate();
        }
        return value;
    };
};

class VariableStatement : public Statement
{
    VariableDeclarationList* list;

public:
    VariableStatement(VariableDeclarationList* list) :
        list(list)
    {
    }

    ~VariableStatement()
    {
        delete list;
    }

    std::string toString(std::string indent) const
    {
        return indent + list->toString(indent) + ";\n";
    }

    CompletionType evaluate()
    {
        list->evaluate();
        return CompletionType(CompletionType::Normal, 0, "");
    };
};

class EmptyStatement : public Statement
{
public:
    std::string toString(std::string indent) const
    {
        return indent + ";\n";
    }

    CompletionType evaluate()
    {
        return CompletionType(CompletionType::Normal, 0, "");
    };
};

class ExpressionStatement : public Statement
{
    Expression* expression;
public:
    ExpressionStatement(Expression* expression) :
        expression(expression)
    {
    }

    ~ExpressionStatement()
    {
        delete expression;
    }

    std::string toString(std::string indent) const
    {
        return indent + expression->toString(indent) + ";\n";
    }

    CompletionType evaluate()
    {
        Value* value = expression->evaluate();
        return CompletionType(CompletionType::Normal, value->getValue(), "");
    };
};

class IfStatement : public Statement
{
    Expression* condition;
    Statement*  then;
    Statement*  otherwise;
public:
    IfStatement(Expression* condition, Statement* then, Statement* otherwise = 0) :
        condition(condition),
        then(then),
        otherwise(otherwise)
    {
    }

    ~IfStatement()
    {
        delete condition;
        delete then;
        if (otherwise)
        {
            delete otherwise;
        }
    }

    CompletionType evaluate()
    {
        if (condition->evaluate()->getValue()->toBoolean())
        {
            return then->evaluate();
        }
        else if (otherwise)
        {
            return otherwise->evaluate();
        }
        else
        {
            return CompletionType(CompletionType::Normal, 0, "");
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "if (";
        s += condition->toString(indent);
        s += ")\n";
        s += then->toString(indent);
        if (otherwise)
        {
            if (dynamic_cast<IfStatement*>(otherwise))
            {
                s += indent + "else ";
            }
            else
            {
                s += indent + "else\n";
            }
            s += otherwise->toString(indent);
        }
        return s;
    }
};

class DoWhileStatement : public Statement
{
    Statement*  statement;
    Expression* expression;
public:
    DoWhileStatement(Statement* statement, Expression* expression) :
        statement(statement),
        expression(expression)
    {
    }

    ~DoWhileStatement()
    {
        delete statement;
        delete expression;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        do
        {
            CompletionType result = statement->evaluate();
            if (result.getValue())
            {
                value = result.getValue();
            }
            if (result.isContinue() && result.getTarget() == "")
            {
                continue;
            }
            if (result.isBreak() && result.getTarget() == "")
            {
                break;
            }
            if (result.isAbrupt())
            {
                return result;
            }
        } while (expression->evaluate()->getValue()->toBoolean());
        return CompletionType(CompletionType::Normal, value, "");
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "do\n";
        s += statement->toString(indent);
        s += indent + "while (";
        s += expression->toString(indent);
        s += ");\n";
        return s;
    }
};

class WhileStatement : public Statement
{
    Expression* expression;
    Statement*  statement;
public:
    WhileStatement(Expression* expression, Statement* statement) :
        expression(expression),
        statement(statement)
    {
    }

    ~WhileStatement()
    {
        delete expression;
        delete statement;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        while (expression->evaluate()->getValue()->toBoolean())
        {
            CompletionType result = statement->evaluate();
            if (result.getValue())
            {
                value = result.getValue();
            }
            if (result.isContinue() && result.getTarget() == "")
            {
                continue;
            }
            if (result.isBreak() && result.getTarget() == "")
            {
                break;
            }
            if (result.isAbrupt())
            {
                return result;
            }
        }
        return CompletionType(CompletionType::Normal, value, "");
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "while (";
        s += expression->toString(indent);
        s += ")\n";
        s += statement->toString(indent);
        return s;
    }
};

class ForStatement : public Statement
{
    Expression* first;
    Expression* second;
    Expression* third;
    Statement*  statement;
public:
    ForStatement(Expression* first, Expression* second, Expression* third,
                 Statement* statement) :
        first(first),
        second(second),
        third(third),
        statement(statement)
    {
    }

    ~ForStatement()
    {
        if (first)
        {
            delete first;
        }
        if (second)
        {
            delete second;
        }
        if (third)
        {
            delete third;
        }
        delete statement;
    }

    CompletionType evaluate()
    {
        Register<Value> value;
        for (first ? first->evaluate()->getValue() : 0;
             second ? second->evaluate()->getValue()->toBoolean() : true;
             third ? third->evaluate()->getValue() : 0)
        {
            CompletionType result = statement->evaluate();
            if (result.getValue())
            {
                value = result.getValue();
            }
            if (result.isBreak() && result.getTarget() == "")
            {
                break;
            }
            if (result.isContinue() && result.getTarget() == "")
            {
                continue;
            }
            if (result.isAbrupt())
            {
                return result;
            }
        }
        return CompletionType(CompletionType::Normal, value, "");
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "for (";
        if (first)
        {
            s += first->toString(indent);
        }
        s += ";";
        if (second)
        {
            s += " ";
            s += second->toString(indent);
        }
        s += ";";
        if (third)
        {
            s += " ";
            s += third->toString(indent);
        }
        s += ")\n";
        s += statement->toString(indent);
        return s;
    }
};

class ForInStatement : public Statement
{
    Expression* first;
    Expression* second;
    Statement*  statement;
public:
    ForInStatement(Expression* first, Expression* second, Statement* statement) :
        first(first),
        second(second),
        statement(statement)
    {
    }

    ~ForInStatement()
    {
        delete first;
        delete second;
        delete statement;
    }

    CompletionType evaluate()
    {
        Register<Value> value = second->evaluate()->getValue();
        Register<ObjectValue> object = value->toObject();
        return object->iterate(first, statement);
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "for (";
        s += first->toString(indent);
        s += " in ";
        s += second->toString(indent);
        s += ")\n";
        s += statement->toString(indent);
        return s;
    }
};

class ContinueStatement : public Statement
{
    Identifier* identifier;
public:
    ContinueStatement(Identifier* identifier = 0) :
        identifier(identifier)
    {
    }

    ~ContinueStatement()
    {
        if (identifier)
        {
            delete identifier;
        }
    }

    CompletionType evaluate()
    {
        if (!identifier)
        {
            return CompletionType(CompletionType::Continue, 0, "");
        }
        else
        {
            return CompletionType(CompletionType::Continue, 0, identifier->toString());
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "continue";
        if (identifier)
        {
            s += " ";
            s += identifier->toString(indent);
        }
        s += ";\n";
        return s;
    }
};

class BreakStatement : public Statement
{
    Identifier* identifier;
public:
    BreakStatement(Identifier* identifier = 0) :
        identifier(identifier)
    {
    }

    ~BreakStatement()
    {
        if (identifier)
        {
            delete identifier;
        }
    }

    CompletionType evaluate()
    {
        if (!identifier)
        {
            return CompletionType(CompletionType::Break, 0, "");
        }
        else
        {
            return CompletionType(CompletionType::Break, 0, identifier->toString());
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s = indent + "break";
        if (identifier)
        {
            s += " ";
            s += identifier->toString(indent);
        }
        s += ";\n";
        return s;
    }
};

class ReturnStatement : public Statement
{
    Expression* expression;
public:
    ReturnStatement(Expression* expression = 0) :
        expression(expression)
    {
    }

    ~ReturnStatement()
    {
        if (expression)
        {
            delete expression;
        }
    }

    CompletionType evaluate()
    {
        if (!expression)
        {
            return CompletionType(CompletionType::Return, UndefinedValue::getInstance(), "");
        }
        else
        {
            return CompletionType(CompletionType::Return, expression->evaluate()->getValue(), "");
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent + "return";
        if (expression)
        {
            s += " " + expression->toString(indent);
        }
        s += ";\n";
        return s;
    }
};

class WithStatement : public Statement
{
    Expression* expression;
    Statement*  statement;
public:
    WithStatement(Expression* expression, Statement* statement) :
        expression(expression),
        statement(statement)
    {
    }

    ~WithStatement()
    {
        delete expression;
        delete statement;
    }

    CompletionType evaluate()
    {
        Register<Value> value = expression->evaluate()->getValue();
        Register<ObjectValue> object = value->toObject();

        ExecutionContext* context = new ExecutionContext(object);

        CompletionType result;
        try
        {
            result = statement->evaluate();
        }
        catch (Value* value)
        {
            result.setType(CompletionType::Throw);
            result.setValue(value);
        }

        delete context;
        return result;
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "with (";
        s += expression->toString(indent);
        s += ")\n";
        s += statement->toString(indent);
        return s;
    }
};

class LabelledStatement : public Statement
{
    Identifier* identifier;
    Statement* statement;
public:
    LabelledStatement(Identifier* identifier, Statement* statement) :
        identifier(identifier),
        statement(statement)
    {
    }

    ~LabelledStatement()
    {
        delete identifier;
        delete statement;
    }

    CompletionType evaluate()
    {
        CompletionType result;
        do
        {
            result = statement->evaluate();
            if (result.isBreak() && identifier->toString() == result.getTarget())
            {
                return CompletionType(CompletionType::Normal, result.getValue(), "");
            }
        } while (result.isContinue() && identifier->toString() == result.getTarget());
        return result;
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += identifier->toString(indent);
        s += ":\n";
        s += statement->toString(indent);
        return s;
    }
};

class CaseClause
{
    Link<CaseClause> link;
    Expression*      expression;
    StatementList*   list;

public:
    // Set expression to zero for DefaultClause.
    CaseClause(Expression* expression, StatementList* list = 0) :
        expression(expression),
        list(list)
    {
    }

    ~CaseClause()
    {
        if (expression)
        {
            delete expression;
        }
        if (list)
        {
            delete list;
        }
    };

    StatementList* getStatementList() const
    {
        return list;
    }

    Value* evaluate(Value* param)
    {
        if (expression)
        {
            return expression->evaluate()->getValue();
        }
        else
        {
            return 0;
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        if (expression)
        {
            s += "case ";
            s += expression->toString(indent);
            s += ":\n";
        }
        else
        {
            s += "default:\n";
        }
        if (list)
        {
            s += list->toString(indent + "    ");
        }
        return s;
    }

    friend class CaseBlock;
    typedef ::List<CaseClause, &CaseClause::link> List;
};

class CaseBlock
{
    mutable CaseClause::List list;

public:
    ~CaseBlock()
    {
        while (!list.isEmpty())
        {
            CaseClause* clause = list.removeLast();
            delete clause;
        }
    }

    void add(CaseClause* clause)
    {
        list.addLast(clause);
    }

    void add(CaseBlock* block)
    {
        while (!block->list.isEmpty())
        {
            CaseClause* clause = block->list.removeFirst();
            list.addLast(clause);
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s += indent + "{\n";
        CaseClause::List::Iterator iter = list.begin();
        while (CaseClause* clause = iter.next())
        {
            s += clause->toString(indent);
        }
        s += indent + "}\n";
        return s;
    }

    CompletionType evaluate(Value* param)
    {
        CaseClause::List::Iterator iter = list.begin();
        CaseClause* clause;
        CaseClause* defaultClause = 0;
        while (clause = iter.next())
        {
            Register<Value> label = clause->evaluate(param);
            if (!static_cast<Value*>(label))
            {
                // default:
                defaultClause = clause;
                continue;
            }
            if (EqualityExpression::areStrictEqual(label, param))
            {
                break;
            }
        }

        if (!clause)
        {
            // Use default
            clause = defaultClause;
            if (clause)
            {
                iter = list.list(clause);
            }
        }

        CompletionType result;
        Register<Value> value;
        if (clause)
        {
            StatementList* list = clause->getStatementList();
            if (list)
            {
                result = list->evaluate();
                if (result.isAbrupt())
                {
                    return result;
                }
                value = result.getValue();
            }

            while (clause = iter.next())
            {
                list = clause->getStatementList();
                if (list)
                {
                    result = list->evaluate();
                    if (result.getValue())
                    {
                        value = result.getValue();
                    }
                    if (result.isAbrupt())
                    {
                        result.setValue(value);
                        return result;
                    }
                }
            }
        }

        return CompletionType(CompletionType::Normal, value, "");
    };
};

class SwitchStatement : public Statement
{
    Expression* expression;
    CaseBlock*  caseBlock;
public:
    SwitchStatement(Expression* expression, CaseBlock* caseBlock) :
        expression(expression),
        caseBlock(caseBlock)
    {
    }

    ~SwitchStatement()
    {
        delete expression;
        delete caseBlock;
    }

    CompletionType evaluate()
    {
        Register<Value> param = expression->evaluate()->getValue();
        CompletionType result = caseBlock->evaluate(param);
        if (result.isBreak() && result.getTarget() == "")
        {
            result.setType(CompletionType::Normal);
        }
        return result;
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "switch (";
        s += expression->toString(indent);
        s += ")\n";
        s += caseBlock->toString(indent);
        return s;
    }
};

class ThrowStatement : public Statement
{
    Expression* expression;
public:
    ThrowStatement(Expression* expression) :
        expression(expression)
    {
    }

    ~ThrowStatement()
    {
        delete expression;
    }

    CompletionType evaluate()
    {
        return CompletionType(CompletionType::Throw, expression->evaluate()->getValue(), "");
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "throw ";
        s += expression->toString(indent);
        s += ";\n";
        return s;
    }
};

class Catch
{
    Identifier* identifier;
    Statement*  block;

public:
    Catch(Identifier* identifier, Statement* block) :
        identifier(identifier),
        block(block)
    {
    }
    ~Catch()
    {
        delete identifier;
        delete block;
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "catch (";
        s += identifier->toString(indent);
        s += ")\n";
        s += block->toString(indent);
        return s;
    };

    CompletionType evaluate(CompletionType& c)
    {
        Register<ObjectValue> object = new ObjectValue;
        object->put(identifier->toString(), c.getValue()); // { DontDelete }
        ExecutionContext* context = new ExecutionContext(object);

        CompletionType result;
        try
        {
            result = block->evaluate();
        }
        catch (Value* value)
        {
            result.setType(CompletionType::Throw);
            result.setValue(value);
        }

        delete context;
        return result;
    }
};

class Finally
{
    Statement*  block;

public:
    Finally(Statement* block) :
        block(block)
    {
    }
    ~Finally()
    {
        delete block;
    }

    std::string toString(std::string indent) const
    {
        return indent + "finally\n" + block->toString(indent);
    };

    CompletionType evaluate()
    {
        return block->evaluate();
    }
};

class TryStatement : public Statement
{
    Statement*  block;
    Catch*      catchBlock;
    Finally*    finallyBlock;

public:
    TryStatement(Statement* block, Catch* catchBlock, Finally* finallyBlock) :
        block(block),
        catchBlock(catchBlock),
        finallyBlock(finallyBlock)
    {
    }
    ~TryStatement()
    {
        delete block;
        if (catchBlock)
        {
            delete catchBlock;
        }
        if (finallyBlock)
        {
            delete finallyBlock;
        }
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "try\n";
        s += block->toString(indent);
        if (catchBlock)
        {
            s += catchBlock->toString(indent);
        }
        if (finallyBlock)
        {
            s += finallyBlock->toString(indent);
        }
        return s;
    };

    CompletionType evaluate()
    {
        CompletionType c;
        try
        {
            c = block->evaluate();
        }
        catch (Value* value)
        {
            c.setType(CompletionType::Throw);
            c.setValue(value);
        }
        Register<Value> v = c.getValue();
        if (c.isThrow() && catchBlock)
        {
            c = catchBlock->evaluate(c);
        }
        if (finallyBlock)
        {
            CompletionType result = finallyBlock->evaluate();
            if (!result.isNormal())
            {
                return result;
            }
        }
        return c;
    }
};

// SourceElements

class SourceElements : public Code
{
    mutable SourceElement::List list;

public:
    virtual ~SourceElements()
    {
        while (!list.isEmpty())
        {
            SourceElement* element = list.removeLast();
            delete element;
        }
    };

    void add(SourceElement* element)
    {
        list.addLast(element);
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        SourceElement::List::Iterator iter = list.begin();
        while (SourceElement* element = iter.next())
        {
            s += element->toString(indent);
        }
        return s;
    }

    void process()
    {
        SourceElement::List::Iterator iter = list.begin();
        while (SourceElement* element = iter.next())
        {
            element->process();
        }
    }

    CompletionType evaluate()
    {
        CompletionType  result;
        Register<Value> value;

        process();

        SourceElement::List::Iterator iter = list.begin();
        while (SourceElement* element = iter.next())
        {
            result = element->evaluate();
            value = result.getValue();
            if (result.isAbrupt())
            {
                return result;
            }
        }
        return result;
    }
};

// FunctionDeclaration

class FormalParameterList
{
    mutable Identifier::List    list;
    int length;

public:
    FormalParameterList() :
        length(0)
    {
    }

    virtual ~FormalParameterList()
    {
        while (!list.isEmpty())
        {
            Expression* element = list.removeLast();
            delete element;
        }
    };

    void add(Identifier* parameter)
    {
        ++length;
        list.addLast(parameter);
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        Expression::List::Iterator iter = list.begin();
        while (Expression* parameter = iter.next())
        {
            s += parameter->toString(indent);
            if (iter.hasNext())
            {
                s += ", ";
            }
        }
        return s;
    };

    void instantiate(ObjectValue* variableObject, ListValue* arguments)
    {
        size_t arg = 0;

        Expression::List::Iterator iter = list.begin();
        while (Identifier* parameter = static_cast<Identifier*>(iter.next()))
        {
            variableObject->put(parameter->toString(), (*arguments)[arg++]);
        }
    }

    int getLength() const
    {
        return length;
    }
};

class FunctionDeclaration : public SourceElement
{
    Identifier*             identifier;
    FormalParameterList*    formalParameterList;
    SourceElements*         functionBody;

public:
    FunctionDeclaration(Identifier* identifier,
                        FormalParameterList* formalParameterList,
                        SourceElements* functionBody) :
        identifier(identifier),
        formalParameterList(formalParameterList),
        functionBody(functionBody)
    {
    }

    ~FunctionDeclaration()
    {
        delete identifier;
        delete formalParameterList;
        delete functionBody;
    }

    std::string toString(std::string indent) const
    {
        std::string s = indent;

        s += "function ";
        s += identifier->toString(indent);
        s += "(";
        s += formalParameterList->toString(indent);
        s += ")\n";
        s += indent + "{\n";
        s += functionBody->toString(indent + "    ");
        s += indent + "}\n";
        return s;
    }

    // 13 Function Definition
    void process()
    {
        Register<ObjectValue> function = new ObjectValue();
        function->setPrototype(getGlobal()->get("Function")->getPrototype());
        function->setParameterList(formalParameterList);
        function->setCode(functionBody);
        function->setScope(getScopeChain());
        function->setIdentifier(identifier);
        Register<ObjectValue> prototype = new ObjectValue;
        function->put("prototype", prototype);
        getScopeChain()->put(identifier->toString(), function);
    }

    CompletionType evaluate()
    {
        return CompletionType(CompletionType::Normal, 0, "");
    };
};

class FunctionExpression : public Expression
{
    Identifier*             identifier;
    FormalParameterList*    formalParameterList;
    SourceElements*         functionBody;

public:
    FunctionExpression(Identifier* identifier,
                       FormalParameterList* formalParameterList,
                       SourceElements* functionBody) :
        identifier(identifier),
        formalParameterList(formalParameterList),
        functionBody(functionBody)
    {
    }

    ~FunctionExpression()
    {
        if (identifier)
        {
            delete identifier;
        }
        delete formalParameterList;
        delete functionBody;
    }

    std::string toString(std::string indent) const
    {
        std::string s;

        s += "function ";
        if (identifier)
        {
            s += identifier->toString(indent);
        }
        s += "(";
        s += formalParameterList->toString(indent);
        s += ")\n";
        s += indent + "{\n";
        s += functionBody->toString(indent + "    ");
        s += indent + "}";
        return s;
    }

    Value* evaluate()
    {
        Register<ObjectValue> function = new ObjectValue();
        function->setParameterList(formalParameterList);
        if (!identifier)
        {
            function->setPrototype(getGlobal()->get("Function")->getPrototype());
            function->setScope(getScopeChain());
        }
        else
        {
            Register<ObjectValue> object = new ObjectValue();
            object->setNext(getScopeChain());
            object->put(identifier->toString(), function); // with { DontDelete, ReadOnly }
            function->setScope(object);
            function->setIdentifier(identifier);
        }
        function->setCode(functionBody);
        Register<ObjectValue> prototype = new ObjectValue;
        function->put("prototype", prototype);
        return function;
    };
};

SourceElements* getProgram();
void setProgram(SourceElements* elements);

void setSource(const std::string& s);
void setSource(es::Stream* stream);

#endif  // NINTENDO_ESJS_H_INCLUDED
