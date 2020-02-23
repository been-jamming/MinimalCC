.globl malloc
malloc:
	lw $a0, -8($sp)
	li $v0, 9
	syscall
	sw $v0, -4($sp)
	lw $ra, 0($sp)
	jr $ra
