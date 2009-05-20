stdout = System.output;
function check(result)
{
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

x = new Date(2007, 7-1);

/*
   [ECMAScript Language Specification 15.9.4.2]
   all of the following expressions should produce the same numeric value in that
   implementation, if all the properties referenced have their initial values:
   x.valueOf()
   Date.parse(x.toString())
   Date.parse(x.toUTCString())
*/

a = x.valueOf();
b = Date.parse(x.toString());
check (a == b);

// [check] test in future releases.
// c = Date.parse(x.toUTCString()); // should parse UTC date format.
// check (a == c);                  // but, not implemeted yet.

