.globl main
main:
	la $t0, return_here
	sw $ra, 4($sp)
	sw $t0, 0($sp)
	li $t0, 55
	sb $t0, -8($sp)
	j guessing_game

return_here:
	lw $ra, 4($sp)
	jr $ra
