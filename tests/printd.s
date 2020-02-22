.globl printd
printd:
	lw $a0, -8($sp)
	li $v0, 1
	syscall
	lw $ra, 0($sp)
	jr $ra
