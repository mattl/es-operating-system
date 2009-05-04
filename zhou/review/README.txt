The patch fixes the following warnnings in es-0.1.4.

1)

../../../trunk/os/kernel/pc/gdbstub.cpp:604: warning: deprecated conversion from string constant to ‘char*’

2)I think this is an error.

../../../trunk/os/kernel/pc/sb16.cpp:85: warning: large integer implicitly truncated to unsigned type
../../../trunk/os/kernel/pc/sb16.cpp:85: warning: large integer implicitly truncated to unsigned type
../../../trunk/os/kernel/pc/es1370.cpp:364: warning: large integer implicitly truncated to unsigned type
../../../trunk/os/kernel/pc/es1370.cpp:364: warning: large integer implicitly truncated to unsigned type
../../../trunk/os/kernel/pc/es1370.cpp:364: warning: large integer implicitly truncated to unsigned type

3)

../../../trunk/os/kernel/port/pageSet.cpp:376:2: warning: no newline at end of file


4)There may be potential bugs. For example, if rc is less than zero,the result of "if (sizeof(RpcHdr) <= rc)" may be true which is against the logic.

../../../trunk/os/libes++/src/rpc.cpp:206: warning: comparison between signed and unsigned integer expressions
../../../trunk/os/libes++/src/rpc.cpp:214: warning: comparison between signed and unsigned integer expressions
../../../trunk/os/libes++/src/rpc.cpp:219: warning: comparison between signed and unsigned integer expressions

