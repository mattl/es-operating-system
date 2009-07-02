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

var encoded = "%E381%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF%E4%B8%96%E7%95%8C%20(Hello%20World)";

try
{
    decodeURI(encoded);
}
catch (e)
{
    a = e;
}
check(a.name == "URIError");



