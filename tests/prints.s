.globl prints
prints:
	lw $a0, -8($sp)
	li $v0, 4
	syscall
	lw $ra, 0($sp)
	jr $ra
