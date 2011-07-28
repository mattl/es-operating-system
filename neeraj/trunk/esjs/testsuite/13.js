sum = 0;
i = 1;
lable:
while (i < 10)
{
    sum = sum + i;
    i = i + 1;
    j = 1;
    while (j < 10)
    {
        break label;
        sum = sum + j;
        j = j + 1;
    }
}
sum;
