// test setUTCSeconds().

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

tick = Date.UTC(1900, 4-1, 1, 23, 10, 30 /* UTC */);
d = new Date(tick);
s = d.toUTCString();
stdout.write(s + '\n', s.length + 1);

d.setUTCSeconds(20);
s = d.toUTCString();

check(s == "Sun Apr  1 23:10:20 1900 UTC");

d.setUTCSeconds(20, 123);
s = d.toUTCString();

check(s == "Sun Apr  1 23:10:20 1900 UTC");
check(d.getUTCMilliseconds() == 123);
