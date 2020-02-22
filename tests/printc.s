.globl printc
printc:
	lb $a0, -8($sp)
	li $v0, 11
	syscall
	lw $ra, 0($sp)
	jr $ra
