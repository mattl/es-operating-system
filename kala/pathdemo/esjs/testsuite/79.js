function f(a, b, c)
{
    var x = a + b + c;
    x = x ? true : false;
    if (a || b)
    {
        x = c;
    }
    else
    {
        x = !c;
    }

    switch (c)
    {
    case 0:
        c *= 2;
        break;
    default:
        c *= 3;
        break;
    }

    return x;
}

f.toString();
