// check toObject().
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

n = 150;
check(n.toString() == "150");
