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


uri = "-_.!~*'()";
check(encodeURI("#" + uri) == "#" + uri);
check(encodeURIComponent("#" + uri) == "%23" + uri);

uri = "http://www.nintendo.co.jp";
encoded = encodeURI(uri);
check(encoded == uri);
check(decodeURI(encoded) == uri);
check(decodeURIComponent(encoded) == uri);

encoded = encodeURIComponent(uri);
check(encoded == "http%3A%2F%2Fwww.nintendo.co.jp");
check(decodeURI(encoded) == "http%3A%2F%2Fwww.nintendo.co.jp");
check(decodeURIComponent(encoded) == uri);
