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

a = new Array("こんにちは", "世界", "！");
check(a.join("　") == "こんにちは　世界　！");

b = new Array("あ", "か",
              "い", "き",
              "う", "く",
              "え", "け",
              "お", "こ");
b.sort();
check(b.join("") == "あいうえおかきくけこ");
