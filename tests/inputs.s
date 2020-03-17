.globl inputs
inputs:
	lw $a0, -8($sp)
	lw $a1, -12($sp)
	li $v0, 8
	syscall
	lw $ra, 0($sp)
	jr $ra
