a = "abcabb";
b = a.replace("bca", function (matched, index, string) { return "'" + matched + "'"; });
b;
