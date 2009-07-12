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

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(1, 2, 3, 4, 5);
check(a.join(" ") == "h 3 4 5 g cd i");
check(b.join(" ") == "ab ef");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(1, 3, "hello");
check(a.join(" ") == "h hello cd i");
check(b.join(" ") == "ab ef g");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(3, 2, "hello", "world");
check(a.join(" ") == "h ab ef hello world i");
check(b.join(" ") == "g cd");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(3, 2, "hello", "world", "!");
check(a.join(" ") == "h ab ef hello world ! i");
check(b.join(" ") == "g cd");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(-1, 2, "hello", "world", "!");
check(a.join(" ") == "h ab ef g cd hello world !");
check(b.join(" ") == "i");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(-10, 2, "hello", "world", "!");
check(a.join(" ") == "hello world ! ef g cd i");
check(b.join(" ") == "h ab");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(0, 7, "hello", "world", "!");
check(a.join(" ") == "hello world !");
check(b.join(" ") == "h ab ef g cd i");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(0, 0, "hello", "world", "!");
check(a.join(" ") == "hello world ! h ab ef g cd i");
check(b.join(" ") == "");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(0, -1, "hello", "world", "!");
check(a.join(" ") == "hello world ! h ab ef g cd i");
check(b.join(" ") == "");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(0, 3, "a", "b");
check(a.join(",") == "a,b,g,cd,i");
check(b.join(",") == "h,ab,ef");

a = new Array("h", "ab", "ef", "g", "cd", "i");
b = a.splice(0, 3, "");
check(a.join(",") == ",g,cd,i");
check(b.join(" ") == "h ab ef");
