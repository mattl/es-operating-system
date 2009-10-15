x = "\\";
a = "$1,$2";
r = new RegExp("(\$(\d))", "g");
b = a.replace(r, "$$1-$1$2");
b;
