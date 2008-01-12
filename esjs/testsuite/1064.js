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

s = new String("Hello");
r = s.concat(" ", "World");
check(r == "Hello World");
check(s.toString() == "Hello");
