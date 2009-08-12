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
b = a.concat("hello", "world");
check(b.join(" ") == a.join(" ") + " " + "hello world");

d = new Array();
c = new Array(1, 10 ,100, 1000);
e = d.concat(a, c);
check(e.join(" ") == a.join(" ") + " " + c.join(" "));

