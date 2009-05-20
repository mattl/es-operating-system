// check Date function.

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

d = new Date(NaN);
check(d.toString() == "NaN");
check(isNaN(d.valueOf()));

d = new Date(NaN, 12-1);
check(isNaN(d.valueOf()));

d = new Date(1970, NaN, 1);
check(isNaN(d.valueOf()));

d = new Date("NaN");
check(isNaN(d.valueOf()));
check(isNaN(d.getYear()));
check(isNaN(d.getMilliseconds()));

d = new Date();
check(isNaN(d.setYear(NaN)));
check(isNaN(d.setSeconds(NaN)));

check(isNaN(Date.UTC(NaN, 12-1)));
check(isNaN(Date.UTC(2007, 1-1, 2, NaN)));
check(isNaN(Date.UTC(2007, 1-1, 2, NaN)));

d = new Date(NaN);
t = new Date();
t.setTime(d);
check(isNaN(t.valueOf()));
