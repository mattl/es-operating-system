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
n = a.unshift(1, 2, 3, 4, 5, "hello", "world!");
check(n == 13); // [check] return value is different from that in IE6.
check(a.join(" ") == "1 2 3 4 5 hello world! h ab ef g cd i");

a = new Array("h", "ab", "ef", "g", "cd", "i");
s = a.join(" ");
n = a.unshift();
check(a.join(" ") == s);
