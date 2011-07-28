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

check("ab".split(/a*?/).join(",") == "a,b");
check("ab".split(/a*/).join(",") == ",b");
check("A<B>bold</B>and<CODE>coded</CODE>".split(/<(\/)?([^<>]+)>/).join(",") == "A,,B,bold,/,B,and,,CODE,coded,/,CODE,");
check("abc".split(/a/).join(",") == ",bc");
check("abc".split(/b/).join(",") == "a,c");
check("abcabcabc".split(/ab/).join(",") == ",c,c,c");
check("Hello World!".split(/[el Wo rl !]/).join(",") == "H,,,,,,,,,d,");
check("2/14/2000 16:45:12".split(/[\/ :]/, 2).join(",") == "2,14");
check("2/14/2000 16:45:12".split(/[\/ :]/, 0).join(",") == "");
check("abc".split("a").join(",") == ",bc");
check("abc".split("").join(",") == "a,b,c");
check("abc".split().join(",") == "abc");
check("あいう".split("").join(",") == "あ,い,う");

check("".split('').length == 0);
check("".split('a').length == 1);
check("".split(/a/).length == 1);
check("".split(/a?/).length == 0);
