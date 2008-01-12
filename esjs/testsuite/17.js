a = Object();
a.member = 3;
b = 0;
with (a)
{
    b = member;
}
b;
