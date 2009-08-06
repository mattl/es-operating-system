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

s = "lineTo(  12 ,  34 )".replace(/\s/g, ""); // remove white spaces.
check(s == "lineTo(12,34)");

s = "lineTo(12,34,56)".split(/[\( \) ,]/).join(" "); // get argv[].
check(s == "lineTo 12 34 56 ");

