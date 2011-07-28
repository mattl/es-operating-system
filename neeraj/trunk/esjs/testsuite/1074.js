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

s = "Hello World!";
check(s.substr(0, 5) == "Hello");
check(s.substr(1, 2) == "el");
check(s.substr(1, 1000) == "ello World!");
check(s.substr(-5, 2) == "or");
check(s.substr(0) == "Hello World!");
check(s.substr(6) == "World!");
check(s.substr() == "Hello World!");
check(s.substr(0, -5) == "");

// s = "こんにちは世界";
// s.substr(1, 2);
