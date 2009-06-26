// test setUTCHours().

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

tick = Date.UTC(1900, 4-1, 1, 17, 1, 30 /* UTC */);
d = new Date(tick);
s = d.toUTCString(); // UTC
stdout.write(s + '\n', s.length + 1);

d.setUTCHours(23);
s = d.toUTCString();

check(s == "Sun Apr  1 23:01:30 1900 UTC");

d.setUTCHours(23, 10);
s = d.toUTCString();

check(s == "Sun Apr  1 23:10:30 1900 UTC");

d.setUTCHours(23, 10, 20);
s = d.toUTCString();

check(s == "Sun Apr  1 23:10:20 1900 UTC");

d.setUTCHours(23, 10, 20, 123);
s = d.toUTCString();

check(s == "Sun Apr  1 23:10:20 1900 UTC");
