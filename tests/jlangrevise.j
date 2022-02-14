; redesign for jlang

function puts(msg as pointer, len as integer) yields integer is
	return syscall3(1, 1, msg, len)
done

function printsomething() yields integer is
	define test as integer is 1
	drop syscall3(1, 1, "Test\n", 5)
	return 0
done

function main() yields integer is
	drop printsomething()
	define result as integer is puts("Hello, World!\n", 14)
	if 14 equal result do
		drop syscall3(1, 1, "Success!\n", 9)
		return 0
	done
done