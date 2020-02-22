.globl inputd
inputd:
	li $v0, 5
	syscall
	sw $v0, -4($sp)
	lw $ra, 0($sp)
	jr $ra
