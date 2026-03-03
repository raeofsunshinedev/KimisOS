# System Calls

System calls are executed by calling `int 0x80` with `eax` equal to the number of the system call you are executing. Arguments to system calls are stored in the other general purpose registers. The meaning of each register for each system call can be seen in the system call table. (TODO: Implement system calls)

|Name|EAX|EBX|ECX|EDX|ESI|EDI|
|----|---|---|---|---|---|---|
|Exit|0  | Exit_code|-|-|-|-|