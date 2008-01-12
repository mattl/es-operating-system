// test setUTCDate

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

tick = Date.UTC(1970, 1-1, 31, 15, 0, 1 /* UTC */);
d = new Date(tick);
s = d.toUTCString(); // UTC
stdout.write(s + '\n', s.length + 1);

d.setUTCDate(15);
s = d.toUTCString();
check(s == "Thu Jan 15 15:00:01 1970 UTC");

