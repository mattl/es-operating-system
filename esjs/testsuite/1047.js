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

a = new Array("ab", "cd", "ef", "g", "h", "i");
a.reverse();

check(a.join(" ") == "i h g ef cd ab");

b = new Array("ab", "cd", "ef", "g", "h");
b.reverse();

check(b.join(" ") == "h g ef cd ab");
