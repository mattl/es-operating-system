function check(result)
{
    stdout = System.getOut();
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

check(1); // dummy.
