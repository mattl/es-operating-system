// check a properties of date constructor, parse().
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

s = new String("Thu Jul 12 16:20:57 2007" /* localtime */);
stdout.write("input  : \"" + s + "\"\n", s.length + 13);

tick = Date.parse(s); // parse s as a localtime.
d = new Date(tick);
s2 = d.toString();

stdout.write("output : \"" + s2 + "\"\n", s2.length + 13);

check(s.toString() == s2.toString());
