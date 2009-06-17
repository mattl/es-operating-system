// check Date(number)
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

stdout = System.output;

stdout.write("origin: ", 8);
d = new Date(0);
s = String(d);
stdout.write(s, s.length);
stdout.write("\n", 1);

check(s == "Thu Jan  1 09:00:00 1970");

stdout.write(" +2min : ", 8);
d = new Date(60000 * 2);
s = String(d);
stdout.write(s + '\n', s.length + 1);

check(s == "Thu Jan  1 09:02:00 1970");

stdout.write(" +1day : ", 8);
d = new Date(60000 * 60 * 24);
s = String(d);
stdout.write(s + '\n', s.length + 1);

check(s == "Fri Jan  2 09:00:00 1970");

