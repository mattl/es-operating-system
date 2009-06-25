#!/usr/bin/awk -f

BEGIN {
    print "#include \"interfaceData.h\""
    print "#include <es/any.h>"
    print "#include <es/includeAllInterfaces.h>"
    print ""
    print "namespace es"
    print "{"
    print ""
    print "InterfaceData interfaceData[] = {"
}

{
    print "    { " $0 "::iid, " $0 "::info },"
}

END {
    print "    { 0, 0 }"
    print "};"
    print ""
    print "}  // namespace es"
}
