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

check(Number.MIN_VALUE < Number.MAX_VALUE); // [check]
check(isNaN(Number.NaN));
check(Number.POSITIVE_INFINITY == Infinity);
check(Number.NEGATIVE_INFINITY == -Infinity);