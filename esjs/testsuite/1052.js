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
check(a.slice(1, 2).join(" ") == "ab");
check(a.slice(-3, 5).join(" ") == "g cd");
check(a.slice(-3, -1).join(" ") == "g cd");
check(a.slice(1, 100).join(" ") == "ab ef g cd i");
check(a.join(" ") == "h ab ef g cd i");
check(a.slice(-1, -2).join(" ") == "");
check(a.slice(1).join(" ") == "ab ef g cd i");
check(a.slice().join(" ") == "h ab ef g cd i");
