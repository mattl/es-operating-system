// test setUTCMonth().

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

tick = Date.UTC(1999, 1-1, 31, 15, 0, 1 /* UTC */);
d = new Date(tick);
s = d.toUTCString();
stdout.write(s + '\n', s.length + 1);

d.setUTCMonth(12-1);
s = d.toUTCString();
check(s == "Fri Dec 31 15:00:01 1999 UTC");

d.setUTCMonth(12-1, 30);
s = d.toUTCString();
check(s == "Thu Dec 30 15:00:01 1999 UTC");

// 3rd. argumet is ignored.
d.setUTCMonth(12-1, 30, 23);
s = d.toUTCString();
check(s == "Thu Dec 30 15:00:01 1999 UTC");
