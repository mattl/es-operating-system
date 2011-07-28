// check Date Constructor.
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
// Months are identified by an integer in the range 0 to 11.
d = new Date(2007, 7-1, 11, 11, 53, 1, 234 /* localtime */);
check(d.getTime() == 1184122381234); // `date -d "11:53:01.234 07/11/2007" +%s%N -u` / 1000000
