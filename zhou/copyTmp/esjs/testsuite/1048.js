function defaultCompare(x, y)
{
    if (y < x)
    {
        return 1;
    }
    else if (x < y)
    {
        return -1;
    }
    return 0;
}

function reverseCompare(x, y)
{
    if (y < x)
    {
        return -1;
    }
    else if (x < y)
    {
        return 1;
    }
    return 0;
}

/*
function invalidCompare(x, y)
{
    return NaN;
}
*/

function check(result)
{
    stdout = System.output;
    if (result)
    {
        stdout.write("OK\n", 3);
    }
    else
    {
        stdout.write("*** ERROR ***\n", 14);
    }
    return result;
}

a = new Array("h", "ab", "ef", "g", "cd", "i");
check(a.sort().join(" ") == "ab cd ef g h i");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a;

a.sort(defaultCompare);
b.sort(reverseCompare);
check(a.reverse().join(" ") == b.join(" "));

c = new Array("h", "ab", "ef", "g", "cd", "i");
delete c[1];
check(c.sort().join(",") == "cd,ef,g,h,i,");

delete c[0];
delete c[1];
delete c[2];
delete c[3];
delete c[4];
c.sort();
check(c.join(",") == ",,,,,");

d = new Array();
check(d.sort().join(",") == "");

// [check]
// If comparefn is not undefined and is not a consistent comparison function for
// the elements of this array, the behaviour of sort is implementation-defined.
// a = new Array("h", "ab", "ef", "g", "cd", "i");
// a.sort(invalidCompare);

