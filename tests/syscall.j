function main is
    assign syscall3 1 1 "Hello, World!\n" 14. to result.
    if result equal 14 do
        drop syscall3 1 1 "Success!\n" 9.
    done.
done.
