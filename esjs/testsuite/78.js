function x(a, b, c)
{
    return a + b + c;
}

x.call(null, 1, 2, 3);      // == 6
x.apply(null, [1, 2, 3]);   // == 6
