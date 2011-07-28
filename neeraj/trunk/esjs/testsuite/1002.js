// Date as Constructor called as a function.
function print(s)
{
    stdout = System.output;
    stdout.write(s, s.length);
    stdout.write("\n", 1);
    return 0;
}

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
s = Date(1192, 7-1, 11, 12, 34, 1, 234);
now = (new Date()).toString();
print(s);
print(now);
check(s == now); // [check]
check(s != "Sat Jul 11 12:34:01 1192"); // any arguments supplied are accepted but are completely ignored.
