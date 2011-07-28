var i = 0;
var a = 0;
var b = 0;

for (i = 1; i <= 1000; i += 4)
{
    a += 1 / (i * Math.pow(5 , i)) - 1 / ((i + 2) * Math.pow(5 ,(i + 2)));
    b += 1 / (i * Math.pow(239 , i)) - 1 / ((i + 2) * Math.pow(239 , (i + 2)));
}

pi = a * 16 - b * 4;
stdout.write(pi.toString(), pi.toString().length + 1);
