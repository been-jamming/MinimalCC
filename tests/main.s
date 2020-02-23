.globl main
main:
	la $t0, return_here
	sw $ra, 4($sp)
	sw $t0, 0($sp)
	j entry

return_here:
	lw $ra, 4($sp)
	jr $ra
