.globl main
main:
	la $t0, __return_here
	sw $t0, 4($sp)
	j entry

__return_here:
	li $v0, 10
	syscall
.globl inputd
inputd:
	li $v0, 5
	syscall
	sw $v0, 0($sp)
	lw $ra, 4($sp)
	jr $ra
.globl prints
prints:
	lw $a0, 0($sp)
	li $v0, 4
	syscall
	lw $ra, 8($sp)
	jr $ra
.globl printd
printd:
	lw $a0, 0($sp)
	li $v0, 1
	syscall
	lw $ra, 8($sp)
	jr $ra
.globl inputs
inputs:
	lw $a0, 4($sp)
	lw $a1, 0($sp)
	li $v0, 8
	syscall
	lw $ra, 12($sp)
	jr $ra
.globl inputc
inputc:
	li $v0, 12
	syscall
	sw $v0, 0($sp)
	lw $ra, 4($sp)
	jr $ra
.globl printc
printc:
	lb $a0, 0($sp)
	li $v0, 11
	syscall
	lw $ra, 8($sp)
	jr $ra
.globl malloc
malloc:
	lw $a0, 0($sp)
	li $v0, 9
	syscall
	sw $v0, 4($sp)
	lw $ra, 8($sp)
	jr $ra
