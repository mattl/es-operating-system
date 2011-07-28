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

map = getOrderedMap();
// name setter and name getter
map.apple = "foo";
check(map.apple == "foo");
// index getter
check(map[0] == "foo");
// index setter
map[0] = "bar";
check(map[0] == "bar");
check(map.apple == "bar");

map.banana = "baz";
check(map.size == 2);
