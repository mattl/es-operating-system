// test setUTCMilliseconds().

stdout = System.output;
function check(result)
{
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

tick = Date.UTC(1900, 4-1, 1, 23, 10, 20); // UTC
d = new Date(tick);
s = d.toUTCString(); // UTC
stdout.write(s + '\n', s.length + 1);

d.setUTCMilliseconds(123);
s = d.toUTCString();
check(s == "Sun Apr  1 23:10:20 1900 UTC");
check(d.getUTCMilliseconds() == 123);
