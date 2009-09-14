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
head = a.shift();
check(head == "h");
a.join();
check(a.join(" ") == "ab ef g cd i");
